#ifndef RDF4CPP_LITERALDATATYPE_HPP
#define RDF4CPP_LITERALDATATYPE_HPP

#include <optional>

#include <rdf4cpp/rdf/datatypes/registry/DatatypeID.hpp>
#include <rdf4cpp/rdf/datatypes/registry/util/ConstexprString.hpp>
#include <rdf4cpp/rdf/storage/node/identifier/LiteralType.hpp>

#include <nonstd/expected.hpp>

namespace rdf4cpp::rdf::datatypes {

template<typename LiteralDatatypeImpl>
concept LiteralDatatype = requires(LiteralDatatypeImpl, std::string_view sv, typename LiteralDatatypeImpl::cpp_type const &cpp_value) {
                              typename LiteralDatatypeImpl::cpp_type;
                              { LiteralDatatypeImpl::identifier } -> std::convertible_to<std::string_view>;
                              { LiteralDatatypeImpl::datatype_id } -> std::convertible_to<registry::DatatypeIDView>;
                              { LiteralDatatypeImpl::from_string(sv) } -> std::convertible_to<typename LiteralDatatypeImpl::cpp_type>;
                              { LiteralDatatypeImpl::to_canonical_string(cpp_value) } -> std::convertible_to<std::string>;
                              { LiteralDatatypeImpl::to_simplified_string(cpp_value) } -> std::convertible_to<std::string>;
                          };

template<typename LiteralDatatypeImpl>
concept LiteralDatatypeOrUndefined = LiteralDatatype<LiteralDatatypeImpl> || std::same_as<LiteralDatatypeImpl, std::false_type>;

/**
 * Errors that can be returned from datatype operations
 */
enum struct DynamicError {
    DivideByZero = 0,    // https://www.w3.org/TR/xpath-functions/#ERRFOAR0001
    OverOrUnderFlow,     // https://www.w3.org/TR/xpath-functions/#ERRFOAR0002
    InvalidValueForCast, // https://www.w3.org/TR/xpath-functions/#ERRFORG0001
    Unsupported,         // generic error that an operation is not supported right now
};


/**
 * A type that is not explicitly a LiteralDatatype but fulfills the requirements for being impl-numeric (see NumericStubLiteralDatatype)
 */
template<typename LiteralDatatypeImpl>
concept NumericImpl = requires(typename LiteralDatatypeImpl::cpp_type const &lhs, typename LiteralDatatypeImpl::cpp_type const &rhs) {
                          requires LiteralDatatypeOrUndefined<typename LiteralDatatypeImpl::add_result>;
                          requires LiteralDatatypeOrUndefined<typename LiteralDatatypeImpl::sub_result>;
                          requires LiteralDatatypeOrUndefined<typename LiteralDatatypeImpl::mul_result>;
                          requires LiteralDatatypeOrUndefined<typename LiteralDatatypeImpl::div_result>;
                          requires LiteralDatatypeOrUndefined<typename LiteralDatatypeImpl::pos_result>;
                          requires LiteralDatatypeOrUndefined<typename LiteralDatatypeImpl::neg_result>;
                          typename LiteralDatatypeImpl::add_result_cpp_type;
                          typename LiteralDatatypeImpl::sub_result_cpp_type;
                          typename LiteralDatatypeImpl::mul_result_cpp_type;
                          typename LiteralDatatypeImpl::div_result_cpp_type;
                          typename LiteralDatatypeImpl::pos_result_cpp_type;
                          typename LiteralDatatypeImpl::neg_result_cpp_type;

                          { LiteralDatatypeImpl::zero_value() } -> std::convertible_to<typename LiteralDatatypeImpl::cpp_type>;
                          { LiteralDatatypeImpl::one_value() } -> std::convertible_to<typename LiteralDatatypeImpl::cpp_type>;
                          { LiteralDatatypeImpl::add(lhs, rhs) } -> std::convertible_to<nonstd::expected<typename LiteralDatatypeImpl::add_result_cpp_type, DynamicError>>;
                          { LiteralDatatypeImpl::sub(lhs, rhs) } -> std::convertible_to<nonstd::expected<typename LiteralDatatypeImpl::sub_result_cpp_type, DynamicError>>;
                          { LiteralDatatypeImpl::mul(lhs, rhs) } -> std::convertible_to<nonstd::expected<typename LiteralDatatypeImpl::mul_result_cpp_type, DynamicError>>;
                          { LiteralDatatypeImpl::div(lhs, rhs) } -> std::convertible_to<nonstd::expected<typename LiteralDatatypeImpl::div_result_cpp_type, DynamicError>>;
                          { LiteralDatatypeImpl::pos(lhs) } -> std::convertible_to<nonstd::expected<typename LiteralDatatypeImpl::pos_result_cpp_type, DynamicError>>;
                          { LiteralDatatypeImpl::neg(lhs) } -> std::convertible_to<nonstd::expected<typename LiteralDatatypeImpl::neg_result_cpp_type, DynamicError>>;
                      };

/**
 * A LiteralDatatype that has an implementation for all numeric ops.
 * This property is also referred to as being "impl-numeric".
 */
template<typename LiteralDatatypeImpl>
concept NumericImplLiteralDatatype = LiteralDatatype<LiteralDatatypeImpl> && NumericImpl<LiteralDatatypeImpl>;

/**
 * A type that is not explicitly a LiteralDatatype but fulfills the requirements for being stub-numeric (see NumericStubLiteralDatatype)
 */
template<typename LiteralDatatypeImpl>
concept NumericStub = requires {
                          requires NumericImplLiteralDatatype<typename LiteralDatatypeImpl::numeric_impl_type>;
                      };

/**
 * A LiteralDatatype that is numeric but does not itself have an impl for any numeric op
 * it instead names another NumericImplLiteralDatatype that the operations should be delegated to.
 * This property is also referred to as being "stub-numeric".
 */
template<typename LiteralDatatypeImpl>
concept NumericStubLiteralDatatype = LiteralDatatype<LiteralDatatypeImpl> && NumericStub<LiteralDatatypeImpl>;

/**
 * A LiteralDatatype that is either stub-numeric (see NumericStubLiteralDatatype) or impl-numeric (see NumericImplLiteralDatatype).
 */
template<typename LiteralDatatypeImpl>
concept NumericLiteralDatatype = LiteralDatatype<LiteralDatatypeImpl> && (NumericImpl<LiteralDatatypeImpl> || NumericStub<LiteralDatatypeImpl>);

template<typename LiteralDatatypeImpl>
concept LogicalLiteralDatatype = LiteralDatatype<LiteralDatatypeImpl> && requires(typename LiteralDatatypeImpl::cpp_type const &value) {
                                                                             { LiteralDatatypeImpl::effective_boolean_value(value) } -> std::convertible_to<bool>;
                                                                         };

template<typename LiteralDatatypeImpl>
concept ComparableLiteralDatatype = LiteralDatatype<LiteralDatatypeImpl> && requires (typename LiteralDatatypeImpl::cpp_type const &lhs, typename LiteralDatatypeImpl::cpp_type const &rhs) {
                                                                                { LiteralDatatypeImpl::compare(lhs, rhs) } -> std::convertible_to<std::partial_ordering>;
                                                                            };

template<typename LiteralDatatypeImpl>
concept PromotableLiteralDatatype = LiteralDatatype<LiteralDatatypeImpl> && requires(typename LiteralDatatypeImpl::cpp_type const &value, typename LiteralDatatypeImpl::template promoted_cpp_type<0> const &promoted_value) {
                                                                                requires LiteralDatatype<typename LiteralDatatypeImpl::template promoted<0>>;
                                                                                typename LiteralDatatypeImpl::template promoted_cpp_type<0>;
                                                                                { LiteralDatatypeImpl::promotion_rank } -> std::convertible_to<size_t>;
                                                                                { LiteralDatatypeImpl::max_promotion_specialization_ix } -> std::convertible_to<size_t>;
                                                                                { LiteralDatatypeImpl::template promote<0>(value) } -> std::convertible_to<typename LiteralDatatypeImpl::template promoted_cpp_type<0>>;
                                                                                { LiteralDatatypeImpl::template demote<0>(promoted_value) } -> std::convertible_to<nonstd::expected<typename LiteralDatatypeImpl::cpp_type, DynamicError>>;
                                                                            };

template<typename LiteralDatatypeImpl>
concept SubtypedLiteralDatatype = LiteralDatatype<LiteralDatatypeImpl> && requires(typename LiteralDatatypeImpl::cpp_type const &value, typename LiteralDatatypeImpl::template super_cpp_type<0> const &super_value) {
                                                                              requires LiteralDatatype<typename LiteralDatatypeImpl::template supertype<0>>;
                                                                              typename LiteralDatatypeImpl::template super_cpp_type<0>;
                                                                              { LiteralDatatypeImpl::subtype_rank } -> std::convertible_to<size_t>;
                                                                              { LiteralDatatypeImpl::max_supertype_specialization_ix } -> std::convertible_to<size_t>;
                                                                              { LiteralDatatypeImpl::template into_supertype<0>(value) } -> std::convertible_to<typename LiteralDatatypeImpl::template super_cpp_type<0>>;
                                                                              { LiteralDatatypeImpl::template from_supertype<0>(super_value) } -> std::convertible_to<nonstd::expected<typename LiteralDatatypeImpl::cpp_type, DynamicError>>;
                                                                          };

/**
 * only exists to resolve a circular lookup problem in LiteralDatatypeImpl::datatype_id
 */
template<typename LiteralDatatypeImpl>
concept HasFixedId = requires {
                         { LiteralDatatypeImpl::fixed_id } -> std::convertible_to<storage::node::identifier::LiteralType>;
                     };

template<typename LiteralDatatypeImpl>
concept FixedIdLiteralDatatype = LiteralDatatype<LiteralDatatypeImpl> && HasFixedId<LiteralDatatypeImpl>;

template<typename LiteralDatatypeImpl>
concept IsInlineable = requires (typename LiteralDatatypeImpl::cpp_type const &value, uint64_t inlined_value) {
                           { LiteralDatatypeImpl::is_inlineable } -> std::convertible_to<std::true_type>;
                           { LiteralDatatypeImpl::try_into_inlined(value) } -> std::convertible_to<std::optional<uint64_t>>;
                           { LiteralDatatypeImpl::from_inlined(inlined_value) } -> std::convertible_to<typename LiteralDatatypeImpl::cpp_type>;
                       };

template<typename LiteralDatatypeImpl>
concept InlineableLiteralDatatype = LiteralDatatype<LiteralDatatypeImpl> && IsInlineable<LiteralDatatypeImpl>;

}  // namespace rdf4cpp::rdf::datatypes
#endif  //RDF4CPP_LITERALDATATYPE_HPP
