#pragma once

#include "../inf_problem/inflation.h"
#include "../symmetry/symmetry.h"
#include "../symmetry/tensor_with_orbits.h"
// Here we use a forward declaration rather than an #include because
// inf::Marginal depends on inf::DualVector::StoreBounds
namespace inf {
class Marginal;
}
#include "../../util/frac.h"
#include "../../util/math.h"

/*! \file */

namespace inf {

/*! \ingroup infcons
 * \brief A "quotiented vector" denotes an element of some \f$\quovecspace{\infmarg}\f$ for some \f$\infmarg\subset\infparties\f$
 * \details Given an inflation constraint \f$\constraintname = (\infmarg_0,\dots,\infmarg_{k-1},\infmargg) \in \infconstraints\f$,
 * the group of constraint symmetries \f$\marggroup\f$ denotes all the inflation symmetries of \f$\distrgroup\f$ (see inf::Inflation::get_inflation_symmetries())
 * that leave both \f$\infmarg_0 \cup \dots \cup \infmarg_{k-1}\f$ and \f$\infmargg\f$ invariant.
 * Then, for an inflation marginal \f$\infmarg \in \{\infmarg_0\cup\cdots\infmarg_{k-1}\cup\infmargg, \infmargg\}\f$,
 * the reduced set of marginal events is \f$\repr{\events\infmarg \setminus \marggroup}\f$,
 * and the quovec vector space \f$\quovecspace\infmarg\f$ is the vector space of all functions \f$\repr{\events\infmarg\setminus\marggroup} \to \R\f$.
 * (In fact, thanks to the way we approach exact arithmetic, a quovec has its image in \f$\Z\f$.)
 * A quovec \f$\quovec\in\quovecspace\infmarg\f$ is occasionally stored in an inf::DualVector when we require the embedding \f$\altquovecembed\infmarg(\quovec)\f$.
 * */
typedef std::vector<Num> Quovec;
/*! \ingroup infcons
 * \brief We use this type to explicitly indicate that an index indicates a component of an inf::Quovec */
typedef Index QuovecIndex;

/*! \ingroup infcons
 * \brief Describes a dual vector \f$\quovec\in\quovecspace{\infmarg}\f$ through its embedding \f$\altquovecembed\infmarg(\quovec) \in \vecset\infmarg\f$
 * \details Given an inflation constraint \f$\constraintname = (\infmarg_0,\dots,\infmarg_{k-1},\infmargg) \in \infconstraints\f$,
 * the group of constraint symmetries \f$\marggroup\f$ denotes all the inflation symmetries of \f$\distrgroup\f$ (see inf::Inflation::get_inflation_symmetries())
 * that leave both \f$\infmarg_0 \cup \dots \cup \infmarg_{k-1}\f$ and \f$\infmargg\f$ invariant.
 * Then, for an inflation marginal \f$\infmarg \in \{\infmarg_0\cup\cdots\infmarg_{k-1}\cup\infmargg, \infmargg\}\f$,
 * the reduced set of marginal events is \f$\repr{\events\infmarg \setminus \marggroup}\f$,
 * and the quovec vector space \f$\quovecspace\infmarg\f$ is the vector space of all functions \f$\repr{\events\infmarg\setminus\marggroup} \to \R\f$.
 *
 * ### Orbits
 *
 * The inf::DualVector first task is to compute the orbits \f$\events\infmarg \setminus \marggroup\f$.
 * This then allow to go from the inf::Quovec representation \f$\quovec \in \quovecspace\infmarg\f$ to the inf::EventTensor representation \f$\altquovecembed\infmarg(\quovec)\in\vecset{\infmarg}\f$.
 * Most of the relevant code is inherited from the inf::Orbitable and inf::TensorWithOrbits classes to avoid duplication of code that is
 * also used by inf::TargetDistr.
 *
 * ### Bounds
 *
 * Another important task of the inf::DualVector class is to compute what we call "bounds" for short.
 * Here is the idea. Suppose that an inf::EventTensor (the inf::DualVector owns such an inf::EventTensor through the inf::TensorWithOrbits inheritance)
 * assigns to the event `(0,0)` the value `12` and to the event `(0,1)` the value `15`.
 * It will be useful for the branch-and-bound algorithm of the inf::TreeOpt and inf::LiveTreeOpt classes to be able to tell in advance
 * what the highest/lowest score of an event that is only partially filled (known) would be.
 * In our example, the partially filled event `(0,?)` (which can be filled to be either `(0,0)` or `(0,1)`) would have `12` as a lower bound
 * on its score and `15` as an upper bound on its score. (By "score" we just mean the value that the inf::DualVector / inf::EventTensor assigns to it.)
 *
 * The way the bounds are stored is explained in inf::DualVector::StoreBounds, which is controles whether or not an inf::DualVector computes these bounds.
 * This is passed to inf::Constraint so that it can adequately initialize each of its inf::DualVector.
 * Whether the bounds that are computed are upper or lower bounds is controlled with the type inf::DualVector::BoundType.
 * The idea is that the left-hand-side inf::DualVector stored in the inf::Constraint has a positive scale factor, and since our optimization algorithms
 * are *minimization* algorithms, we need the left-hand-side dual vector to compute lower bounds.
 * The right-hand-side inf::DualVector stored in the inf::Constraint has a negative scale factor, and so it needs to compute upper bounds.
 *
 * The way the bounds are computed is explained in inf::DualVector::BoundRule.
 *
 * The bound mechanism is tested in the application user::dual_vector_bounds and user::opt_bounds.
 *
 * An inf::DualVector is an util::Serializable, allowing to write/read them from disks, which we use for nonlocality certicates as described in inf::FeasProblem.
 * This serialization mechanism is tested in the application user::dual_vector_io.
 * */
class DualVector : public inf::TensorWithOrbits, public util::Serializable {
  public:
    typedef std::unique_ptr<inf::DualVector> UniquePtr;

