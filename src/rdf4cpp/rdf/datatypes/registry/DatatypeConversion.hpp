#ifndef RDF4CPP_DATATYPECONVERSION_HPP
#define RDF4CPP_DATATYPECONVERSION_HPP

#include <any>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include <nonstd/expected.hpp>

#include <rdf4cpp/rdf/datatypes/registry/util/TypeList.hpp>
#include <rdf4cpp/rdf/datatypes/LiteralDatatype.hpp>
#include <rdf4cpp/rdf/datatypes/registry/DatatypeConversionTyping.hpp>

namespace rdf4cpp::rdf::datatypes::registry {

namespace conversion_detail {

/**
 * Generate a linear conversion table
 * consisting only of one type of conversion,
 * so either only promotion or only subtype substitution.
 *
 * This is a recursive function and the first call is expected to have BaseType == Type.
 *
 * for more details see: make_promotion_layer or make_subtype_layer
 *
 * @tparam BaseType the type for which to generate the linear conversions
 * @tparam Type the current type getting processed in the linear hierarchy
 * @tparam Conversion a common interface to carry out a conversion (see adaptor namespace)
 * @tparam HasConversion check if a LiteralDatatype has a next conversion, e.g. if it is promotable
 * @tparam RankRule a boolean predicate to check if the defined conversion is valid
 * @param table_acc accumulator for ConversionEntries that will eventually be returned
 * @return a std::tuple<ConversionEntry, ...> containing all linear conversions reachable from BaseType
 */
template<LiteralDatatype BaseType,
         LiteralDatatype Type,
         template<LiteralDatatype> typename Conversion,
         template<LiteralDatatype> typename HasConversion,
         template<LiteralDatatype, LiteralDatatype> typename RankRule,
         ConversionLayer LayerAcc>
consteval ConversionLayer auto make_conversion_layer_impl() {
    if constexpr (!HasConversion<Type>::value) {
        return LayerAcc{};
    } else {
        using next = Conversion<Type>;

        // defining a hierarchy which does not follow the rank rules (see adaptors::PromoteRankRule, adaptors::SubtypeRankRule)
        // would result in not being able to find all conversions between types (because of assumptions used in the search algorithm)
        // and thus yield unexpected behaviour when trying to convert
        // e.g. with
        //        A --> Z               Legend:
        //        |     |                   --> : promotable to
        //        B --> Y --> X (!)          |  : subtype of
        //
        // The algorithm would not be able to find B --> X or Y --> X,
        // because X has subtype rank 0 while Y and B have subtype rank 1.
        // This is problematic when trying to jump to the common subtype level for both arguments
        // because while in the graph they are visually at the correct level, their subtype ranks do not reflect that.
        // The same problem exists when a supertype (or promoted type) has a subtype (promotion) rank that is not exactly one less than the origin type's rank.
        static_assert(RankRule<Type, typename next::converted>::value,
                      "detected invalid hierarchy, would not be able to discover all conversions");

        // conversion must preserve numericity, so:
        // NumericLiteralDatatype<Type> <-> NumericLiteralDatatype<typename next::converted>
        // This is because the numeric operations assume that if any of the operands is not numeric
        // the common type and the result type will also not be numeric to allow for an early return
        // without doing the actual conversion.
        static_assert((NumericLiteralDatatype<Type> && NumericLiteralDatatype<typename next::converted>) || (!NumericLiteralDatatype<Type> && !NumericLiteralDatatype<typename next::converted>),
                      "conversion must preserve numericity");

        // conversion cannot downgrade impl-numericity class back to stub-numericity, so:
        // NumericImplLiteralDatatype<Type> -> NumericImplLiteralDatatype<typename next::converted>
        // This is required so that skipping to a numeric-impl in the hierarchy will guarantee
        // that we can only find numeric impls from now on, which ensures the search algorithm can be single pass.
        static_assert(!NumericImplLiteralDatatype<Type> || NumericImplLiteralDatatype<typename next::converted>,
                      "conversion must preserve impl-numericity");

        // conversion must preserve comparability, so:
        // ComparableLiteralDatatype<Type> <-> ComparableLiteralDatatype<typename next::converted>
        // The reasoning is the same as in the static_assert for numericity.
        static_assert((ComparableLiteralDatatype<Type> && ComparableLiteralDatatype<typename next::converted>) || (!ComparableLiteralDatatype<Type> && !ComparableLiteralDatatype<typename next::converted>),
                      "conversion must preserve comparability");

        if constexpr (BaseType::identifier == Type::identifier) {
            struct FirstConversion {
                using source_type = Type;
                using target_type = typename next::converted;

                inline static typename target_type::cpp_type convert(typename source_type::cpp_type const &value) noexcept {
                    return next::convert(value);
                }

                inline static nonstd::expected<typename source_type::cpp_type, DynamicError> inverse_convert(typename target_type::cpp_type const &value) noexcept {
                    return next::inverse_convert(value);
                }
            };

            using next_table = typename LayerAcc::template append<FirstConversion>;
            return make_conversion_layer_impl<BaseType, typename next::converted, Conversion, HasConversion, RankRule, next_table>();
        } else {
            auto const table_size = LayerAcc::length;
            using prev_promotion_t = typename LayerAcc::template select<table_size - 1>;

            struct NextConversion {
                using source_type = typename prev_promotion_t::source_type;
                using target_type = typename next::converted;

                inline static typename target_type::cpp_type convert(typename source_type::cpp_type const &value) noexcept {
                    return next::convert(prev_promotion_t::convert(value));
                }

                inline static nonstd::expected<typename source_type::cpp_type, DynamicError> inverse_convert(typename target_type::cpp_type const &value) noexcept {
                    auto const converted = next::inverse_convert(value);
                    if (!converted.has_value()) {
                        return nonstd::make_unexpected(converted.error());
                    }

                    return prev_promotion_t::inverse_convert(*converted);
                }
            };

            using next_table = typename LayerAcc::template append<NextConversion>;
            return make_conversion_layer_impl<BaseType, typename next::converted, Conversion, HasConversion, RankRule, next_table>();
        }
    }
}

namespace adaptor {

/**
 * Adapt Promotion to a common "conversion" interface.
 */
template<LiteralDatatype LiteralDatatypeImpl>
struct PromoteConversion;

template<PromotableLiteralDatatype LiteralDatatypeImpl>
struct PromoteConversion<LiteralDatatypeImpl> {
    using cpp_type = typename LiteralDatatypeImpl::cpp_type;

