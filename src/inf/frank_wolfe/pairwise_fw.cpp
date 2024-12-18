#include "pairwise_fw.h"

#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/math.h"
#include <cmath>
#include <limits>

// inf::PairwiseFW::Data

const double inf::PairwiseFW::Data::cleanup_tolerance = 1.0e-10;

inf::PairwiseFW::Data::Data(Index dimension)
    : m_dimension(dimension),
      m_vertex_count(0),
      m_events{},
      m_weights{},
      m_vertices{},
      m_x_dot_vertex{},
      m_vertex_dot_vertex{},
      m_x_dot_x(0.0),
      m_x(dimension, 0.0) {}

void inf::PairwiseFW::Data::check_health() const {
    util::logger << "Checking health... ";

    HARD_ASSERT_EQUAL(m_events.size(), m_vertex_count)
    HARD_ASSERT_EQUAL(m_weights.size(), m_vertex_count)
    HARD_ASSERT_EQUAL(m_vertices.size(), m_vertex_count)
    HARD_ASSERT_EQUAL(m_x_dot_vertex.size(), m_vertex_count)

    double const epsilon = 1.0e-10;

    // Check the normalization of the weights as well as the dimnensions of the vertices
    {
        double weight_sum = 0.0;
        for (Index const vertex_i : util::Range(m_vertex_count)) {
            HARD_ASSERT_EQUAL(m_vertices[vertex_i].size(), m_dimension)

            weight_sum += m_weights[vertex_i];
        }
        HARD_ASSERT_LT(std::abs(weight_sum - 1.0), epsilon)
    }

    // Check that we indeed have x = \sum_i w[i] * v[i]
    {
        double x_vs_convex_error = 0.0;
        for (Index const dim_i : util::Range(m_dimension)) {
            double convex_component = 0.0;
            for (Index vertex_i : util::Range(m_vertex_count)) {
                convex_component += m_weights[vertex_i] * m_vertices[vertex_i][dim_i];
            }
            x_vs_convex_error += std::abs(m_x[dim_i] - convex_component);
        }
        HARD_ASSERT_LT(x_vs_convex_error, epsilon)
    }

    // The norm of x
    HARD_ASSERT_LT(std::abs(m_x_dot_x - util::inner_product(m_x, m_x)), epsilon)

    // Check the dot products
    for (Index const vertex_i : util::Range(m_vertex_count)) {
        HARD_ASSERT_LT(std::abs(util::inner_product(m_x, m_vertices[vertex_i]) - m_x_dot_vertex[vertex_i]), epsilon)

        for (Index const vertex_j : util::Range(m_vertex_count)) {
            HARD_ASSERT_LT(std::abs(util::inner_product(m_vertices[vertex_i], m_vertices[vertex_j]) - get_vertex_dot_vertex(vertex_i, vertex_j)), epsilon)
        }
    }

    util::logger << "ok." << util::cr;
}

void inf::PairwiseFW::Data::reset() {
    m_vertex_count = 0;
    m_events.clear();
    m_weights.clear();
    m_vertices.clear();
    m_x_dot_vertex.clear();
    m_vertex_dot_vertex.clear();
    m_x_dot_x = 0.0;
    m_x = std::vector<double>(m_dimension, 0.0);
}

void inf::PairwiseFW::Data::memorize_event_and_vertex(inf::Event const &event,
                                                      std::vector<double> const &vertex) {
    ++m_vertex_count;
    m_events.push_back(event);
    m_vertices.push_back(vertex);

    if (m_vertex_count == 1) {
        m_weights.push_back(1.0);
        // Will be updated in update_x_from_weights()
        m_x_dot_vertex.push_back(42.0);
        // This effectively sets m_x = m_vertices.back()
        update_x_from_weights();
    } else {
        m_weights.push_back(0.0);
        m_x_dot_vertex.push_back(util::inner_product(vertex, m_x));
    }

    // Append to m_vertex_dot_vertex
    {
        // See notes for this formula. Based on bijection from pairs of natural numbers to natural numbers.
        m_vertex_dot_vertex.resize(2 * m_vertex_count * (m_vertex_count - 1) + 1);
        // The position where we just inserted
        const Index new_vertex_i = m_vertex_count - 1;
        for (Index vertex_j : util::Range(m_vertex_count)) {
            const double inner_product = util::inner_product(m_vertices[new_vertex_i], m_vertices[vertex_j]);
            get_vertex_dot_vertex(new_vertex_i, vertex_j) = inner_product;
            get_vertex_dot_vertex(vertex_j, new_vertex_i) = inner_product;
        }
    }
}