    /*! \brief This controls whether or not an inf::DualVector will store some bounds
     * \details The type of bounds is controlled with inf::DualVector::BoundType.
     * The way the bounds are stored is as follows: the bound for the partially filled event `(0,?)` is stored
     * as if the number of outcomes was one more than the "physical" number of outcomes, i.e., instead of \f$\nouts\f$
     * outcome, we now have \f$\nouts+1\f$ outcome. The last outcome represents the "unknown" outcome, denoted "?" in the documentation.
     * */
    enum class StoreBounds {
        yes, ///< Do store bounds, implies a slightly increased memory usage
        no   ///< Do not store bounds
    };

    static void log(inf::DualVector::StoreBounds store_bounds);

    // We need this to make the `util::logger << dual_vector` syntax possible, otherwise the abovel log()
    // function hides it
    using inf::TensorWithOrbits::log;

    /*! \brief This controls whether the bounds that the inf::DualVector computes are upper or lower bounds */
    enum class BoundType {
        lower, ///< Lower bounds are computed
        upper  ///< Upper bounds are computed
    };

    /*! \brief This method returns `n_outcomes+1` if `store_bounds == inf::DualVector::StoreBounds::yes` and `n_outcomes` otherwise */
    static inf::Outcome get_outcomes_per_party(inf::Outcome n_outcomes, inf::DualVector::StoreBounds store_bounds);
    /*! \brief This method calls inf::EventTensor::get_party_weights(), but increments `n_outcomes` by 1 if `store_bounds == inf::DualVector::StoreBounds::yes`
     * \details Recall that this method is useful (e.g., for inf::Marginal) to be able to manipulate event hashes directly, and thus save
     * a bit of time when querying inf::EventTensor::get_num() */
    static std::vector<Index> compute_weights(Index n_parties, inf::Outcome n_outcomes, inf::DualVector::StoreBounds store_bounds);

    // Constructor

