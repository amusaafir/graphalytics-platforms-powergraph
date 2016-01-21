#include <graphlab.hpp>
#include <limits>

#include "algorithms.hpp"
#include "utils.hpp"

using namespace std;

class vertex_data_type;

typedef graphlab::vertex_id_type vertex_id_type;
typedef histogram<vertex_id_type> gather_type;
typedef gather_type::map_type neighbors_type;
typedef size_t msg_type;
typedef graphlab::distributed_graph<vertex_data_type, graphlab::empty> graph_type;

class vertex_data_type {
    public:
        double clustering_coef;
        neighbors_type neighbors;

        void save(graphlab::oarchive& oarc) const {
            oarc << clustering_coef << neighbors;
        }

        void load(graphlab::iarchive& iarc) {
            iarc >> clustering_coef >> neighbors;
        }
};

static bool global_directed;

class triangle_count :
    public graphlab::ivertex_program<graph_type, gather_type, msg_type>,
    public graphlab::IS_POD_TYPE {

    public:
        msg_type last_msg;

        void init(icontext_type& context, const vertex_type& vertex, const msg_type& msg) {
            last_msg = msg;
        }

        edge_dir_type gather_edges(icontext_type& context, const vertex_type& vertex) const {
            if (context.iteration() == 0) {
                return graphlab::ALL_EDGES;
            } else {
                return graphlab::NO_EDGES;
            }
        }

        gather_type gather(icontext_type& context, const vertex_type& vertex, edge_type& edge) const {
            if (context.iteration() == 0) {
                if (vertex.id() == edge.source().id()) {
                    return gather_type(edge.target().id());
                } else {
                    return gather_type(edge.source().id());
                }
            } else {
                return gather_type();
            }
        }

        void apply(icontext_type& context, vertex_type& vertex, const gather_type &total) {
            if (context.iteration() == 0) {
                vertex.data().neighbors = total.get();
            } else {
                size_t d = vertex.data().neighbors.size();
                size_t t = last_msg;
                vertex.data().clustering_coef = t / (d * (d - 1.0));
            }
        }

        edge_dir_type scatter_edges(icontext_type& context, const vertex_type& vertex) const {
            if (context.iteration() == 0) {
                return graphlab::OUT_EDGES;
            } else {
                return graphlab::NO_EDGES;
            }
        }

        pair<size_t, size_t> count_triangles(const vertex_type& a, const vertex_type& b) const {
            typedef neighbors_type::const_iterator neighbors_iterator_type;

            const vertex_id_type a_id = a.id();
            const vertex_id_type b_id = b.id();

            const neighbors_type& a_adj = a.data().neighbors;
            const neighbors_type& b_adj = b.data().neighbors;

            const bool directed = global_directed;

            if (a_adj.at(b_id) > 1 && a.id() < b.id()) {
                return make_pair(0, 0);
            }

            if (a_adj.size() > b_adj.size()) {
                return reverse(count_triangles(b, a));
            }

            size_t a_count = 0;
            size_t b_count = 0;

            for (neighbors_iterator_type it_a = a_adj.begin(); it_a != a_adj.end(); it_a++) {
                const vertex_id_type c_id = it_a->first;
                neighbors_iterator_type it_b = b_adj.find(it_a->first);

                if (it_b != b_adj.end()) {
                    if (b_id < c_id) {
                        a_count += directed ? it_b->second : 2;
                    }

                    if (a_id < c_id) {
                        b_count += directed ? it_a->second : 2;
                    }
                }
            }

            return make_pair(a_count, b_count);
        }

        void scatter(icontext_type& context, const vertex_type& vertex, edge_type& edge) const {
            if (context.iteration() == 0) {
                pair<size_t, size_t> p = count_triangles(edge.source(), edge.target());

                if (p.first > 0) {
                    context.signal(edge.source(), p.first);
                }

                if (p.second > 0) {
                    context.signal(edge.target(), p.second);
                }
            } else {
                //
            }
        }
};




void run_lcc(context_t& ctx, bool directed) {

    // process parmaeters
    global_directed = directed;

    // load graph
    graph_type graph(ctx.dc);
    graph.load_format(ctx.graph_path, "snap");
    graph.finalize();

    // start engine
    graphlab::omni_engine<triangle_count> engine(ctx.dc, graph, "synchronous", ctx.clopts);
    engine.signal_all();
    engine.start();

    const float runtime = engine.elapsed_seconds();
    ctx.dc.cout() << "finished in " << runtime << " sec" << endl;

    if (ctx.output_stream) {
        vector<pair<graphlab::vertex_id_type, vertex_data_type> > data;
        collect_vertex_data(graph, data);

        for (size_t i = 0; i < data.size(); i++) {
            (*ctx.output_stream) << data[i].first << " " << data[i].second.clustering_coef << endl;
        }
    }

}