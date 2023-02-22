#ifndef RDF4CPP_DATATYPEREGISTRY_HPP
#define RDF4CPP_DATATYPEREGISTRY_HPP

#include <rdf4cpp/rdf/datatypes/LiteralDatatype.hpp>
#include <rdf4cpp/rdf/datatypes/registry/DatatypeConversion.hpp>
#include <rdf4cpp/rdf/datatypes/registry/FixedIdMappings.hpp>

#include <any>
#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace rdf4cpp::rdf::datatypes::registry {

/**
 * Registry for LiteralDatatype implementations.
 * Data types are registered by defining, implementing and specializing members of LiteralDatatype.
 * @see LiteralDatatype
 */
class DatatypeRegistry {
public:
    static constexpr size_t dynamic_datatype_offset = min_dynamic_datatype_id - 1; // ids from 1 to n stored in places 0 to n-1

    /**
     * Constructs an instance of a type from a string.
     */
    using factory_fptr_t = std::any (*)(std::string_view);
    using to_string_fptr_t = std::string (*)(std::any const &) noexcept;
    using ebv_fptr_t = bool (*)(std::any const &) noexcept;
    using try_into_inlined_fptr_t = std::optional<uint64_t> (*)(std::any const &) noexcept;
    using from_inlined_fptr_t = std::any (*)(uint64_t) noexcept;

    struct NumericOpResult {
        DatatypeID result_type_id;
        nonstd::expected<std::any, DynamicError> result_value;
    };

    using nullop_fptr_t = std::any (*)() noexcept;
    using unop_fptr_t = NumericOpResult (*)(std::any const &) noexcept;
    using binop_fptr_t = NumericOpResult (*)(std::any const &, std::any const &) noexcept;

    using compare_fptr_t = std::partial_ordering (*)(std::any const &, std::any const &) noexcept;

    struct NumericOpsImpl {
        nullop_fptr_t zero_value_fptr; // 0
        nullop_fptr_t one_value_fptr; // 1

        binop_fptr_t add_fptr;  // a + b
        binop_fptr_t sub_fptr;  // a - b
        binop_fptr_t mul_fptr;  // a * b
        binop_fptr_t div_fptr;  // a / b

        unop_fptr_t pos_fptr;  // +a
        unop_fptr_t neg_fptr;  // -a

        unop_fptr_t abs_fptr;   // abs(a)
        unop_fptr_t round_fptr; // round(a)
        unop_fptr_t floor_fptr; // floor(a)
        unop_fptr_t ceil_fptr;  // ceil(a)
    };

    struct NumericOpsStub {
        /**
         * Determines the number of subtype levels to immediately skip up in
         * the hierarchy, if searching for a _numeric_ type conversion.
         *
         * @example if start_s_off = 1 then the search will _start_ at the immediate supertype
         *      of the type this NumericOpsStub belongs to.
         */
        size_t start_s_off;
    };

    struct NumericOps : public std::variant<NumericOpsStub, NumericOpsImpl> {
        [[nodiscard]] constexpr bool is_stub() const noexcept {
            return this->index() == 0;
        }

        [[nodiscard]] constexpr bool is_impl() const noexcept {
            return this->index() == 1;
        }

        [[nodiscard]] constexpr NumericOpsStub const &get_stub() const noexcept {
            return std::get<NumericOpsStub>(*this);
        }

        [[nodiscard]] constexpr NumericOpsImpl const &get_impl() const noexcept {
            return std::get<NumericOpsImpl>(*this);
        }
    };

    struct InliningOps {
        try_into_inlined_fptr_t try_into_inlined_fptr;
        from_inlined_fptr_t from_inlined_fptr;
    };

    struct DatatypeEntry {
        std::string datatype_iri;                   // datatype IRI string
        factory_fptr_t factory_fptr;                // construct from string
        to_string_fptr_t to_canonical_string_fptr;  // convert to canonical string
        to_string_fptr_t to_simplified_string_fptr; // convert to simplified string (e.g. for casting to xsd:string)

        ebv_fptr_t ebv_fptr; // convert to effective boolean value

        std::optional<NumericOps> numeric_ops;
        std::optional<InliningOps> inlining_ops;

        compare_fptr_t compare_fptr;

        RuntimeConversionTable conversion_table;

        inline static DatatypeEntry placeholder() noexcept {
            return DatatypeEntry{
                    .datatype_iri = "",
                    .factory_fptr = nullptr,
                    .to_canonical_string_fptr = nullptr,
                    .to_simplified_string_fptr = nullptr,
                    .ebv_fptr = nullptr,
                    .numeric_ops = std::nullopt,
                    .inlining_ops = std::nullopt,
                    .compare_fptr = nullptr,
                    .conversion_table = RuntimeConversionTable::empty()};
        }

