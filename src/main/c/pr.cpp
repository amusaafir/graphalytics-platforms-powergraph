#include <graphlab.hpp>
#include <limits>

#include "algorithms.hpp"
#include "utils.hpp"


using namespace std;

static double global_damping_factor;
static double global_dangling_total;
static bool global_directed;

typedef double vertex_data_type;
typedef vertex_data_type gather_type;
typedef graphlab::distributed_graph<vertex_data_type, graphlab::empty> graph_type;


void init_vertex(graph_type::vertex_type &vertex, size_t num_vertices) {
    vertex.data() = 1.0 / num_vertices;
}


class pagerank :
    public graphlab::ivertex_program<graph_type, gather_type>,
    public graphlab::IS_POD_TYPE {

    public:
        edge_dir_type gather_edges(icontext_type& context, const vertex_type& vertex) const {
            return global_directed ? graphlab::IN_EDGES : graphlab::ALL_EDGES;
        }

        gather_type gather(icontext_type& context, const vertex_type& vertex, edge_type& edge) const {
            const vertex_type& other = edge.source().id() == vertex.id() ? edge.target() : edge.source();

            double v = other.data();
            int d = other.num_out_edges() + (global_directed ? 0 : other.num_in_edges());

            return gather_type(v / d);
        }

        void apply(icontext_type& context, vertex_type& vertex, const gather_type &total) {
            size_t num_vertices = context.num_vertices();

            vertex.data() = (1.0 - global_damping_factor) / num_vertices
                          + global_damping_factor * (total + global_dangling_total / num_vertices);

            context.signal(vertex);
        }

        edge_dir_type scatter_edges(icontext_type& context, const vertex_type& vertex) const {
            return graphlab::NO_EDGES;
        }
};

template <typename engine_type>
vertex_data_type get_vertex_data(typename engine_type::icontext_type& context, const graph_type::vertex_type& vertex) {
    return vertex.num_out_edges() == 0 ? vertex.data() : 0.0;
}

template <typename engine_type>
void set_total_residual(typename engine_type::icontext_type& context, vertex_data_type total) {
    global_dangling_total = total;
}


void run_pr(context_t &ctx, bool directed, double damping_factor, int max_iter) {
    typedef graphlab::omni_engine<pagerank> engine_type;

    // process parameters
    global_directed = directed;
    global_damping_factor = damping_factor;
    ctx.clopts.engine_args.set_option("max_iterations", max_iter);

    graph_type graph(ctx.dc);
    graph.load_format(ctx.graph_path, "snap");
    graph.finalize();

    graph.transform_vertices(boost::bind(init_vertex, _1, graph.num_vertices()));

    engine_type engine(ctx.dc, graph, "synchronous", ctx.clopts);

    // For directed graphs we need to collect the sum of vertices which are dangling (i.e., no outgoing edges)
    // after every iteration. For undirected graphs, dangling vertices do not exist so the sum is always zero.
    if (directed) {
        engine.add_vertex_aggregator<vertex_data_type>("residual",
                                                       &get_vertex_data<engine_type>,
                                                       &set_total_residual<engine_type>);

        engine.aggregate_now("residual");
        engine.aggregate_periodic("residual", 0);
    } else {
        global_dangling_total = 0;
    }

    engine.signal_all();
    engine.start();

    // print output
    const float runtime = engine.elapsed_seconds();
    ctx.dc.cerr() << "finished in " << runtime << " sec" << endl;

    if (ctx.output_stream) {
         vector<pair<graphlab::vertex_id_type, vertex_data_type> > data;
         collect_vertex_data(graph, data);

         for (size_t i = 0; i < data.size(); i++) {
             (*ctx.output_stream) << data[i].first << " " << data[i].second << endl;
         }
    }
}