void inf::PairwiseFW::Data::take_pairwise_step(Index i_min, Index i_max) {
    // 1 - find gamma

    double gamma = 0.0;
    {
        double const norm_direction_squared =
            get_vertex_dot_vertex(i_max, i_max) - 2.0 * get_vertex_dot_vertex(i_max, i_min) + get_vertex_dot_vertex(i_min, i_min);

        if (norm_direction_squared > 1e-20) {
            // gamma = util::inner_product(m_x, direction) / norm_direction_squared;
            gamma = (m_x_dot_vertex[i_max] - m_x_dot_vertex[i_min]) / norm_direction_squared;
        }

        double const gamma_max = m_weights[i_max];

        if (gamma < 0.0)
            gamma = 0.0;
        else if (gamma > gamma_max)
            gamma = gamma_max;
    }

    // 2 - update x & norm of x

    m_x_dot_x = 0.0;
    for (Index const dim_i : util::Range(m_dimension)) {
        m_x[dim_i] -= gamma * (m_vertices[i_max][dim_i] - m_vertices[i_min][dim_i]);
        m_x_dot_x += m_x[dim_i] * m_x[dim_i];
    }

    // 3 - update m_x_dot_vertex

    for (Index vertex_i : util::Range(m_vertex_count)) {
        // <x', v_i> = <x, v_i> - gamma * <d, v_i>
        // = <x, v_i> - gamma * ( <v_max, v_i> - <v_min, v_i>)
        m_x_dot_vertex[vertex_i] -= gamma * (get_vertex_dot_vertex(i_max, vertex_i) - get_vertex_dot_vertex(i_min, vertex_i));
    }

    // 4 - update convex decomposition

    m_weights[i_min] += gamma;
    m_weights[i_max] -= gamma;
    if (m_weights[i_max] < inf::PairwiseFW::Data::cleanup_tolerance) {
        // Careful, doing this can invalidate i_min
        remove_vertex(i_max);
        // Here we could in principle renormalize but not needed
    }
}

void inf::PairwiseFW::Data::clean_up_vertices() {
    bool removed_at_least_one_vertex = false;
    Index vertex_i = 0;
    while (vertex_i < m_vertex_count) {
        if (m_weights[vertex_i] < inf::PairwiseFW::Data::cleanup_tolerance) {
            remove_vertex(vertex_i);
            removed_at_least_one_vertex = true;
        }
        // We don't increment i here since the new i-th element
        // is the old back element, which might also need to be clean up
        else
            ++vertex_i;
    }

    // We now need to renormalize
    double sum = 0.0;
    for (double weight : m_weights) {
        sum += weight;
    }

    if (sum > 1.0 + inf::PairwiseFW::Data::cleanup_tolerance) {
        THROW_ERROR("The vertex weights have a sum of " + util::str(sum))
    }

    for (double &weight : m_weights) {
        weight /= sum;
    }

    if (removed_at_least_one_vertex)
        update_x_from_weights();
}

// Private methods

