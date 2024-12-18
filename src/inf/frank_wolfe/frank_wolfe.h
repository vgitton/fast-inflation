#pragma once

#include "../../util/chrono.h"
#include "../../util/loggable.h"
#include "../events/event.h"

#include <memory>
#include <set>

namespace inf {

/*! \defgroup fw Frank-Wolfe
 * \ingroup infback
 * \brief The Frank-Wolfe algorithms enabling the discovery of nonlocality certificates
 * \details The general structure of these algorithms is described in inf::FrankWolfe.
 * */

/*! \ingroup fw
 * \brief The interface to the different Frank-Wolfe algorithms that we implement
 * \details The general Frank-Wolfe algorithm as used in inf::FeasProblem works as follows:
 * 1. Store some inflation event \f$\infevent\in\infevents\f$ and the associated quovec \f$\quovec = \totconstraintmap(\detdistr\infevent) \in \totquovecspace\f$ (see inf::FeasProblem::init_frank_wolfe()).
 * Let \f$\mathcal S \leftarrow \{\quovec\}\f$ (although, see inf::FeasProblem::RetainEvents).
 * 2. Using inf::Optimizer::optimize(), minimize the inner product \f$\inner{\quovec}{\totconstraintmap(\detdistr{\infevent'})}_{\constraintlist}\f$ for all \f$\infevent' \in \infevents\f$.
 * 3. If the result of the minimization is strictly positive, then \f$\quovec\f$ is a nonlocality certificate, as shown in the paper.
 * Else, let \f$\mathcal S \leftarrow \mathcal S \cup \{\totconstraintmap(\detdistr{\infevent^*})\}\f$ (see inf::FrankWolfe::memorize_event_and_quovec()), where \f$\infevent^*\f$ is the inflation event with the minimum score.
 * 4. Based on \f$\mathcal S\f$, find a new dual vector \f$\quovec \in \totquovecspace\f$ (see inf::FrankWolfe::solve()), and go back to step 2.
 * If no new dual vector \f$\quovec\f$ could be found, exit the loop, concluding that the inf::FeasProblem is inconclusive.
 *
 * We propose two inf::FrankWolfe algorithms to solve step 4, see inf::FrankWolfe::Algo.
 * - inf::FullyCorrectiveFW is the most efficient algorithm for the typical inflation problems that we consider,
 *   which, in the landscape of general polytope membership problems, is characterized as by few dimensions (few scalar constraints), in the order of thousands,
 *   but many extremal vertices (many inflation events), in the order of billions.
 *   It requires Mosek, see https://docs.mosek.com/latest/cxxfusion/install-interface.html for details on how to install it.
 * - inf::PairwiseFW is in principle more scalable for general polytope membership problems with many dimensions (say, millions of dimensions), and does not require
 *   to setup Mosek. We leave this algorithm for completeness but we expect this algorithm to be much slower than inf::FullyCorrectiveFW in applications.
 * */
class FrankWolfe {
  public:
    /*! \brief To time the total time spent solving the Frank-Wolfe problem
     * \details I.e., the problem of finding the next inf::DualVector that may be a separating hyperplane
     * based on the inflation events that have been encountered. */
    static util::Chrono total_fw_chrono;

    typedef std::unique_ptr<inf::FrankWolfe> UniquePtr;

    /*! \brief The different types of Frank-Wolfe algorithms that we implement, see inf::FrankWolfe for a general description */
    enum class Algo {
        fully_corrective, ///< Fully-corrective Frank-Wolfe
        pairwise,         ///< Pairwise Frank-Wolfe
    };

    static void log(inf::FrankWolfe::Algo algo);

    /*! \brief To conveniently instantiate a subclass an inf::FullyCorrectiveFW or an inf::PairwiseFW
     * \param algo The algorithm choice
     * \param dimension The dimension of the space in which the Frank-Wolfe algorithm takes place,
     * which in our case is the dimension of dual vectors \f$\quovec \in \totquovecspace\f$,
     * i.e.,
     * \f[
     *     \sum_{\constraintname=(\infmarg_0,\dots,\infmarg_{k-1},\infmargg) \in \constraintlist} |\events{\infmarg_0\cdots\infmarg_{k-1}\infmargg} \setminus \marggroup|,
     * \f]
     * where \f$\constraintlist \subset \infconstraints\f$ is the set of constraints describing the inflation problem at hand,
     * see inf::ConstraintSet.
     * \param n_threads The number of threads to use. Currently, this is only used if `algo == inf::FrankWolfe::Algo::fully_corrective`, see inf::FullyCorrectiveFW. */
    static inf::FrankWolfe::UniquePtr get_frank_wolfe(inf::FrankWolfe::Algo algo,
                                                      Index dimension,
                                                      Index n_threads);