    using converted = typename LiteralDatatypeImpl::promoted;
    using converted_cpp_type = typename LiteralDatatypeImpl::promoted_cpp_type;

    inline static converted_cpp_type convert(cpp_type const &value) noexcept {
        return LiteralDatatypeImpl::promote(value);
    }

    inline static nonstd::expected<cpp_type, DynamicError> inverse_convert(converted_cpp_type const &value) noexcept {
        return LiteralDatatypeImpl::demote(value);
    }
};

/**
 * Adapt subtype substitution to a common "conversion" interface.
 */
template<LiteralDatatype LiteralDatatypeImpl>
struct SupertypeConversion;

template<SubtypedLiteralDatatype LiteralDatatypeImpl>
struct SupertypeConversion<LiteralDatatypeImpl> {
    using cpp_type = typename LiteralDatatypeImpl::cpp_type;

    using converted = typename LiteralDatatypeImpl::supertype;
    using converted_cpp_type = typename LiteralDatatypeImpl::super_cpp_type;

    inline static converted_cpp_type convert(cpp_type const &value) noexcept {
        return LiteralDatatypeImpl::into_supertype(value);
    }

    inline static nonstd::expected<cpp_type, DynamicError> inverse_convert(converted_cpp_type const &value) noexcept {
        return LiteralDatatypeImpl::from_supertype(value);
    }
};

template<LiteralDatatype Type>
struct PromoteConcept : std::bool_constant<PromotableLiteralDatatype<Type>> {};

template<LiteralDatatype Type>
struct SubtypeConcept : std::bool_constant<SubtypedLiteralDatatype<Type>> {};

/**
 * - A promotion must preserve subtype rank
 * - A promotion must lower the promotion rank by exactly 1
 */
template<LiteralDatatype Type, LiteralDatatype Promoted>
struct PromoteRankRule : std::bool_constant<detail_rank::DatatypeSubtypeRank<Type::identifier>::value == detail_rank::DatatypeSubtypeRank<Promoted::identifier>::value && detail_rank::DatatypePromotionRank<Promoted::identifier>::value == detail_rank::DatatypePromotionRank<Type::identifier>::value - 1> {};

/**
 * - A subtype substitution must lower the subtype rank by exactly 1
 */
template<LiteralDatatype Type, LiteralDatatype Super>
struct SubtypeRankRule : std::bool_constant<detail_rank::DatatypeSubtypeRank<Super::identifier>::value == detail_rank::DatatypeSubtypeRank<Type::identifier>::value - 1> {};

}  // namespace adaptor

/**
 * Generate the linear conversion table consisting only of type promotion conversions with a LiteralDatatype
 * as a source
 *
 * Example: Given the hierarchy
 *         A    --> Z
 *      |    |      |
 *     B --> B2 --> Y
 *     |
 *     C
 *
 * make_promotion_layer<B>()  would generate a ConversionLayer containing: (B -> B2, B -> Y)
 * make_promotion_layer<A>()  would generate a ConversionLayer containing: (A -> Z)
 * make_promotion_layer<B2>() would generate a ConversionLayer containing: (B2 -> Y)
 * make_promotion_layer<Z>()  would generate a ConversionLayer containing nothing
 *
 * @tparam Type the source datatype to generate the promotion table for
 * @return compile-time linear promotion table
 */
template<LiteralDatatype Type>
consteval ConversionLayer auto make_promotion_layer() {
    if constexpr (PromotableLiteralDatatype<Type>) {
        return conversion_detail::make_conversion_layer_impl<Type,
                                                             Type,
                                                             conversion_detail::adaptor::PromoteConversion,
                                                             conversion_detail::adaptor::PromoteConcept,
                                                             conversion_detail::adaptor::PromoteRankRule,
                                                             mz::type_list<>>();
    } else {
        return mz::type_list{};
    }
}

/**
 * Generate the linear conversion table consisting only of subtype substitution conversions with a LiteralDatatype
 * as a source.
 *
 * Example: Given the hierarchy
 *         A    --> Z
 *      |    |      |
 *     B --> B2 --> Y
 *     |
 *     C
 *
 * make_subtype_layer<C>()  would generate a ConversionLayer containing: (C -> B, C -> A)
 * make_subtype_layer<B2>() would generate a ConversionLayer containing: (B2 -> A)
 * make_subtype_layer<Y>()  would generate a ConversionLayer containing (Y -> Z)
 * make_subtype_layer<A>()  would generate a ConversionLayer containing nothing
 *
 * @tparam Type the source datatype to generate the subtype substitution table for
 * @return compile-time linear subtype substitution table
 */
template<LiteralDatatype Type>
consteval ConversionLayer auto make_subtype_layer() {
    if constexpr (SubtypedLiteralDatatype<Type>) {
        return conversion_detail::make_conversion_layer_impl<Type,
                                                             Type,
                                                             conversion_detail::adaptor::SupertypeConversion,
                                                             conversion_detail::adaptor::SubtypeConcept,
                                                             conversion_detail::adaptor::SubtypeRankRule,
                                                             mz::type_list<>>();
    } else {
        return mz::type_list{};
    }
}

/**
 * Generate the combined table that contains conversions
 * for every type that is reachable from Type by a combination
 * of subtype substitution and type promotion. (For a definition of reachable see make_conversion_table_for.)
 *
 * The table gets generated by combining linear layers generated by make_subtype_layer and make_promotion_layer.
 *      - The first layer is just the promotion layer for `Type` with an identity conversion prepended at the beginning.
 *      - Every other layer gets generated by: Foreach `target` in the linear subtype layer of `Type` generate the linear promotion layer
 *          and compose the layer (element-wise) with the subtype conversion that was required to reach the `target`. Then the conversion
 *          from `Type` to `target` is prepended at the front.
 *
 * Example: Given the hierarchy:
 *         A   --> Z
 *      |    |
 *     B --> C
 *
 * So resulting ConversionTable for B is generated like this:
 *      tuple{
 *           tuple_cat( tuple{B -> B}, make_promotion_layer<B>() ),
 *           tuple_cat( tuple{B -> A}, tuple_map( make_promotion_layer<A>(), `compose with B -> A` ) )
 *      }
 *      ==
 *      tuple{
 *           tuple{B -> B, B -> C},
 *           tuple{B -> A, tuple_map(tuple{A -> Z}, `compose with B -> A` )}
 *      }
 *      ==
 *      tuple{
 *           tuple{B -> B, B -> C},
 *           tuple{B -> A, B -> Z}
 *      }
 *
 * @tparam Type the datatype to generate the combined table for
 * @return the combined table
 */
template<LiteralDatatype Type>
consteval ConversionTable auto make_conversion_table() {
    struct IdConversion {
        using source_type = Type;
        using target_type = Type;

        inline static typename target_type::cpp_type convert(typename source_type::cpp_type const &value) noexcept {
            return value;
        }

        inline static nonstd::expected<typename source_type::cpp_type, DynamicError> inverse_convert(typename target_type::cpp_type const &value) noexcept {
            return value;
        }
    };

    using level_0_table = typename decltype(make_promotion_layer<Type>())::template prepend<IdConversion>;

    if constexpr (!SubtypedLiteralDatatype<Type>) {
        return mz::type_list<level_0_table>{};
    } else {
        using s_table = decltype(make_subtype_layer<Type>());

        // generate the linear promotion table for each supertype
        auto const other_level_tables = util::type_list_map<s_table>([]<ConversionEntry ToSuper>() {
            using level_p_table = decltype(make_promotion_layer<typename ToSuper::target_type>());

            // compose each promotion for the supertype with the function to convert to the supertype
            // to get direct conversions from Type to promoted supertype
            auto const to_promoted_supers = util::type_list_map<level_p_table>([]<ConversionEntry PromoteSuper>() {
                struct PromotedSuperConversion {
                    using source_type = typename ToSuper::source_type;
                    using target_type = typename PromoteSuper::target_type;

                    inline static typename target_type::cpp_type convert(typename source_type::cpp_type const &value) noexcept {
                        return PromoteSuper::convert(ToSuper::convert(value));
                    }

                    inline static nonstd::expected<typename source_type::cpp_type, DynamicError> inverse_convert(typename target_type::cpp_type const &value) noexcept {
                        auto const demoted = PromoteSuper::inverse_convert(value);
                        if (!demoted.has_value()) {
                            return nonstd::make_unexpected(demoted.error());
                        }

                        return ToSuper::inverse_convert(*demoted);
                    }
                };

                return PromotedSuperConversion{};
            });

            return typename decltype(to_promoted_supers)::template prepend<ToSuper>{};
        });

        return typename decltype(other_level_tables)::template prepend<level_0_table>{};
    }
}

/**
 * Calculates the subtype offset from a subtype to a supertype, so the
 * number of steps to traverse upwards in the hierarchy until the supertype
 * is reached from the subtype.
 *
 * @tparam Supertype the to be searched supertype
 * @tparam SubtypeTable the conversion table of the subtype
 * @return equivalent to: subtype.subtype_rank - supertype.subtype_rank if sub- and supertype are in the same subtype hierarchy, else nullopt
 */
template<LiteralDatatype Supertype, ConversionTable SubtypeTable>
consteval std::optional<size_t> calculate_subtype_offset() {
    return util::type_list_find_if<SubtypeTable>([]<ConversionLayer Layer>() {
        using target_t = typename Layer::first::target_type;
        return std::is_same_v<target_t, Supertype>;
    });
}

}  // namespace conversion_detail

/**
 * Generate a compile-time conversion table for the given type
 * this will include all conversions that are reachable from T.
 *
 * Reachable in this context means that there is a (*) directed path in the hierarchy
 * from T to the other type.
 *
 * *: only paths of the form
 *      supertype -> ... -> supertype -> promotion -> ... -> promotion
 *      are included, meaning paths that do a promotion first and then go to a supertype
 *      are not included
 *
 * for more details see: conversion_detail::make_conversion_table
 *
 * @tparam T the type for which to generate the table
 * @return the generated table
 */
template<LiteralDatatype T>
consteval ConversionTable auto make_conversion_table_for() {
    return conversion_detail::make_conversion_table<T>();
}

/**
 * Generate a runtime conversion for the given type
 * by first generating the compile-time version with make_conversion_table_for
 * and then constructing a RuntimeConversionTable from it.
 *
 * for more details see: make_conversion_table_for
 *
 * @tparam T the type for which to generate the table
 * @return the generated runtime table
 */
template<LiteralDatatype T>
RuntimeConversionTable make_runtime_conversion_table_for() noexcept {
    using convert_table_t = decltype(make_conversion_table_for<T>());
    return RuntimeConversionTable::from_concrete<convert_table_t>();
}

}  // namespace rdf4cpp::rdf::datatypes::registry

#endif  //RDF4CPP_DATATYPECONVERSION_HPP