        inline static DatatypeEntry for_search(std::string_view const datatype_iri) noexcept {
            return DatatypeEntry{
                    .datatype_iri = std::string{datatype_iri},
                    .factory_fptr = nullptr,
                    .to_canonical_string_fptr = nullptr,
                    .to_simplified_string_fptr = nullptr,
                    .ebv_fptr = nullptr,
                    .numeric_ops = std::nullopt,
                    .inlining_ops = std::nullopt,
                    .compare_fptr = nullptr,
                    .conversion_table = RuntimeConversionTable::empty()};
        }
    };

    struct DatatypeConverter {
        DatatypeIDView target_type_id;
        RuntimeConversionEntry::convert_fptr_t convert_lhs;
        RuntimeConversionEntry::inverted_convert_fptr_t inverted_convert_lhs;

        RuntimeConversionEntry::convert_fptr_t convert_rhs;
        RuntimeConversionEntry::inverted_convert_fptr_t inverted_convert_rhs;

        inline static DatatypeConverter from_individuals(RuntimeConversionEntry const &l, RuntimeConversionEntry const &r) noexcept {
            assert(l.target_type_id == r.target_type_id);

            return DatatypeConverter{
                    .target_type_id = l.target_type_id,
                    .convert_lhs = l.convert,
                    .inverted_convert_lhs = l.inverted_convert,
                    .convert_rhs = r.convert,
                    .inverted_convert_rhs = r.inverted_convert};
        }
    };

private:
    using registered_datatypes_t = std::vector<DatatypeEntry>;

    static registered_datatypes_t &get_mutable() noexcept;

    /**
     * Tries to find the datatype corresponding to datatype_id
     * if found `f` is applied to it to form the function result.
     *
     * @param datatype_iri the datatype to search
     * @param f function that maps a DatatypeEntry to (typically) a member of it
     * @return the result of `f` applied to the DatatypeEntry, if it exists, else nullopt
     */
    template<typename Map>
        requires std::invocable<Map, DatatypeEntry const &>
    static std::optional<std::invoke_result_t<Map, DatatypeEntry const &>> find_map_entry(DatatypeIDView datatype_id, Map f) noexcept(std::is_nothrow_invocable_v<Map, DatatypeEntry const &>);

    /**
     * Creates NumericOps based on a NumericLiteralDatatype
     * by generating type-erased versions of all necessary functions (add, sub, ...).
     */
    template<datatypes::NumericImplLiteralDatatype LiteralDatatype_t>
    static NumericOpsImpl make_numeric_ops_impl() noexcept;

    template<datatypes::InlineableLiteralDatatype LiteralDatatype_t>
    static InliningOps make_inlining_ops() noexcept;

    static void add_fixed(DatatypeEntry entry_to_add, LiteralType type_id) noexcept;

public:
    /**
     * Auto-register a datatype that fulfills DatatypeConcept
     * @tparam datatype_info type that is registered.
     */
    template<datatypes::LiteralDatatype LiteralDatatype_t>
    static void add() noexcept;

    /**
     * Register an datatype manually
     */
    static void add(DatatypeEntry entry_to_add) noexcept;

    /**
     * Retrieve all registered datatypes.
     * @return vector of DatatypeEntries
     */
    [[nodiscard]] static registered_datatypes_t const &registered_datatypes() noexcept;

    /**
     * Get the database entry for a datatype_id.
     * @param datatype_id datatype id of the datatype
     * @return if available database entry else nullopt
     */
    [[nodiscard]] static DatatypeEntry const *get_entry(DatatypeIDView datatype_id) noexcept;

    /**
     * Get the IRI for a datatype_id
     *
     * @param datatype_id datatype id of the datatype
     * @return if the type exists then its IRI else nullopt
     */
    [[nodiscard]] static std::optional<std::string_view> get_iri(DatatypeIDView datatype_id) noexcept;

    /**
     * Get a factory_fptr_t for a datatype. The factory_fptr_t can be used like `std::any type_instance = factory_fptr("types string representation")`.
     * @param datatype_id datatype id for the corresponding datatype
     * @return function pointer or nullptr
     */
    [[nodiscard]] static factory_fptr_t get_factory(DatatypeIDView datatype_id) noexcept;

