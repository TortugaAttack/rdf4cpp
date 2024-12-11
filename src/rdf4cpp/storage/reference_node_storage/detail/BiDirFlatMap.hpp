#ifndef RDF4CPP_RDF_REFERENCENODESTORAGE_BIDIRFLATMAP_HPP
#define RDF4CPP_RDF_REFERENCENODESTORAGE_BIDIRFLATMAP_HPP

#include <dice/sparse-map/sparse_set.hpp>
#include <rdf4cpp/storage/reference_node_storage/detail/IndexFreeList.hpp>

namespace rdf4cpp::storage::reference_node_storage::detail {

/**
 * A bidirectional map from Id to Value
 *
 * @tparam Id id type
 * @tparam Value stored value type
 * @tparam View view of Value
 * @tparam Hash hash for View
 * @tparam Equal equality for View and Value
 * @tparam Allocator allocator
 */
template<typename Id, typename Value, typename View, typename Hash = std::hash<View>, typename Equal = std::equal_to<>, typename Allocator = std::allocator<Value>>
struct BiDirFlatMap {
    using id_type = Id;
    using mapped_type = Value;
    using view_type = View;
    using size_type = size_t;
    using allocator_type = Allocator;
    using hasher = Hash;
    using key_equal = Equal;

private:
    using forward_value_type = std::optional<mapped_type>;
    using forward_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<forward_value_type>;
    using forward_type = std::vector<forward_value_type, forward_allocator_type>;

    struct backward_key_type {
        size_t hash; //< hash of the Value being looked up
        id_type id;  //< id into forward_
    };

    struct backward_key_equal {
        using is_transparent = void;

        using forward_const_pointer = typename std::allocator_traits<allocator_type>::template rebind_traits<forward_type>::const_pointer;

        forward_const_pointer forward_ptr; //< pointer to forward_ to be able to check for equality between View and Value
        [[no_unique_address]] Equal eq; //< eq for backend and view

        bool operator()(backward_key_type const &a, backward_key_type const &b) const noexcept {
            return a.id == b.id;
        }

        bool operator()(backward_key_type const &a, view_type const &b) const noexcept {
            auto const &ref = (*forward_ptr)[to_index(a.id)];
            if (!ref.has_value()) {
                return false;
            }

            return eq(*ref, b);
        }

        bool operator()(view_type const &a, backward_key_type const &b) const noexcept {
            auto const &ref = (*forward_ptr)[to_index(b.id)];
            if (!ref.has_value()) {
                return false;
            }

            return eq(a, *ref);
        }
    };

    struct backward_hasher {
        using is_transparent = void;

        [[no_unique_address]] Hash hash; //< hasher for views

        size_t operator()(backward_key_type const &a) const noexcept {
            return a.hash;
        }

        size_t operator()(view_type const &a) const noexcept {
            return hash(a);
        }
    };

    using backward_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<backward_key_type>;
    using backward_type = dice::sparse_map::sparse_set<backward_key_type, backward_hasher, backward_key_equal, backward_allocator_type>;

    using index_free_list_bitmap_type = size_t;
    using index_free_list_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<index_free_list_bitmap_type>;
    using index_free_list_type = IndexFreeList<index_free_list_bitmap_type, index_free_list_allocator>;

    forward_type forward_;   //< forward_[to_index(id)] stores Value for Id id
    backward_type backward_; //< backward_[view] stores Id into forward_
    index_free_list_type freelist_; //< freelist for forward_
    [[no_unique_address]] allocator_type alloc_;

    /**
     * Translates the given id into an index into forward_
     */
    [[nodiscard]] static constexpr size_type to_index(id_type const id) noexcept {
        if constexpr (std::same_as<uint64_t, size_t>) {
            return static_cast<size_type>(id) - 1;
        } else {
            return static_cast<size_type>(static_cast<uint64_t>(id)) - 1;
        }
    }

    /**
     * Translates the given index into forward_ into an id
     */
    [[nodiscard]] static constexpr id_type to_id(size_type const ix) noexcept {
        return static_cast<id_type>(ix + 1);
    }

public:
    explicit BiDirFlatMap(hasher const &hash = hasher{},
                          key_equal const &equal = key_equal{},
                          allocator_type const &alloc = allocator_type{}) noexcept : forward_{alloc},
                                                                                     backward_{0, backward_hasher{hash}, backward_key_equal{&forward_, equal}, alloc},
                                                                                     alloc_{alloc} {
    }