void inf::PairwiseFW::Data::remove_vertex(Index vertex_i) {
    ASSERT_LT(vertex_i, m_vertex_count)

    --m_vertex_count;
    m_events[vertex_i] = m_events.back();
    m_events.pop_back();
    m_weights[vertex_i] = m_weights.back();
    m_weights.pop_back();
    m_vertices[vertex_i] = m_vertices.back();
    m_vertices.pop_back();
    m_x_dot_vertex[vertex_i] = m_x_dot_vertex.back();
    m_x_dot_vertex.pop_back();

    // Here don't reduce size of m_vertex_dot_vertex, just update the valid entries
    const Index last_vertex_i = m_vertex_count;
    // Suppose we have four vertices, and we remove vertex_i = 1.
    // The 4x4 matrix of inner products gets updated as follows:
    // [  -  p0   -  c0d ]
    // [ p0  p*  p1    d ]
    // [  -  p2   -  c1d ]
    // [ c0d   d c1d c*d ]
    // Legend:
    // - = nothing happens,
    // ci = copy to register i,
    // pi = paste register i,
    // d = deleted entry
    for (Index const vertex_j : util::Range(m_vertex_count)) {
        if (vertex_j == vertex_i)
            continue;

        double const inner_product = get_vertex_dot_vertex(last_vertex_i, vertex_j);
        get_vertex_dot_vertex(vertex_i, vertex_j) = inner_product;
        get_vertex_dot_vertex(vertex_j, vertex_i) = inner_product;
    }
    // Update the diagonal entry separately, it doesn't follow the above pattern
    get_vertex_dot_vertex(vertex_i, vertex_i) = get_vertex_dot_vertex(last_vertex_i, last_vertex_i);
}

void inf::PairwiseFW::Data::update_x_from_weights() {
    m_x_dot_x = 0.0;

    for (Index dim_i : util::Range(m_dimension)) {
        m_x[dim_i] = 0.0;

        for (Index vertex_i : util::Range(m_vertex_count)) {
            m_x[dim_i] += m_weights[vertex_i] * m_vertices[vertex_i][dim_i];
        }

        m_x_dot_x += m_x[dim_i] * m_x[dim_i];
    }

    for (Index vertex_i : util::Range(m_vertex_count)) {
        m_x_dot_vertex[vertex_i] = util::inner_product(m_x, m_vertices[vertex_i]);
    }
}

// inf::PairwiseFW

const double inf::PairwiseFW::inconclusive_tolerance = 1.0e-12;
const double inf::PairwiseFW::lazy_tolerance = 1.0;

inf::PairwiseFW::PairwiseFW(Index dimension)
    : inf::FrankWolfe(dimension),

      m_store_iterates(false),
      m_iterates{},
      m_last_fw_vertex{},

      m_data(dimension),
      m_events{},
      m_phi(0.0) {
    util::logger << "Creating an inf::PairwiseFW using:" << util::cr
                 << util::begin_comment << "inconclusive_tolerance = " << util::end_comment
                 << inf::PairwiseFW::inconclusive_tolerance << util::cr
                 << util::begin_comment << "        lazy_tolerance = " << util::end_comment
                 << inf::PairwiseFW::lazy_tolerance << util::cr
                 << util::begin_comment << "     cleanup_tolerance = " << util::end_comment
                 << inf::PairwiseFW::Data::cleanup_tolerance << util::cr;
}

// Public

inf::FrankWolfe::Solution inf::PairwiseFW::solve() {
    ASSERT_LT(0, m_data.get_vertex_count())

    // m_data.check_health();

    if (m_data.get_vertex_count() == 1) {
        return get_current_solution();
    }

    bool first_step = true;

    while (true) {
        // m_data.check_health();

        if (m_data.get_x_dot_x() < inf::PairwiseFW::inconclusive_tolerance)
            break;

        Index i_min, i_max;
        find_min_and_max_inner_products(i_min, i_max);
        double const pairwise_gap = m_data.get_x_dot_vertex(i_max) - m_data.get_x_dot_vertex(i_min);

        if (first_step and pairwise_gap < m_phi / inf::PairwiseFW::lazy_tolerance)
            m_phi *= 0.5;

        if (pairwise_gap < m_phi)
            break;

        m_data.take_pairwise_step(i_min, i_max);

        if (m_store_iterates) {
            if (first_step)
                m_iterates.emplace_back(m_data.get_x(), m_last_fw_vertex, true);
            else
                m_iterates.emplace_back(m_data.get_x(), std::vector<double>{}, false);
        }

        first_step = false;
    }

    return get_current_solution();
}

