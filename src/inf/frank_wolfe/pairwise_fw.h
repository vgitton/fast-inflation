#pragma once

#include "../../util/chrono.h"
#include "../../util/debug.h"
#include "frank_wolfe.h"

#include <map>

namespace inf {

/*! \ingroup fw
 * \brief A pairwise Frank-Wolfe algorithm
 * \details This class and the algorithm it implements are based on the algorithm described in Appendix C of the article
 * "Improved local models and new Bell inequalities via Frank-Wolfe algorithms",
 * by Sébastien Designolle, Gabriele Iommazzo, Mathieu Besançon, Sebastian Knebel, Patrick Gelß, and Sebastian Pokutta
 * published in Phys. Rev. Research 5, 043059 (2023).
 * See https://arxiv.org/abs/2302.04721.
 * The notation that we use in the documentation of this class refers to the notation of their Appendix C.
 *
 * We are minimizing the function \f$f(x) = \frac12\norm{x}^2\f$.
 * The gradient is thus \f$\nabla f(x) = x\f$.
 *
 * Note that our tests suggest that this algorithm is far slower than the fully-corrective algorithm of inf::FullyCorrectiveFW
 * for the typical inflation problems that we considered.
 * We leave this class for completeness, and to allow users that do not have access to Mosek to try out the code on simple instances.
 * It may well be that the implementation of inf::PairwiseFW is flawed and that a better implementation would perform better.
 *
 * The behavior of this class is tested in the user::pairwise_fw application.
 */
class PairwiseFW : public inf::FrankWolfe {
  public:
    /*! \brief The data held by the inf::PairwiseFW algorithm */
    class Data {
      public:
        /*! \brief This tolerance parameter is used to know when to use a drop step */
        static const double cleanup_tolerance;

        Data(Index dimension);
        //! \cond
        Data(Data const &) = delete;
        Data(Data &&) = delete;
        Data &operator=(Data const &) = delete;
        Data &operator=(Data &&) = delete;
        //! \endcond

        // Getters

        inline Index get_vertex_count() const {
            return m_vertex_count;
        }
        inline std::vector<inf::Event> const &get_events() const {
            return m_events;
        }
        /*! \brief The weight \f$q_\mu\f$ of the current iterate \f$x = \sum_\mu q_\mu d_\mu\f$
         * \param vertex_i Describes \f$\mu\f$ */
        inline double get_weight(Index vertex_i) const {
            ASSERT_LT(vertex_i, m_vertex_count)
            return m_weights[vertex_i];
        }
        /*! \brief The vertex \f$d_\mu\f$
         * \param vertex_i Describes \f$\mu\f$ */
        inline std::vector<double> const &get_vertex(Index vertex_i) const {
            ASSERT_LT(vertex_i, m_vertex_count)
            return m_vertices[vertex_i];
        }
        /*! \brief The inner product \f$\inner{x}{d_\mu}\f$ where \f$x\f$ denotes the current iterate
         * \param vertex_i Describes \f$\mu\f$ */
        inline double get_x_dot_vertex(Index vertex_i) const {
            return m_x_dot_vertex[vertex_i];
        }

      private:
        /*! \brief The list of inner products \f$\inner{d_\mu}{d_\lambda}\f$ is stored as a flat array
         * using the canonical bijection between \f$\N\f$ and \f$\N^2\f$
         * \param i Describes \f$\mu\f$
         * \param j Describes \f$\lambda\f$ */
        inline double &get_vertex_dot_vertex(Index i, Index j) {
            return m_vertex_dot_vertex[((i + j) * (i + j) + i + 3 * j) / 2];
        }

      public:
        /*! \brief The list of inner products \f$\inner{d_\mu}{d_\lambda}\f$ is stored as a flat array
         * using the canonical bijection between \f$\N\f$ and \f$\N^2\f$
         * \param i Describes \f$\mu\f$
         * \param j Describes \f$\lambda\f$ */
        inline double get_vertex_dot_vertex(Index i, Index j) const {
            return m_vertex_dot_vertex[((i + j) * (i + j) + i + 3 * j) / 2];
        }
        /*! \brief The norm \f$\norm{x}^2\f$ of the current iterate */
        inline double get_x_dot_x() const {
            return m_x_dot_x;
        }
        /*! \brief The current iterate \f$x\f$ */
        inline std::vector<double> const &get_x() const {
            return m_x;
        }

        // Interface

        /*! \brief Hard assertions about the state of the data */
        void check_health() const;

        /*! \brief Resets the data as if it had just been created from the constructor */
        void reset();

        /*! \brief Appends a zero weighted vertex, unless we had
         * no vertex so far, in which case it appends a one weighted vertex and also sets x to be this vertex */
        void memorize_event_and_vertex(inf::Event const &event,
                                       std::vector<double> const &vertex);

        /*! \brief Takes a pairwise step in the direction \f$a = d_\alpha - d_\lambda\f$
         * \param i_min Describes \f$\lambda\f$
         * \param i_max Describes \f$\alpha\f$ */
        void take_pairwise_step(Index i_min, Index i_max);