    /**
     * Get a to_canonical_string function for a datatype. The factory_fptr_t can be used like `std::string str_repr = to_canonical_string_fptr(any_typed_arg)`.
     * @param datatype_id datatype id for the corresponding datatype
     * @return function pointer or nullptr
     */
    [[nodiscard]] static to_string_fptr_t get_to_canonical_string(DatatypeIDView datatype_id) noexcept;

    /**
     * Get the to_simplified_string function for a datatype. This function can be used for user friendly output.
     * @param datatype_id datatype id for the corresponding datatype
     * @return function pointer if datatype was found else nullptr
     */
    [[nodiscard]] static to_string_fptr_t get_to_simplified_string(DatatypeIDView datatype_id) noexcept;

    /**
     * Try to get the numerical ops function table for a datatype.
     * Returns nullptr if the datatype is not numeric, or does not exist.
     *
     * @param datatype_id datatype id for the corresponding datatype
     * @return if available a pointer to a structure containing function pointers for all numeric ops; otherwise a null pointer
     */
    [[nodiscard]] static NumericOps const *get_numerical_ops(DatatypeIDView datatype_id) noexcept;

    /**
     * Try to get the conversion function that converts a value of a datatype to it's effective boolean value.
     * Returns nullptr if the datatype is not logical, or does not exist.
     *
     * @param datatype_id datatype id for the corresponding datatype
     * @return if available a the conversion function
     */
    [[nodiscard]] static ebv_fptr_t get_ebv(DatatypeIDView datatype_id) noexcept;

    /**
     * Try to get the comparison function for a datatype.
     * Returns nullptr if the datatype is not comparable, or does not exist.
     *
     * @param datatype_id datatype id for the corresponding datatype
     * @return if available the comparison function
     */
    [[nodiscard]] static compare_fptr_t get_compare(DatatypeIDView datatype_id) noexcept;

    /**
     * Try to get the inlining ops structure for a given datatype.
     *
     * @param datatype_id datatype id for the corresponding datatype
     * @return A pointer to the inlining ops of the given datatype if it exists and fullfills Inlineable, else nullptr
     */
    [[nodiscard]] static InliningOps const *get_inlining_ops(DatatypeIDView datatype_id) noexcept;

    inline static RuntimeConversionTable const *get_conversion_table(DatatypeIDView const datatype_id) noexcept {
        auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
            return &entry.conversion_table;
        });

        return res.has_value() ? *res : nullptr;
    }

    /**
     * Tries to find a conversion to a common type in the conversion tables lhs_conv and rhs_conv.
     * @return the found conversion if there is a viable one
     */
    [[nodiscard]] static std::optional<DatatypeConverter> get_common_type_conversion(RuntimeConversionTable const &lhs_conv,
                                                                                     RuntimeConversionTable const &rhs_conv,
                                                                                     size_t lhs_init_soff = 0,
                                                                                     size_t rhs_init_soff = 0) noexcept;

    /**
     * Tries to find a conversion to a common type in the context of numeric operations.
     * This function is similar to get_common_type_conversion except that it is aware of numeric-stubs
     * and will always return a conversion to a numeric-impl or none at all.
     *
     * @note must be called with datatypes that are numeric
     * @return A conversion to a common type that is also a numeric-impl if there is a viable one
     */
    [[nodiscard]] static std::optional<DatatypeConverter> get_common_numeric_op_type_conversion(DatatypeEntry const &lhs_entry, DatatypeEntry const &rhs_entry) noexcept;

    /**
     * Returns the conversion that turns a value for a numeric-stub DatatypeEntry into
     * a value of it's corresponding impl-type.
     *
     * @note must be called with a datatype that is stub-numeric
     * @return the conversion to the corresponding impl-type
     */
    [[nodiscard]] static RuntimeConversionEntry const &get_numeric_op_impl_conversion(DatatypeEntry const &entry) noexcept;

    /**
     * Tries to find a conversion to convert lhs_type_id and rhs_type_id into a
     * common type to be used in e.g. numeric calculations.
     *
     * @return nullopt if any of lhs_type_id or rhs_type_id does not have a datatype registered
     * or there is no viable conversion, else the found conversion
     */
    [[nodiscard]] static std::optional<DatatypeConverter> get_common_type_conversion(DatatypeIDView lhs_type_id, DatatypeIDView rhs_type_id) noexcept;
};

template<typename Map>
    requires std::invocable<Map, DatatypeRegistry::DatatypeEntry const &>