    /*! \brief The \p marginal contains the necessary information to initialize the inf::DualVector
     * \param marginal Identifies the marginal \f$\infmarg \subset \infparties\f$, and furthermore indicates the
     * relevant constraint symmetries \f$\marggroup\f$ (in a slightly compressed form: inf::Marginal::get_marginal_symmetries()
     * returns party symmetries that act directly on the indices of marginal events)
     * \param bound_type See the description of inf::DualVector
     * */
    DualVector(inf::Marginal const &marginal, inf::DualVector::BoundType bound_type);
    //! \cond ignore deleted
    DualVector(inf::DualVector const &other) = delete;
    DualVector(inf::DualVector &&other) = delete;
    DualVector &operator=(inf::DualVector const &other) = delete;
    DualVector &operator=(inf::DualVector &&other) = delete;
    //! \endcond

    // Getters and setters

    /*! \brief The underlying inflation */
    inf::Inflation::ConstPtr const &get_inflation() const;

    /*! \brief To map the hash of each marginal event \f$\infevent \in \events\infmarg\f$ to the quovec index of its orbit
     * \details This returns `m_event_to_quovec_index`.
     * Recall that inf::EventTensor assigns a hash to each event it is valued on (see inf::EventTensor::get_event_hash()).
     * Given an event \f$\infevent\in\events\infmarg\f$ with hash `i`, `m_event_to_quovec_index[i]` identifies
     * the orbit (or its representative) \f$\act{\marggroup}{\events\infmarg}(\infevent)\f$.
     * The representatives are then ordered and assigned an index `m_event_to_quovec_index[i]`. */
    std::vector<inf::QuovecIndex> const &get_event_to_quovec_index() const;

    /*! \brief This sets the coefficients (image) of the inf::DualVector, including bounds if necessary
     * \details The idea is that the underlying `m_event_tensor` (an inf::EventTensor) will have its coefficients set as
     * `m_event_tensor[j] = quovec[start_pos + i]`, where `i = get_event_to_quovec_index()[j]`,
     * which represents the embedding \f$\altquovecembed\infmarg(\quovec)(\infevent) = \quovec(o_{\infevent})\f$,
     * where \f$\infevent \in \events\infmarg\f$ is a marginal event (assigned the hash `j`),
     * \f$\quovec\in\quovecspace\infmarg\f$ is the parameter \p quovec (offset with \p start_pos),
     * \f$\altquovecembed\infmarg(\quovec) \in \vecset\infmarg\f$ represents the underlying `m_event_tensor`,
     * and \f$o_{\infevent} = \act{\marggroup}{\events\infmarg}(\infevent)\f$ denotes the orbit (assigned the index `i`)
     * of the marginal event \f$\infevent\f$ .
     *
     * Furthermore, this methods compute the lower/upper bound, depending on how the inf::DualVector was initialized
     * (see inf::DualVector::StoreBounds and inf::DualVector::BoundType).
     *
     * \param start_pos Indicates where to start reading the quovec. This is there to allow us to represent
     * a set of quovec \f$\{\quovec_\constraintname\}_{\constraintname\in\constraintlist}\f$ as a flat list of coefficients
     * when we deal with a set of constraints \f$\constraintlist \subset \infconstraints\f$. */
    void set_from_quovec(inf::Quovec const &quovec, inf::QuovecIndex start_pos);

    /*! \brief This hard-asserts that the dual vector's components are less than the specified \p bound in absolute value
     * \param bound Denoted \f$B\f$ in the paper */
    void hard_assert_within_bound(Num const bound) const;

    // Overrides from inf::TensorWithOrbits

    std::string get_math_name() const override;
    void log_header() const override;

    // Overrides from inf::Orbitable

    void log_event(inf::Event const &event) const override;

    // We need to modify the access to the number of orbits based on whether or not there are unknowns
    // Hide these ones, they are ambiguous
  private:
    using inf::Orbitable::get_orbits;
    using inf::TensorWithOrbits::get_n_orbits;

