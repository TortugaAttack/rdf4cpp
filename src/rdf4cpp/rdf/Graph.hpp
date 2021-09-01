#ifndef RDF4CPP_GRAPH_HPP
#define RDF4CPP_GRAPH_HPP

#include <rdf4cpp/rdf/Statement.hpp>
#include <rdf4cpp/rdf/query/TriplePattern.hpp>
#include <rdf4cpp/rdf/storage/tuple/DatasetStorage.hpp>

#include <memory>
#include <utility>

namespace rdf4cpp::rdf {

class Dataset;

class Graph {
    friend class Dataset;
    using DatasetStorage = ::rdf4cpp::rdf::storage::tuple::DatasetStorage;
    
    using NodeStorage = storage::node::NodeStorage;

    DatasetStorage dataset_storage;
    IRI graph_name = IRI::default_graph(dataset_storage.node_storage());

    Graph(DatasetStorage dataset_backend, const IRI &graph_name);

public:
    // TODO: allow to change default backend impl.
    explicit Graph(NodeStorage node_storage = NodeStorage::primary_instance());

    explicit Graph(const IRI &graph_name, NodeStorage node_storage = NodeStorage::primary_instance());

    void add(const Statement &statement);

    [[nodiscard]] bool contains(const Statement &statement) const;

    [[nodiscard]] query::SolutionSequence match(const query::TriplePattern &triple_pattern) const;

    [[nodiscard]] size_t size() const;

    Dataset dataset();

    [[nodiscard]] const IRI &name() const;

    DatasetStorage &backend();

    [[nodiscard]] const DatasetStorage &backend() const;

    // TODO: support union (+) and difference (-); open question: which graph name should be assigned?
    // TODO: add empty
};
}  // namespace rdf4cpp::rdf

#endif  //RDF4CPP_GRAPH_HPP
