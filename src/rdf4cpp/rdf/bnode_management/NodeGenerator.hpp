#ifndef RDF4CPP_RDF_UTIL_BLANKNODEMANAGER_HPP
#define RDF4CPP_RDF_UTIL_BLANKNODEMANAGER_HPP

#include <rdf4cpp/rdf/Node.hpp>
#include <rdf4cpp/rdf/bnode_management/IIdGenerator.hpp>
#include <rdf4cpp/rdf/bnode_management/reference_backends/factory/BNodeFactory.hpp>
#include <rdf4cpp/rdf/bnode_management/reference_backends/factory/SkolemIRIFactory.hpp>
#include <rdf4cpp/rdf/bnode_management/reference_backends/generator/IncreasingIdGenerator.hpp>
#include <rdf4cpp/rdf/bnode_management/reference_backends/generator/RandomIdGenerator.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace rdf4cpp::rdf::util {

struct NodeScope;

/**
 * A standalone generator to generate nodes using a given backend.
 * Nodes generated by this generator are not remembered in any way
 * and new generate_node/generate_id calls generate new nodes on every invocation
 * (* this is only guaranteed if the used generator backend guarantees it).
 *
 * Generating nodes is always thread safe.
 */
struct NodeGenerator {
    using NodeStorage = storage::node::NodeStorage;
private:
    friend struct NodeScope;

    std::unique_ptr<IIdGenerator> generator;
    std::unique_ptr<INodeFactory> factory;

    explicit NodeGenerator(std::unique_ptr<IIdGenerator> &&generator, std::unique_ptr<INodeFactory> &&factory) noexcept;

    [[nodiscard]] Node generate_node_impl(NodeScope const *scope, NodeStorage &node_storage);
public:
    NodeGenerator(NodeGenerator &&other) noexcept = default;

    /**
     * Fetch the default generator. It uses the reference_backends/generator/RandomIdGenerator to generate ids.
     * @return a reference to the default generator.
     */
    [[nodiscard]] static NodeGenerator &default_instance();

    /**
     * Creates a generator from a given generator backend.
     * @param backend the backend to use
     * @return a NodeGenerator using the given backend
     */
    [[nodiscard]] static NodeGenerator with_backends(std::unique_ptr<IIdGenerator> generator, std::unique_ptr<INodeFactory> factory);

    /**
     * Creates a new instances using the ReferenceNodeScope backend
     */
    static NodeGenerator new_instance() noexcept;

    /**
     * Creates a generator from the generator backend type, the factory backend type and constructor arguments
     * to construct the given backend types.
     *
     * @tparam GeneratorBackend type of generator backend to use
     * @tparam FactoryBackend type of factory backend to use
     * @param generator_args arguments to pass to the constructor of GeneratorBackend
     * @param factory_args arguments to pass to the constructor of FactoryBackend
     *
     * @return a NodeGenerator using the newly constructed backends
     */
    template<typename GeneratorBackend, typename FactoryBackend, typename ...GeneratorArgs, typename ...FactoryArgs>
        requires std::derived_from<GeneratorBackend, IIdGenerator> && std::derived_from<FactoryBackend, INodeFactory>
    static NodeGenerator new_instance(std::piecewise_construct_t, std::tuple<GeneratorArgs ...> generator_args, std::tuple<FactoryArgs ...> factory_args) {
        return NodeGenerator{std::apply(std::make_unique<GeneratorBackend>, std::move(generator_args)),
                             std::apply(std::make_unique<FactoryBackend>, std::move(factory_args))};
    }

    /**
     * Creates a new generator by default-constructing the given backends
     *
     * @tparam GeneratorBackend type of generator backend to use
     * @tparam FactoryBackend type of factory backend to use
     * @return the created generator
     */
    template<typename GeneratorBackend, typename FactoryBackend>
        requires std::derived_from<GeneratorBackend, IIdGenerator> && std::derived_from<FactoryBackend, INodeFactory>
    static NodeGenerator new_instance() {
        return NodeGenerator{std::make_unique<GeneratorBackend>(),std::make_unique<BNodeFactory>()};
    }

    /**
     * Creates a generator from the generator backend type and constructor arguments.
     * The factory backend will be BNodeFactory.
     *
     * @tparam GeneratorBackend type of generator backend to use
     * @param args arguments to pass to the constructor of GeneratorBackend
     *
     * @return a NodeGenerator using the newly constructed backends
     */
    template<typename GeneratorBackend, typename ...Args>
        requires std::derived_from<GeneratorBackend, IIdGenerator>
    static NodeGenerator new_instance_with_generator(Args &&...args) {
        return NodeGenerator{std::make_unique<GeneratorBackend>(std::forward<Args>(args)...),std::make_unique<BNodeFactory>()};
    }


    /**
     * Creates a generator from the factory backend type and constructor arguments.
     * The generator backend will be RandomIdGenerator.
     *
     * @tparam FactoryBackend type of factory backend to use
     * @param args arguments to pass to the constructor of FactoryBackend
     *
     * @return a NodeGenerator using the newly constructed backends
     */
    template<typename FactoryBackend, typename ...Args>
        requires std::derived_from<FactoryBackend, INodeFactory>
    static NodeGenerator new_instance_with_factory(Args &&...args) {
        return NodeGenerator{std::make_unique<RandomIdGenerator>(),std::make_unique<FactoryBackend>(std::forward<Args>(args)...)};
    }

    /**
     * Generates a new id as std::string using the backend.
     * @return a new id
     */
    [[nodiscard]] std::string generate_id();

    /**
     * Generates a new node by first generating a new id using the backend, and then
     * wrapping the id into a node by using the given factory.
     *
     * @param factory factory to wrap the generated id into a node
     * @param node_storage the node storage in which the new node is placed
     * @return the generated node
     */
    [[nodiscard]] Node generate_node(NodeStorage &node_storage = NodeStorage::default_instance());
};

}  //namespace rdf4cpp::rdf::util

#endif  //RDF4CPP_RDF_UTIL_BLANKNODEMANAGER_HPP