  public:
    /*! \brief Returns the total number of orbits of the dual vector, including the one with unknowns */
    Index get_n_orbits_with_unknown() const;
    /*! \brief Returns the number of orbits with no unknown of the dual vector */
    Index get_n_orbits_no_unknown() const;
    /*! \brief Returns the orbit representatives with no unknowns */
    std::vector<inf::Event> const &get_orbit_repr_no_unknown() const;

    // Override from util::Serializable

    void io(util::FileStream &stream) override;

  private:
    /*! \brief The underlying inflation */
    const inf::Inflation::ConstPtr m_inflation;

    /*! \brief Whether or not the inf::DualVector stores bounds, see inf::DualVector::StoreBounds */
    const inf::DualVector::StoreBounds m_store_bounds;
    /*! \brief This is get_n_orbits() if we don't store the bounds, otherwise, it is `get_orbit_repr_no_unknown().size()` */
    inf::QuovecIndex m_n_orbits_no_unknown;
    /*! \brief The representative of each orbit with no unknowns */
    std::vector<inf::Event> m_orbit_repr_no_unknown;

    /*! \brief Initializes the orbits depending on `m_store_bounds` and based on \p marginal_symmetries which represents \f$\marggroup\f$ */
    void init_orbits(inf::Symmetry::Group const &marginal_symmetries);

    /*! \brief Contains, e.g., `{ {(0,0,0),(1,1,1),(2,2,2),(3,3,3)}, {(0,0,1),(0,0,2),...}, ...}` but with the events hashed. */
    std::vector<std::vector<inf::EventTensor::EventHash>> m_quovec_index_to_orbit;
    /*! \brief Contains, e.g., {(0,0,0) -> 0, (1,1,1) -> 0, ..., (0,0,1) -> 1, (0,0,2) -> 1, ..} but with the events hashed. */
    std::vector<inf::QuovecIndex> m_event_to_quovec_index;
    /*! \brief Initializes `m_quovec_index_to_orbit`, `m_event_to_quovec_index`, `m_orbit_repr_no_unknown`, `m_n_orbits_no_unknown` */
    void init_quovec_index_maps();

    /*! \brief To set all the coefficients of an orbit to the same value, used in inf::DualVector::set_from_quovec()
     * \param quovec_index Identifies an orbit \f$o_{\infevent} = \act{\marggroup}{\events\infmarg}(\infevent)\f$
     * \param coeff The value to assign as the image of each \f$\infevent' \in o_{\infevent}\f$ */
    void set_orbit_coeff(inf::QuovecIndex quovec_index, Num coeff);

    /*! \brief This indicates whether the dual vector computes its lower or upper bounds, see inf::DualVector::BoundType
     * \details If `m_store_bounds == inf::DualVector::StoreBounds::no`, this is ignored. */
    inf::DualVector::BoundType m_bound_type;

    /*! \brief This type describes how to compute upper/lower bounds: `bound_rule.first` is the quovec index to set as the min/max of
     * the quovec elements indexed by `bound_rule.second`
     * \details In principle, we could simply say that the event `(0,?)` computes its bounds based on the min/max of the values assigned
     * to `(0,0)` and `(0,1)`. However, we can directly compute the bounds at the level of the quovec: the quovec index corresponding
     * to `(0,?)` (stored in `bound_rule.first`) looks at the quovec indices (basically, the orbits) of `(0,0)` and `(0,1)`. */
    typedef std::pair<QuovecIndex, std::vector<QuovecIndex>> BoundRule;
    /*! \brief The bound rules allowing to quickly compute the upper/lower bounds directly from the quovecs */
    std::vector<inf::DualVector::BoundRule> m_bound_rules;
    /*! \brief Initializes the rules for computing the upper/lower bounds.
     * \details We could implement more simplifications, but ok, this should not take long anyway. */
    void init_bound_rules();

    /*! \brief Returns min(a,b) or max(a,b) depending on m_bound_type */
    Num min_or_max(Num const a, Num const b) const;
};

} // namespace inf