        /*! \brief Remove cache elements with low weight. Warning: this invalidates indices referring to the cache. */
        void clean_up_vertices();

      private:
        /*! \brief The dimension of every vertex */
        Index m_dimension;
        /*! \brief The size of m_events, m_weights, m_vertices, m_x_dot_vertex */
        Index m_vertex_count;
        /*! \brief At any point in time, m_events, m_weights and m_vertices
         * have the same size, and the entries of index i refer to each other. */
        std::vector<inf::Event> m_events;
        /*! \brief The weights \f$q_\mu\f$ of the current iterate \f$x = \sum_\mu q_\mu d_\mu\f$ */
        std::vector<double> m_weights;
        /*! \brief The list of vertices \f$\activeset = \{d_\mu\}_{\mu}\f$, each stored as a list of components for each dimension */
        std::vector<std::vector<double>> m_vertices;
        /*! \brief The i-th entry contains `<m_x, m_vertex_cache[i]>` */
        std::vector<double> m_x_dot_vertex;
        /*! \brief The f(i,j)-th entry contains `<m_vertex_cache[i], m_vertex_cache[j]>`,
         * where f is a bijection from \f$\mathbb{N}^{\times 2}\f$ to \f$\mathbb{N}\f$. */
        std::vector<double> m_vertex_dot_vertex;
        /*! \brief The norm squared of m_x */
        double m_x_dot_x;
        /*! \brief Always have `m_x = \sum_i m_weights[i]*m_vertices[i]`.
         * This vector is meant to minimize the Euclidean norm over the convex hull
         * of the cached vertices. */
        std::vector<double> m_x;

        /*! \brief Warning: this induces a re-ordering of the cache */
        void remove_vertex(Index vertex_i);

        /*! \brief This update m_x from the weights, and sets m_x_dot_vertex and m_x_dot_x */
        void update_x_from_weights();
    };

    /*! \brief This is used to keep track of the steps that we took to be able to plot the steps that
     * the algorithm follows in e.g. Mathematica
     * \details The first element indicates the current iterate \f$x\f$.
     * The second indicates the direction that was followed to obtain the iterate.
     * The third is true for Frank-Wolfe steps, and false for pairwise steps. */
    typedef std::tuple<std::vector<double>, std::vector<double>, bool> Iterate;

    /*! \brief To be compared to m_s_squared */
    static const double inconclusive_tolerance;
    /*! \brief The lazy tolerance \f$K\f$ */
    static const double lazy_tolerance;

    PairwiseFW(Index dimension);
    //! \cond
    PairwiseFW(PairwiseFW const &) = delete;
    PairwiseFW(PairwiseFW &&) = delete;
    PairwiseFW &operator=(PairwiseFW const &) = delete;
    PairwiseFW &operator=(PairwiseFW &&) = delete;
    //! \endcond

    inf::FrankWolfe::Solution solve() override;

    std::set<inf::Event> const &get_stored_events() const override;
    Index get_n_stored_events() const override;

    void reset() override;

    /*! \brief This allows to tell the inf::PairwiseFW to store the iterates for plotting purposes
     * \details It is recommended to not call this for performance, but it can be nice to investigate
     * what happens on small instances. */
    void set_store_iterates(bool);
    /*! \brief Returns the steps take by inf::PairwiseFW, see inf::PairwiseFW::Iterate */
    std::vector<inf::PairwiseFW::Iterate> get_iterates() const;

  protected:
    void memorize_event_and_quovec_double(inf::Event const &event,
                                          std::vector<double> const &row) override;

  private:
    /*! \brief Whether or not to store the iterates, see inf::PairwiseFW::Iterate. `false` by default. */
    bool m_store_iterates;
    /*! \brief The list of steps taken by inf::PairwiseFW, or empty list depending on `m_store_iterates` */
    std::vector<inf::PairwiseFW::Iterate> m_iterates;
    /*! \brief The last vertex that was used for a Frank-Wolfe step, only relevant if `m_store_iterates == true` */
    std::vector<double> m_last_fw_vertex;
    /*! \brief The vertices, current iterate and its weights */
    inf::PairwiseFW::Data m_data;
    /*! \brief This is mutable to allow the method inf::FrankWolfe::get_stored_events() to retrieve the set of events,
     * even though technically `m_events` has to change then to store the set of events. */
    mutable std::set<inf::Event> m_events;
    /*! \brief The parameter \f$\Phi\f$ of the pairwise algorithm */
    double m_phi;

    // Private methods

    /*! \brief Returns `true` if the norm \f$\norm{x}\f$ of the current iterate is too small */
    bool is_inconclusive() const;
    /*! \brief Returns the current iterate \f$x\f$, essentially */
    inf::FrankWolfe::Solution get_current_solution() const;
    /*! \brief This method determines the pair of vertices to use to do the next pairwise step
     * \details Two different approaches are proposed for this purpose, see the implementation */
    void find_min_and_max_inner_products(Index &i_min, Index &i_max) const;
};

} // namespace inf