std::optional<std::invoke_result_t<Map, DatatypeRegistry::DatatypeEntry const &>> DatatypeRegistry::find_map_entry(DatatypeIDView const datatype_id, Map f) noexcept(std::is_nothrow_invocable_v<Map, DatatypeEntry const &>) {

    using ret_type = std::optional<std::invoke_result_t<Map, DatatypeEntry const &>>;
    auto const &registry = registered_datatypes();

    return visit(DatatypeIDVisitor{
                         [&registry, f](LiteralType const fixed_id) -> ret_type {
                             auto const id_as_index = static_cast<size_t>(fixed_id.to_underlying()) - 1;  // ids from 1 to n stored in places 0 to n-1
                             assert(id_as_index < dynamic_datatype_offset);

                             return f(registry[id_as_index]);
                         },
                         [&registry, f](std::string_view const other_iri) -> ret_type {
                             auto found = std::lower_bound(registry.begin() + dynamic_datatype_offset, registry.end(),
                                                           DatatypeEntry::for_search(other_iri),
                                                           [](const auto &left, const auto &right) { return left.datatype_iri < right.datatype_iri; });

                             if (found != registry.end() and found->datatype_iri == other_iri) {
                                 return f(*found);
                             } else {
                                 return std::nullopt;
                             }
                         }},
                 datatype_id);
}

template<datatypes::LiteralDatatype LiteralDatatype_t>
inline void DatatypeRegistry::add() noexcept {
    using conversion_table_t = decltype(make_conversion_table_for<LiteralDatatype_t>());

    auto const num_ops = []() -> std::optional<NumericOps> {
        if constexpr (datatypes::NumericImpl<LiteralDatatype_t>) {
            return NumericOps{make_numeric_ops_impl<LiteralDatatype_t>()};
        } else if constexpr (datatypes::NumericStub<LiteralDatatype_t>) {
            constexpr auto soff = conversion_detail::calculate_subtype_offset<typename LiteralDatatype_t::numeric_impl_type, conversion_table_t>();
            static_assert(soff.has_value(), "a stub-numeric type must define linearly reachable supertype that is impl-numeric as numeric_impl_type");

            return NumericOps{NumericOpsStub{.start_s_off = *soff}};
        } else {
            return std::nullopt;
        }
    }();

    auto const ebv_fptr = []() -> ebv_fptr_t {
        if constexpr (datatypes::LogicalLiteralDatatype<LiteralDatatype_t>) {
            return [](std::any const &operand) noexcept -> bool {
                auto const &operand_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(operand);
                return LiteralDatatype_t::effective_boolean_value(operand_val);
            };
        } else {
            return nullptr;
        }
    }();

    auto const compare_fptr = []() -> compare_fptr_t {
        if constexpr (datatypes::ComparableLiteralDatatype<LiteralDatatype_t>) {
            return [](std::any const &lhs, std::any const &rhs) noexcept -> std::partial_ordering {
                auto const &lhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(lhs);
                auto const &rhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(rhs);

                return LiteralDatatype_t::compare(lhs_val, rhs_val);
            };
        } else {
            return nullptr;
        }
    }();

    auto const inlining_ops = []() -> std::optional<InliningOps> {
        if constexpr (datatypes::IsInlineable<LiteralDatatype_t>) {
            return make_inlining_ops<LiteralDatatype_t>();
        } else {
            return std::nullopt;
        }
    }();

    DatatypeEntry entry{
            .datatype_iri = std::string{LiteralDatatype_t::identifier},
            .factory_fptr = [](std::string_view string_repr) -> std::any {
                return LiteralDatatype_t::from_string(string_repr);
            },
            .to_canonical_string_fptr = [](std::any const &value) noexcept -> std::string {
                return LiteralDatatype_t::to_canonical_string(std::any_cast<typename LiteralDatatype_t::cpp_type>(value));
            },
            .to_simplified_string_fptr = [](std::any const &value) noexcept -> std::string {
                return LiteralDatatype_t::to_simplified_string(std::any_cast<typename LiteralDatatype_t::cpp_type>(value));
            },
            .ebv_fptr = ebv_fptr,
            .numeric_ops = num_ops,
            .inlining_ops = inlining_ops,
            .compare_fptr = compare_fptr,
            .conversion_table = RuntimeConversionTable::from_concrete<conversion_table_t>()};

    if constexpr (FixedIdLiteralDatatype<LiteralDatatype_t>) {
        DatatypeRegistry::add_fixed(std::move(entry), LiteralDatatype_t::fixed_id);
    } else {
        DatatypeRegistry::add(std::move(entry));
    }
}