    /*! \brief This describes the solution that inf::FrankWolfe returns */
    class Solution : public util::Loggable {
      public:
        /*! \brief To be initialized with \p s the norm of the dual vector \p vec
         * \param s The Euclidean norm \f$\lVert \quovec \lVert\f$, where \f$\quovec\f$ is described by \p vec
         * \param vec The next dual vector \f$\quovec \in \totquovecspace\f$
         * \param valid Describes whether or not \p vec is a valid solution --- essentially, this is `true` is
         * \p vec has a nonzero norm. */
        Solution(double s, std::vector<double> const &vec, bool valid);

        /*! The Euclidean norm \f$\lVert \quovec \lVert\f$, where \f$\quovec\f$ is described by \p vec */
        const double s;
        /*! \brief The next dual vector \f$\quovec \in \totquovecspace\f$ */
        const std::vector<double> vec;
        /*! \brief Describes whether or not \p vec is a valid solution --- essentially, this is `true` is
         * \p vec has a nonzero norm. */
        const bool valid;

        void log() const override;
    };

    /*! \brief This just initializes `m_dimension` */
    FrankWolfe(Index dimension);
    virtual ~FrankWolfe() {}

    //! \cond
    FrankWolfe(FrankWolfe const &) = delete;
    FrankWolfe(FrankWolfe &&) = delete;
    FrankWolfe &operator=(FrankWolfe const &) = delete;
    FrankWolfe &operator=(FrankWolfe &&) = delete;
    //! \endcond

    /*! \brief This method is called with the output of the inf::Optimizer, notifying the Frank-Wolfe algorithm that a new interesting
     * inflation event \f$\infevent\in\infevents\f$ with associated quovec \f$\quovec = \totconstraintmap(\detdistr\infevent)\f$ has been found
     * \details This rescales \p quovec by dividing each of its component by \p denom and calls inf::FrankWolfe::memorize_event_and_quovec_double()
     * \param event The inflation event \f$\infevent\in\infevents\f$ associated to \p quovec
     * \param quovec Up to a constant scale factor, this is the dual vector \f$\quovec = \totconstraintmap(\detdistr\infevent)\f$, where \f$\infevent\f$ corresponds to \p event
     * \param denom The scale factor that will divide each component of \p quovec to obtain a floating-point dual vector */
    void memorize_event_and_quovec(inf::Event const &event,
                                   std::vector<Num> const &quovec,
                                   double denom);
    /*! \brief This times a call to inf::FrankWolfe::solve() */
    inf::FrankWolfe::Solution time_and_solve();
    /*! \brief This solves the Frank-Wolfe problem, i.e., the problem of finding the next inf::DualVector that may be a separating hyperplane
     * based on the inflation events that have been encountered. */
    virtual inf::FrankWolfe::Solution solve() = 0;
    /*! \brief This returns the inflation events that have been passed to inf::FrankWolfe::memorize_event_and_quovec()
     * since the last call of inf::FrankWolfe::reset() */
    virtual std::set<inf::Event> const &get_stored_events() const = 0;
    /*! \brief This corresponds to `inf::FrankWolfe::get_stored_events().size()` */
    virtual Index get_n_stored_events() const = 0;
    /*! \brief This is used to reset the inf::FrankWolfe algorithm to its initial state (m_dimension does not change).
     * \details When the previously encountered inflation events are needed, the user should call inf::FrankWolfe::get_stored_events() first */
    virtual void reset() = 0;

  protected:
    /*! \brief dimension The dimension of the space in which the Frank-Wolfe algorithm takes place
     * \details In our case, this is the dimension of dual vectors \f$\quovec \in \totquovecspace\f$,
     * i.e.,
     * \f[
     *     \sum_{\constraintname=(\infmarg_0,\dots,\infmarg_{k-1},\infmargg) \in \constraintlist} |\events{\infmarg_0\cdots\infmarg_{k-1}\infmargg} \setminus \marggroup|,
     * \f]
     * where \f$\constraintlist \subset \infconstraints\f$ is the set of constraints describing the inflation problem at hand,
     * see inf::ConstraintSet. */
    const Index m_dimension;
    /*! \brief This method is called by inf::FrankWolfe::memorize_event_and_quovec()
     * \details This will be called with the output of the inf::Optimizer, notifying the Frank-Wolfe algorithm that a new interesting
     * inflation event \f$\infevent\in\infevents\f$ with associated quovec \f$\quovec = \totconstraintmap(\detdistr\infevent)\f$ has been found
     * \param event The inflation event \f$\infevent\in\infevents\f$ associated to \p quovec
     * \param quovec The dual vector \f$\quovec = \totconstraintmap(\detdistr\infevent)\f$, where \f$\infevent\f$ corresponds to \p event */
    virtual void memorize_event_and_quovec_double(inf::Event const &event,
                                                  std::vector<double> const &quovec) = 0;
};

} // namespace inf
