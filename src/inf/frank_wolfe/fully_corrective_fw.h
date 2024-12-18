#pragma once

#include "frank_wolfe.h"

// For mosek
#include <fusion.h>

#include <set>

#include "../../types.h"
#include "../../util/chrono.h"
#include "../../util/loggable.h"

/*! \file */

namespace inf {

/*! \ingroup fw
 * \brief The fully-corrective algorithm
 * \details This algorithm has proven to be the fastest one for the typical inflation problems that we consider,
 * which, in the landscape of general polytope membership problems, is characterized as by few dimensions (few scalar constraints), in the order of thousands,
 * but many extremal vertices (many inflation events), in the order of billions.
 * It requires Mosek, see https://docs.mosek.com/latest/cxxfusion/install-interface.html for details on how to install it.
 *
 * The fully-corrective Frank-Wolfe algorithm stores all of the vertices (quovecs) is receives through inf::FrankWolfe::memorize_event_and_quovec(), and
 * the function inf::FullyCorrectiveFW::solve() returns an inf::FrankWolfe::Solution that holds the minimum value \f$\minnorm \geq 0\f$
 * as well as an optimizer \f$\quovec\in\totquovecspace\f$ of the following minimization problem:
 * \f[
 *     \min \Big\{ \norm{\quovec} \Bigsetst \quovec \in \conv(\activeset) \Big\}.
 * \f]
 * As shown in the paper, the equivalent dual quadratic problem is
 * \f[
 *     \max \Big\{ s \Bigsetst \quovec \in \totquovecspace, \norm{\quovec} \leq 1, s \in \R \text{ s.t. } \forall \quovec' \in \activeset \st
 *     s \leq \inner{\quovec}{\quovec'}_{\constraintlist} \Big\}.
 * \f]
 * It is convenient to solve the dual problem rather than the primal problem, in particular, because
 * it allows us to reuse the same Mosek model when successively solving the above dual problem;
 * we just need to add one more constraint every time a new quovec is added to the active set \f$\activeset\f$.
 *
 * A basic test checking that this class behaves as expected is provided in the class user::fully_corrective_fw.
 *
 * Notice that this class can be accelerated using multithreading, which Mosek supports out of the box.
 * We have not thoroughly tested to what extent this leads to a faster resolution of the underlying quadratic problem,
 * but is does not seem to hurt, at least. */
class FullyCorrectiveFW : public inf::FrankWolfe {
  public:
    /*! \brief Initialize the Frank-Wolfe problem with \f$\activeset = \emptyset\f$
        \param dimension The number of variables, or equivalently, the dimension of the vector space \f$\totquovecspace\f$.
        \param n_threads The number of threads to be used by Mosek to solve the quadratic program */
    FullyCorrectiveFW(Index dimension, Index n_threads);
    //! \cond
    FullyCorrectiveFW(inf::FullyCorrectiveFW const &other) = delete;
    FullyCorrectiveFW(inf::FullyCorrectiveFW &&other) = delete;
    inf::FullyCorrectiveFW &operator=(inf::FullyCorrectiveFW const &other) = delete;
    inf::FullyCorrectiveFW &operator=(inf::FullyCorrectiveFW &&other) = delete;
    //! \endcond

    inf::FrankWolfe::Solution solve() override;
    std::set<inf::Event> const &get_stored_events() const override;
    Index get_n_stored_events() const override;

    void reset() override;

  protected:
    void memorize_event_and_quovec_double(inf::Event const &event,
                                          std::vector<double> const &row) override;

  private:
    /*! \brief The number of threads that Mosek will use */
    Index const m_n_threads;
    /*! \brief The events passed to inf::FrankWolfe::memorize_event_and_quovec() are all stored in this set denoted \f$\activeset\f$ in the paper */
    std::set<inf::Event> m_events;
    /*! \brief `m_matrix[i]` stores the i-th vertex/quovec \f$\quovec\in\totquovecspace\f$ received from inf::FrankWolfe::memorize_event_and_quovec() */
    std::vector<std::vector<double>> m_matrix;
    /*! \brief The Mosek model instance */
    mosek::fusion::Model::t m_model;
    /*! \brief Initializes `m_model` */
    void init_model();
    /*! \brief This Mosek object takes care of freeing up the memory allocated for `m_model` */
    monty::finally m_model_disposer;
    /*! \brief The variable \f$w\f$ in the dual maximization problem */
    mosek::fusion::Variable::t m_w;
    /*! \brief Initializes `m_w` */
    void init_w();
    /*! \brief The norm variable \f$s\f$ in the dual maximization problem */
    mosek::fusion::Expression::t m_s;
    /*! \brief Initializes `m_s` */
    void init_s();

    // Utilities

    /*! \brief Converts a Mosek variable to a Mosek expression
     * \details This explicit conversion is used to avoid compiler warnings */
    static mosek::fusion::Expression::t var_to_expr(mosek::fusion::Variable::t &var);

    /*! \brief Stringifies Mosek's problem status */
    static std::string to_str(mosek::fusion::ProblemStatus pb_status);
    /*! \brief Stringifies Mosek's solution status */
    static std::string to_str(mosek::fusion::SolutionStatus sol_status);

    /*! \brief This returns \f$\min_i \inner{\quovec_i}{v}\f$, where \f$v\f$ represents \p vec and
     * where \f$\quovec_i\f$ represents `m_matrix[i]` */
    double get_min_inner_product(std::vector<double> const &vec) const;
};
} // namespace inf
