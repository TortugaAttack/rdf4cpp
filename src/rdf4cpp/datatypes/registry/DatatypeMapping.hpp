#ifndef RDF4CPP_DATATYPEMAPPING_HPP
#define RDF4CPP_DATATYPEMAPPING_HPP

#include <rdf4cpp/datatypes/registry/util/ConstexprString.hpp>

#include <type_traits>

namespace rdf4cpp::datatypes::registry {

/**
 * Mapping type_iri -> c++ datatype
 */
template<util::ConstexprString type_iri>
struct DatatypeMapping {
    using cpp_datatype = std::false_type;
};

/**
 * Mapping type_iri -> promoted version of itself
 * @note promoted must be LiteralDatatype
 */
template<util::ConstexprString type_iri>
struct DatatypePromotionMapping {
    using promoted = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypePromotionSpecializationOverride {
    static constexpr size_t max_specialization_ix = 0;
};

/**
 * Mapping type_iri -> supertype of itself
 * @note supertype must be LiteralDatatype
 */
template<util::ConstexprString type_iri>
struct DatatypeSupertypeMapping {
    using supertype = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypeSupertypeSpecializationOverride {
    static constexpr size_t max_specialization_ix = 0;
};

/**
 * Mapping type_iri of numeric stub -> a supertype that is the numeric impl that should be used for this type
 * @note numeric_impl_type must be LiteralDatatype and NumericImpl
 */
template<util::ConstexprString type_iri>
struct DatatypeNumericStubMapping {
    using numeric_impl_type = std::false_type;
};

/**
 * Mapping type_iri -> result of addition operation
 * @note op_result must be LiteralDatatypeOrUndefined
 */
template<util::ConstexprString type_iri>
struct DatatypeAddResultMapping {
    using op_result = std::false_type;
};

/**
 * Mapping type_iri -> result of subtraction operation
 * @note op_result must be LiteralDatatypeOrUndefined
 */
template<util::ConstexprString type_iri>
struct DatatypeSubResultMapping {
    using op_result = std::false_type;
};

/**
 * Mapping type_iri -> result of multiplication operation
 * @note op_result must be LiteralDatatypeOrUndefined
 */
template<util::ConstexprString type_iri>
struct DatatypeMulResultMapping {
    using op_result = std::false_type;
};

/**
 * Mapping type_iri -> result of division operation
 * @note op_result must be LiteralDatatypeOrUndefined
 */
template<util::ConstexprString type_iri>
struct DatatypeDivResultMapping {
    using op_result = std::false_type;
};

/**
 * Mapping type_iri -> result of unary plus operation
 * @note op_result must be LiteralDatatypeOrUndefined
 */
template<util::ConstexprString type_iri>
struct DatatypePosResultMapping {
    using op_result = std::false_type;
};

/**
 * Mapping type_iri -> result of unary minus operation
 * @note op_result must be LiteralDatatypeOrUndefined
 */
template<util::ConstexprString type_iri>
struct DatatypeNegResultMapping {
    using op_result = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypeAbsResultMapping {
    using op_result = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypeRoundResultMapping {
    using op_result = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypeFloorResultMapping {
    using op_result = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypeCeilResultMapping {
    using op_result = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypeTimepointDurationOperandMapping {
    using duration_type = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypeTimepointSubResultMapping {
    using op_result = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypeDurationScalarMapping {
    using scalar_type = std::false_type;
};

template<util::ConstexprString type_iri>
struct DatatypeDurationDivResultMapping {
    using op_result = std::false_type;
};

namespace detail_rank {

template<util::ConstexprString type_iri, size_t N>
struct NthPromotion {
    using prev = typename NthPromotion<type_iri, N - 1>::promoted;
    using promoted = typename DatatypePromotionMapping<prev::identifier>::promoted;
};

template<util::ConstexprString type_iri>
struct NthPromotion<type_iri, 0> {
    using promoted = typename DatatypePromotionMapping<type_iri>::promoted;
};

template<util::ConstexprString type_iri, size_t N>
struct NthSupertype {
    using prev = typename NthSupertype<type_iri, N - 1>::supertype;
    using supertype = typename DatatypeSupertypeMapping<prev::identifier>::promoted;
};

template<util::ConstexprString type_iri>
struct NthSupertype<type_iri, 0> {
    using supertype = typename DatatypeSupertypeMapping<type_iri>::supertype;
};

/**
 * The promotion rank of a type (the number of times a type can be promoted)
 */
template<util::ConstexprString type_iri, typename enable = void>
struct DatatypePromotionRank {
    static constexpr size_t value = 0;
};

template<util::ConstexprString type_iri>
struct DatatypePromotionRank<type_iri, std::enable_if_t<!std::is_same_v<typename DatatypePromotionMapping<type_iri>::promoted, std::false_type>>> {
    static constexpr size_t value = 1 + DatatypePromotionRank<DatatypePromotionMapping<type_iri>::promoted::identifier>::value;
};

/**
 * The subtype rank of a type (the number of supertypes that are above in the hierarchy of a type)
 */
template<util::ConstexprString type_iri, typename enable = void>
struct DatatypeSubtypeRank {
    static constexpr size_t value = 0;
};

template<util::ConstexprString type_iri>
struct DatatypeSubtypeRank<type_iri, std::enable_if_t<!std::is_same_v<typename DatatypeSupertypeMapping<type_iri>::supertype, std::false_type>>> {
    static constexpr size_t value = 1 + DatatypeSubtypeRank<DatatypeSupertypeMapping<type_iri>::supertype::identifier>::value;
};

}  // namespace detail_rank
}  // namespace rdf4cpp::datatypes::registry
#endif  //RDF4CPP_DATATYPEMAPPING_HPP
