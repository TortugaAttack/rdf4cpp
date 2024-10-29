#ifndef RDF4CPP_VARIABLE_HPP
#define RDF4CPP_VARIABLE_HPP

#include <ostream>
#include <rdf4cpp/Node.hpp>

namespace rdf4cpp::query {

struct Variable : Node {
private:
    [[nodiscard]] static Variable find(std::string_view name, bool anonymous, storage::DynNodeStoragePtr node_storage) noexcept;

public:
    Variable() noexcept;

    explicit Variable(storage::identifier::NodeBackendHandle handle) noexcept;

    explicit Variable(std::string_view name, bool anonymous = false,
                      storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    [[nodiscard]] static Variable make_named(std::string_view name, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);
    [[nodiscard]] static Variable make_anonymous(std::string_view name, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    [[nodiscard]] static Variable make_unchecked(std::string_view name, bool anonymous = false, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    Variable to_node_storage(storage::DynNodeStoragePtr node_storage) const;
    [[nodiscard]] Variable try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept;

    /**
     * searches for a named Variable in the specified node storage and returns it.
     * returns a null Variable, if not found.
     * @param name
     * @param node_storage
     * @return
     */
    [[nodiscard]] static Variable find_named(std::string_view name, storage::DynNodeStoragePtr node_storage = storage::default_node_storage) noexcept;
    /**
     * searches for an anonymous Variable in the specified node storage and returns it.
     * returns a null Variable, if not found.
     * @param name
     * @param node_storage
     * @return
     */
    [[nodiscard]] static Variable find_anonymous(std::string_view name, storage::DynNodeStoragePtr node_storage = storage::default_node_storage) noexcept;

    /**
     * Validates that the given name is a valid Variable name
     * @param var_name name to check
     * @param anonymous if the variable is supposed to be anonymous
     * @throws ParsingError if the variable name is not valid
     */
    static void validate(std::string_view var_name, bool anonymous = false);

    [[nodiscard]] bool is_anonymous() const;

    [[nodiscard]] std::string_view name() const;

    /**
     * See Node::serialize
     */
    bool serialize(writer::BufWriterParts writer) const noexcept;

    [[nodiscard]] explicit operator std::string() const;
    friend std::ostream &operator<<(std::ostream &os, const Variable &variable);

    bool is_blank_node() const = delete;
    bool is_literal() const = delete;
    bool is_variable() const = delete;
    bool is_iri() const = delete;

    friend struct Node;

    // todo unbound()
};

}  // namespace rdf4cpp::query

template<>
struct std::hash<rdf4cpp::query::Variable> {
    inline size_t operator()(rdf4cpp::query::Variable const &v) const noexcept {
        return std::hash<rdf4cpp::Node>()(v);
    }
};

template<>
struct std::formatter<rdf4cpp::query::Variable> : std::formatter<rdf4cpp::Node> {
    auto format(rdf4cpp::query::Variable n, format_context &ctx) const -> decltype(ctx.out());
};

#endif  //RDF4CPP_VARIABLE_HPP
