#ifndef UTILS_HPP
#define UTILS_HPP

#include <graphlab.hpp>
#include <limits>



template <typename T>
class histogram {

    // We cannot define INVALID_ITEM as a constant since numeric_limits::max is
    // an expression and cannot be assigned to a constant field. As a workaround,
    // we defined it as a macro and undefine it at the end of the class definition.
#define INVALID_ITEM (std::numeric_limits<T>::max())

    public:
        typedef typename boost::unordered_map<T, size_t> map_type;
        typedef typename map_type::const_iterator map_iterator_type;

        T first_item;
        map_type *data;

        histogram() {
            first_item = INVALID_ITEM;
            data = NULL;
        }

        histogram(T t) {
            first_item = t;
            data = NULL;
        }

        histogram(const histogram<T> &other) {
            first_item = INVALID_ITEM;
            data = NULL;
            *this = other;
        }

        histogram<T>& operator +=(const histogram<T>& other) {
            if (data == NULL) {
                data = new map_type;
                if (first_item != INVALID_ITEM) (*data)[first_item] = 1;
            }

            if (other.data != NULL) {
                for (map_iterator_type it = other.data->begin(); it != other.data->end(); it++) {
                    (*data)[it->first] += it->second;
                }
            } else if (other.first_item != INVALID_ITEM) {
                (*data)[other.first_item] += 1;
            }

            return *this;
        }

        histogram<T>& operator=(const histogram<T>& other) {
            if (data) delete data;
            data = NULL;

            first_item = other.first_item;
            if (other.data != NULL) data = new map_type(*other.data);

            return *this;
        }

        const map_type get() const {
            if (data == NULL) {
                map_type tmp;
                if (first_item != INVALID_ITEM) tmp[first_item] = 1;
                return tmp;
            }

            return *data;
        }

        void save(graphlab::oarchive& oarc) const {
            if (data == NULL) {
                map_type tmp;
                if (first_item != INVALID_ITEM) (*data)[first_item] = 1;
                oarc << tmp;
            } else {
                oarc << *data;
            }
        }

        void load(graphlab::iarchive& iarc) {
            if (data) delete data;
            data = new map_type;
            iarc >> data;
        }

        ~histogram() {
            if (data) delete data;
        }

#undef INVALID_ITEM
};

template <typename T>
struct min_reducer : public graphlab::IS_POD_TYPE {
    T value;

    public:
        min_reducer(T v=std::numeric_limits<T>::max()) {
            value = v;
        }

        min_reducer<T>& operator +=(const min_reducer<T>& other) {
            value = std::min(value, other.value);
            return *this;
        }

        T get() const {
            return value;
        }
};

template <typename A, typename B>
std::pair<B, A> reverse(std::pair<A, B> p) {
    return std::make_pair(p.second, p.first);
}


template <typename G>
void collect_vertex_data(G &graph, std::vector<std::pair<typename G::vertex_id_type, typename G::vertex_data_type> > &result) {
    typedef typename G::local_vertex_type local_vertex_type;

    for (size_t i = 0, n = graph.num_local_vertices(); i < n; i++) {
        const local_vertex_type& v = graph.l_vertex(i);

        if (v.owned()) {
            result.push_back(std::make_pair(v.global_id(), v.data()));
        }
    }

    // TODO ADD SUPPORT FOR MPI
}

#endif