    explicit BiDirFlatMap(allocator_type const &alloc) noexcept : BiDirFlatMap{hasher{}, key_equal{}, alloc} {
    }

    // deleted because type is self-referential
    BiDirFlatMap(BiDirFlatMap const &) = delete;
    BiDirFlatMap(BiDirFlatMap &&) = delete;
    BiDirFlatMap &operator=(BiDirFlatMap const &) = delete;
    BiDirFlatMap &operator=(BiDirFlatMap &&) = delete;

    /**
     * Number of elements stored in this map
     */
    [[nodiscard]] size_type size() const noexcept {
        return forward_.size();
    }

    /**
     * Requests the removal of unused capacity.
     */
    void shrink_to_fit() {
        forward_.shrink_to_fit();
        backward_.rehash(0); // force rehash (zero is special value)
        freelist_.shrink_to_fit();
    }

    /**
     * Look up the value corresponding to the given id
     *
     * @param id id for value to look up
     * @return if a value was found: a view to that value, otherwise nullopt
     */
    [[nodiscard]] std::optional<view_type> lookup_value(id_type const id) const noexcept {
        if (id == id_type{}) [[unlikely]] {
            return std::nullopt;
        }

        auto const ix = to_index(id);
        if (ix >= forward_.size() || !forward_[ix].has_value()) {
            return std::nullopt;
        }

        return static_cast<view_type>(*forward_[ix]);
    }

    /**
     * Look up the id corresponding the given (view to a) value
     *
     * @param view view of value of which to find the id
     * @return id of the value if it was found, otherwise id_type{} if no id was found
     */
    [[nodiscard]] id_type lookup_id(view_type const &view) const noexcept {
        auto it = backward_.find(view);
        if (it == backward_.end()) {
            return id_type{};
        }

        return it->id;
    }

    /**
     * Reserve capacity such that min_id is the first id that
     * triggers an allocation if it is inserted.
     */
    void reserve_until(id_type const min_id) {
        auto const new_size = to_index(min_id);
        if (new_size < forward_.size()) {
            return;
        }

        forward_.resize(new_size);
        backward_.reserve(new_size);
        freelist_.occupy_until(new_size);
    }

    /**
     * Insert a value at the first free id
     *
     * @precondition the value is not yet present in this map
     *
     * @param view view of the value to construct
     * @return id of newly constructed value
     */
    [[nodiscard]] id_type insert_assume_not_present(view_type const &view) {
        assert(lookup_id(view) == Id{});

        auto const assigned_ix = freelist_.occupy_next_available();
        if (assigned_ix >= forward_.size()) {
            assert(assigned_ix == forward_.size());
            forward_.emplace_back();
        }

        forward_[assigned_ix] = std::make_obj_using_allocator<mapped_type>(alloc_, view);

        auto const assigned_id = to_id(assigned_ix);
        auto const h = backward_.hash_function().hash(view);
        backward_.emplace(h, assigned_id);

        return assigned_id;
    }

    /**
     * Insert a value at the given id
     *
     * @precondition the value is not yet present in this map
     * @precondition there is no value present at the requested id
     * @precondition sufficient capacity was allocated using reserve
     *
     * @param view view of the value to construct
     * @param requested_id id to place the value at
     * @return id of newly constructed value
     */
    void insert_assume_not_present_at(view_type const &view, id_type const requested_id) {
        assert(lookup_id(view) == Id{});

        auto const lookup_ix = to_index(requested_id);
        assert(lookup_ix < forward_.size());
        assert(!forward_[lookup_ix].has_value());

        forward_[lookup_ix] = std::make_obj_using_allocator<mapped_type>(alloc_, view);

        auto const h = backward_.hash_function().hash(view);
        backward_.emplace(h, requested_id);
    }

    /**
     * Erase the value at the given id
     *
     * @precondition there is actually a value at the given id
     * @param id id of the value to be erased
     */
    void erase_assume_present(id_type const id) {
        assert(lookup_value(id).has_value());

        auto const ix = to_index(id);
        auto &value = forward_[ix];
        auto const view = static_cast<view_type>(*value);

        backward_.erase(view);
        value.reset();
        freelist_.vacate(ix);
    }

    void clear() noexcept {
        forward_.clear();
        backward_.clear();
        freelist_.clear();
    }
};

} // namespace rdf4cpp::storage::reference_node_storage::detail

#endif // RDF4CPP_RDF_REFERENCENODESTORAGE_BIDIRFLATMAP_HPP