namespace detail {

template<LiteralDatatypeOrUndefined OpRes, LiteralDatatype Fallback>
struct SelectOpRes {
    using type = OpRes;
};

template<LiteralDatatype Fallback>
struct SelectOpRes<std::false_type, Fallback> {
    using type = Fallback;
};

template<LiteralDatatypeOrUndefined OpRes, LiteralDatatype Fallback>
struct SelectOpResIRI {
    inline static DatatypeID select() noexcept {
        using op_res = typename detail::SelectOpRes<OpRes, Fallback>::type;

        if constexpr (FixedIdLiteralDatatype<op_res>) {
            return DatatypeID{op_res::fixed_id};
        } else {
            return DatatypeID{std::string{op_res::identifier}};
        }
    }
};

template<typename T>
[[nodiscard]] nonstd::expected<std::any, DynamicError> map_expected(nonstd::expected<T, DynamicError> const &e) noexcept {
    if (e.has_value()) {
        return *e;
    } else {
        return nonstd::make_unexpected(e.error());
    }
}
}  // namespace detail

template<datatypes::NumericImplLiteralDatatype LiteralDatatype_t>
DatatypeRegistry::NumericOpsImpl DatatypeRegistry::make_numeric_ops_impl() noexcept {
    return NumericOpsImpl{
            // 0
            .zero_value_fptr = []() noexcept -> std::any {
                return LiteralDatatype_t::zero_value();
            },
            // 1
            .one_value_fptr = []() noexcept -> std::any {
                return LiteralDatatype_t::one_value();
            },
            // a + b
            .add_fptr = [](std::any const &lhs, std::any const &rhs) noexcept -> NumericOpResult {
                auto const &lhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(lhs);
                auto const &rhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(rhs);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::add_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::add(lhs_val, rhs_val))};
            },
            // a - b
            .sub_fptr = [](std::any const &lhs, std::any const &rhs) noexcept -> NumericOpResult {
                auto const &lhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(lhs);
                auto const &rhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(rhs);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::sub_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::sub(lhs_val, rhs_val))};
            },
            // a * b
            .mul_fptr = [](std::any const &lhs, std::any const &rhs) noexcept -> NumericOpResult {
                auto const &lhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(lhs);
                auto const &rhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(rhs);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::mul_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::mul(lhs_val, rhs_val))};
            },
            // a / b
            .div_fptr = [](std::any const &lhs, std::any const &rhs) noexcept -> NumericOpResult {
                auto const &lhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(lhs);
                auto const &rhs_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(rhs);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::div_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::div(lhs_val, rhs_val))};
            },
            // +a
            .pos_fptr = [](std::any const &operand) noexcept -> NumericOpResult {
                auto const &operand_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(operand);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::pos_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::pos(operand_val))};
            },
            // -a
            .neg_fptr = [](std::any const &operand) noexcept -> NumericOpResult {
                auto const &operand_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(operand);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::neg_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::neg(operand_val))};
            },
            // abs(a)
            .abs_fptr = [](std::any const &operand) noexcept -> NumericOpResult {
                auto const &operand_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(operand);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::abs_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::abs(operand_val))};
            },
            // round(a)
            .round_fptr = [](std::any const &operand) noexcept -> NumericOpResult {
                auto const &operand_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(operand);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::round_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::round(operand_val))};
            },
            // floor(a)
            .floor_fptr = [](std::any const &operand) noexcept -> NumericOpResult {
                auto const &operand_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(operand);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::floor_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::floor(operand_val))};
            },
            // ceil(a)
            .ceil_fptr = [](std::any const &operand) noexcept -> NumericOpResult {
                auto const &operand_val = std::any_cast<typename LiteralDatatype_t::cpp_type>(operand);

                return NumericOpResult{
                        .result_type_id = detail::SelectOpResIRI<typename LiteralDatatype_t::ceil_result, LiteralDatatype_t>::select(),
                        .result_value = detail::map_expected(LiteralDatatype_t::ceil(operand_val))};
            }};
}

template<datatypes::InlineableLiteralDatatype LiteralDatatype_t>
DatatypeRegistry::InliningOps DatatypeRegistry::make_inlining_ops() noexcept {
    return InliningOps {
        .try_into_inlined_fptr = [](std::any const &value) noexcept -> std::optional<uint64_t> {
            auto const &val = std::any_cast<typename LiteralDatatype_t::cpp_type>(value);
            return LiteralDatatype_t::try_into_inlined(val);
        },
        .from_inlined_fptr = [](uint64_t inlined_value) noexcept -> std::any {
            return LiteralDatatype_t::from_inlined(inlined_value);
        }};
}

}  // namespace rdf4cpp::rdf::datatypes::registry
#endif  //RDF4CPP_DATATYPEREGISTRY_HPP