std::set<inf::Event> const &inf::PairwiseFW::get_stored_events() const {
    m_events = std::set<inf::Event>(m_data.get_events().begin(), m_data.get_events().end());
    return m_events;
}

Index inf::PairwiseFW::get_n_stored_events() const {
    return m_data.get_vertex_count();
}

void inf::PairwiseFW::reset() {
    m_iterates.clear();
    m_data.reset();
    m_phi = 0.0;
}

void inf::PairwiseFW::set_store_iterates(bool store) {
    m_store_iterates = store;
}

std::vector<inf::PairwiseFW::Iterate> inf::PairwiseFW::get_iterates() const {
    return m_iterates;
}

// Private & protected

void inf::PairwiseFW::memorize_event_and_quovec_double(inf::Event const &event,
                                                       std::vector<double> const &row) {
    m_data.memorize_event_and_vertex(event, row);

    if (m_data.get_vertex_count() == 1)
        m_phi = 0.5 * util::inner_product(row, row);

    if (m_store_iterates and m_data.get_vertex_count() == 1)
        m_iterates.emplace_back(row, std::vector<double>{0.0, 0.0}, true);

    if (m_store_iterates)
        m_last_fw_vertex = row;
}

bool inf::PairwiseFW::is_inconclusive() const {
    return m_data.get_x_dot_x() < inf::PairwiseFW::inconclusive_tolerance;
}

inf::FrankWolfe::Solution inf::PairwiseFW::get_current_solution() const {
    return inf::FrankWolfe::Solution(std::sqrt(m_data.get_x_dot_x()), m_data.get_x(), not this->is_inconclusive());
}

void inf::PairwiseFW::find_min_and_max_inner_products(Index &i_min, Index &i_max) const {
    // It's not clear what's best between true and false, both make sense for sure
    bool use_traditional_pairwise_fw = false;
    if (use_traditional_pairwise_fw) {
        double min_inner = std::numeric_limits<double>::max();
        // Don't use min() here!!!
        double max_inner = std::numeric_limits<double>::lowest();

        ASSERT_LT(0, m_data.get_vertex_count())

        for (Index vertex_i : util::Range(m_data.get_vertex_count())) {
            double const current_inner = m_data.get_x_dot_vertex(vertex_i);

            if (current_inner > max_inner) {
                max_inner = current_inner;
                i_max = vertex_i;
            }

            if (current_inner < min_inner) {
                min_inner = current_inner;
                i_min = vertex_i;
            }

            ASSERT_LT(std::numeric_limits<double>::lowest(), max_inner)
            ASSERT_LT(min_inner, std::numeric_limits<double>::max())
        }
    } else {
        // Renormalize
        double best_score = std::numeric_limits<double>::lowest();

        ASSERT_LT(0, m_data.get_vertex_count())

        Index vertex_i, vertex_j;

        for (vertex_i = 0; vertex_i < m_data.get_vertex_count(); ++vertex_i) {
            for (vertex_j = 0; vertex_j < m_data.get_vertex_count(); ++vertex_j) {
                // Note: pairs (vertex_i, vertex_j) are ordered to take the sign into account
                if (vertex_j == vertex_i)
                    continue;

                double const norm = std::sqrt(
                    m_data.get_vertex_dot_vertex(vertex_i, vertex_i) - 2.0 * m_data.get_vertex_dot_vertex(vertex_i, vertex_j) + m_data.get_vertex_dot_vertex(vertex_j, vertex_j));

                if (norm > 0.0) {
                    double const score = (m_data.get_x_dot_vertex(vertex_i) - m_data.get_x_dot_vertex(vertex_j)) / norm;

                    if (score > best_score) {
                        best_score = score;
                        i_min = vertex_j;
                        i_max = vertex_i;
                    }
                }
            }
        }
    }

    ASSERT_LT(i_min, m_data.get_vertex_count())
    ASSERT_LT(i_max, m_data.get_vertex_count())
}
