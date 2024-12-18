#pragma once

#include "../../util/rand.h"
#include "../events/event_tree.h"
#include "target_distr.h"

/*! \file */

/*! \defgroup inflation Inflation
    \brief Inflation-related problems and classes */

/*! \defgroup infback Inflation backend
 * \ingroup inflation
 * \brief The inflation code allowing the resolution of inflation problems
 * */

/*! \brief The namespace for inflation-related things */
namespace inf {

/*! \ingroup infback
    \brief Describes an inflation of a network, currently restricted to canonical inflations of the triangle network
    \details Recall that the class inf::Network currently only describes the triangle network, but paves the way for treating more general networks.
    The inf::Inflation class is similar: some of its methods are (almost) ready for more general inflations of more general networks,
    but currently, it only allow to describe canonical inflations of the triangle network.

    ### Description of the inflation

    We consider inflations indexed by an inflation size \f$\infsize = (\infsize_1,\infsize_2,\infsize_3) \in \N^3\f$,
    where \f$\infsize_\type\f$ denotes the number of copies of the source of type \f$\type\f$ (where \f$\type=0\f$ means the \f$\alpha\f$ source,
    \f$\type=1\f$ means the \f$\beta\f$ source, and \f$\type=2\f$ means the \f$\gamma\f$ source).
    The inflation parties are denoted with the type inf::Inflation::Party, and take the form \f$(\type,(j,k))\f$,
    where \f$\type\f$ denotes the type of the party (\f$\type = 0\f$ means Alice, \f$\type = 1\f$ means Bob, and \f$\type = 2\f$ means Charlie) and \f$j\f$ (\f$k\f$) is the index of the copy of the left (right) source that the party receives.

    For instance, in the case \f$\infsize = (2,2,2)\f$ (two copies of each source), the inflation graph looks as follows:

    \image html inf_graph_222.png width=350px

    ### Party ordering

    We need to pick an ordering of the inflation parties, i.e., a bijection \f$f : \{0,\dots,|\infparties|-1\} \to \infparties\f$,
    so that we can represent an inflation event \f$\infevent\in\infevents\f$ as a list \f$(\infevent(f(0)), \infevent(f(1)), \dots)\f$.
    A priori, any ordering would work. However, since inf::TreeOpt optimizes events using a tree search based on first filling the outcome
    of the party \f$f(0)\f$, then \f$f(1)\f$, etc, the order potentially has an impact on the runtime once we add the branch-and-bound
    mechanism (see inf::DualVector::StoreBounds).

    The ordering that we choose in inf::Inflation::init_parties() is thought (though, not tested) to be good because it enumerates the parties
    in a way where the volume of the cube (in the cube representation of triangle network strategies/inflation events) is attempted to be
    maximized early on.
    */

class Inflation : public util::Loggable {
  public:
    // Types

    /*! \brief Used for a list of source indices such as \f$(\alpha_0,\beta_1,\gamma_0)\f$, represented as `(0,1,0)` */
    typedef std::vector<Index> Sources;
    /*! \brief The inflation size (number of copies of each source), e.g., \f$\infsize = (2,2,2)\f$ */
    typedef std::vector<Index> Size;

    /*! \brief The inflation parties \f$\infparties\f$ are denoted with this type, and take the form \f$(\type,(j,k))\f$,
        where \f$\type\f$ denotes the type of the party
        (\f$\type = 0\f$ means Alice, \f$\type = 1\f$ means Bob, and \f$\type = 2\f$ means Charlie)
        and \f$j\f$ (\f$k\f$) is the index of the copy of the left (right) source that the party receives.
        \details For instance, `(0, (1,2))` means the Alice (index 0) connected to the 2nd (index 1) \f$\beta\f$ source,
        and connected to the 3rd (index 2) \f$\gamma\f$ source. */
    typedef std::pair<Index, inf::Inflation::Sources> Party;
    /*! \brief The inflation sources \f$\infsources\f$ are denoted with this type, and take the form \f$(\type,j)\f$,
     * where \f$\type\f$ denotes the type of the source (where \f$\type=0\f$ means the \f$\alpha\f$ source,
    \f$\type=1\f$ means the \f$\beta\f$ source, and \f$\type=2\f$ means the \f$\gamma\f$ source). */
    typedef std::pair<Index, Index> Source;

    typedef std::shared_ptr<const inf::Inflation> ConstPtr;

    /*! \brief Whether or not the distribution symmetries will be exploited to enlarge the inflation symmetry group
        \details This impacts both the columns and rows of the linear program matrix.
        In other words, this impacts the set of symmetrized events (obtained with inf::Inflation::get_symtree(), which is exploited by the inf::Optimizer),
        but also the symmetry structure that can be assumed for the inf::DualVector, which results in a lower-dimensional constraint space.
        More specifically, this option controls what the symmetry group \f$\distrgroup\f$ of the inflation will be:
        - If this is inf::Inflation::UseDistrSymmetries::no, then the inflation symmetry group will consist of just the source symmetries, i.e., \f$\sourcegroup\f$.
        - If this is inf::Inflation::UseDistrSymmetries::yes, then the inflation symmetry group will be given by \f$\distrgroup\f$ as defined in the paper (a subset of \f$\fullgroup = \sourcegroup\times\partygroup\times\outgroup\f$).

        It is highly recommended to always use inf::Inflation::UseDistrSymmetries::yes, since this allows to drastically improve the runtime of the resolution of the inflation problem without changing the mathematical problem that is being solved. */
    enum class UseDistrSymmetries {
        yes, ///< Much, much faster when inf::TargetDistr has usable symmetries
        no   ///< Mostly for testing purposes
    };

    static void log(inf::Inflation::UseDistrSymmetries use_distr_symmetries);

    /*! \brief Constructor with number of copies of each source
        \param target_distr Yields the inf::Network and the relevant symmetries. It is stored to keep track of its symmetries and
        to have an adequate name to write as metadata in the output file of the corresponding inf::EventTree (see inf::Inflation::get_symtree())
        \param size We consider inflations indexed by an inflation size \f$\infsize = (\infsize_1,\infsize_2,\infsize_3) \in \N^3\f$,
        where \f$\infsize_\type \geq 1\f$ denotes the number of copies of the source of type \f$\type\f$ (where \f$\type=0\f$ means the \f$\alpha\f$ source,
    \f$\type=1\f$ means the \f$\beta\f$ source, and \f$\type=2\f$ means the \f$\gamma\f$ source).
        \param use_distr_symmetries Whether or not the distribution symmetries will be incorporated in the inflation symmetry group. See inf::Inflation::UseDistrSymmetries for more details. */
    Inflation(inf::TargetDistr::ConstPtr const &target_distr,
              inf::Inflation::Size const &size,
              inf::Inflation::UseDistrSymmetries use_distr_symmetries = inf::Inflation::UseDistrSymmetries::yes);

    //! \cond
    Inflation(inf::Inflation const &other) = delete;
    Inflation(inf::Inflation &&other) = delete;
    inf::Inflation &operator=(inf::Inflation const &other) = delete;
    inf::Inflation &operator=(inf::Inflation &&other) = delete;
    //! \endcond

    // String stuff

    /*! \brief A string that summarizes the main information about the inf::Inflation, its symmetries and whether or not
     * these include the symmetries of an inf::TargetDistr, the name of the underlying inf::TargetDistr, etc.
     * \details This is used when writing the inf::EventTree to be sure to read from the correct file.
     * See inf::Inflation::get_symtree(). */
    std::string get_metadata() const;
    /*! \brief The filename to which the inf::EventTree is stored, including the inflation size and the short name of the inf::TargetDistr used for symmetry accelaration */
    std::string get_symtree_filename() const;
    void log() const override;
    /*! \brief To print inflation events \f$\infevent \in \infevents\f$ to `util::logger` */
    void log_event(inf::Event const &event) const;
    /*! \brief To convert the list-of-indices representation of an inflation marginal to a list of party names. */
    std::string get_marginal_name(std::vector<Index> const &marginal) const;
    /*! \brief To log an inflation marginal to `util::logger` */
    void log_marginal(std::vector<Index> const &marginal) const;
    /*! \brief Logs an event (whose values are expected to be single-character strings)
     * to the `util::logger` in a nice matrix-like format */
    void log_event_strings(std::vector<std::string> const &event) const;

    // Getters

    /*! \brief Number of parties \f$|\infparties|\f$ of the inflation */
    Index get_n_parties() const { return m_n_parties; }
    /*! \brief All zero event over the inflation parties */
    inf::Event get_all_zero_event() const;
    /*! \brief All unknown event over the inflation parties */
    inf::Event get_all_unknown_event() const;
    /*! \brief Returns a random event over the inflation parties
        \details The randomness is seeded at the time of creation of the inf::Inflation,
        and the instance then queries its own pseudorandom number generator. */
    inf::Event get_random_event() const;
    /*! \brief Event range over all inflation parties, to iterate over \f$\infparties\f$ */
    inf::EventRange get_event_range() const;
    /*! \brief The range of all copy indices of the form \f$(i_0,i_1,i_2)\f$, where \f$i_\type \in \ints{\infsize_\type}\f$,
     * where \f$\infsize = (\infsize_0,\infsize_1,\infsize_2)\f$ denotes the inflations size */
    util::ProductRange<Index> get_all_source_range() const;
    /*! \brief The list of explicit inf::Inflation::Party
     * \details This list allows to see the ordering that we choose on inflation parties,
     * which eventually generates the ordering of inflation events */
    std::vector<inf::Inflation::Party> get_parties() const;
    /*! \brief Internally, each explicit inf::Inflation::Party is assigned an index in \f$\{0,1,\dots,|\infparties|-1\}\f$ */
    Index get_party_index(inf::Inflation::Party const &party) const;
    /*! \brief Get e.g. "A00" -> 0, "A01" -> 1, etc.
     * \details Internally, each explicit inf::Inflation::Party is assigned an index in \f$\{0,1,\dots,|\infparties|-1\}\f$.
     * Furthermore, each inf::Inflation::Party is assigned a string of the form "Ajk", where "A" is obtained with
     * inf::Network::get_party_names(), and j and k are the copy indices of the party. */
    Index get_party_index(std::string const &party) const;
    /*! \brief Get e.g. 0 -> "A00", 1 -> "A01", etc.
     * \details Internally, each explicit inf::Inflation::Party is assigned an index in \f$\{0,1,\dots,|\infparties|-1\}\f$.
     * Furthermore, each inf::Inflation::Party is assigned a string of the form "Ajk", where "A" is obtained with
     * inf::Network::get_party_names(), and j and k are the copy indices of the party. */
    std::string const &get_party_name(Index party) const;
    /*! \brief Check that the given \p party name is of the form `Ajk`, where `A` must be obtained from the underlying inf::Network::get_party_names()
     * and j and k must be in the appropriate range related to inf::Inflation::get_size() (see also inf::Inflation::Party) */
    bool is_valid_party_name(std::string const &party) const;
    /*! \brief The inflation size \f$\infsize = (\infsize_0,\infsize_1,\infsize_2)\f$ specifying the number of copies of each source */
    inf::Inflation::Size const &get_size() const;
    /*! \brief Returns the underlying inf::Network */
    inf::Network::ConstPtr const &get_network() const;
    /*! \brief Returns the symmetries of the inflation, typically including the symmetries of the specified inf::TargetDistr
     * \details See inf::Inflation::UseDistrSymmetries for more details. */
    inf::Symmetry::Group const &get_inflation_symmetries() const;
    /*! \brief Returns `true` if the parameter \p d has the same symmetries as the original inf::TargetDistr that was used to intialize the inflation
     * \details If the inf::Inflation was initialized with inf::Inflation::UseDistrSymmetries::no, then this always return `true` */
    bool has_symmetries_compatible_with(inf::TargetDistr const &d) const;
    /*! \brief Returns the group \f$\sourcegroup\f$, i.e., all the party symmetries of the inflation that are induced by source permutations
        \return The reference is valid until `*this` is. */
    std::vector<inf::PartySym> const &get_source_induced_syms() const;

    /*! \brief This converts a source permutation into the corresponding party permutation
        \details This is non-static because it relies on inf::Inflation::get_party_index().
        \param perm_sources List of permutation for each source, i.e., `perm_sources[t]` should be a permutation of the
        sources of type `t`, so an element of \f$\shortsymgroup{\infsize_\type}\f$, where \f$\infsize = (\infsize_0,\infsize_1,\infsize_2)\f$
        denotes the inflation size. Each source permutations have to be represented as \f$(\sigma(0),\dots,\sigma(\infsize_\type-1))\f$,
        i.e., as the list of its images.
        \return The inf::PartySym referring to the parties of the inflation */
    inf::PartySym source_sym_to_party_sym(std::vector<std::vector<Index>> const &perm_sources) const;
    /*! \brief This converts (lifts) a network party permutation to the adequate inflation party permutation
        \param network_party_sym The inf::PartySym referring to the network parties (here, the parity obtained with inf::PartySym::is_even() is used)
        \return The inf::PartySym referring to the parties of the inflation */
    inf::PartySym network_party_to_inf_party_sym(inf::PartySym const &network_party_sym) const;

    /*! \brief An inf::EventTree representation of the symmetrized event representatives, denoted \f$\redinfevents \subset \infevents\f$
     * in the paper.
     * \details The set \f$\redinfevents\f$ depends on the symmetry group that is being used: this includes or does not include
     * the inf::TargetDistr (passed to the constructor) symmetries, see inf::Inflation::UseDistrSymmetries for more details.
     *
     * On the first call to this method, `m_symtree` is initialized and then cached.
     * How `m_symtree` is initialized depends on the parameter \p io: if \p io is inf::EventTree::IO::read,
     * then the inf::Inflation reads the inf::EventTree from the file inf::Inflation::get_symtree_filename().
     * Else, it computes the inf::EventTree using the class inf::TreeFiller.
     * If \p io is inf::EventTree::IO::write, the inf::Inflation furthermore saves the resulting inf::EventTree
     * to the file inf::Inflation::get_symtree_filename().
     *
     * In practice, the inf::EventTree::IO parameter will come from the `--symtree-io` command-line parameter,
     * see \ref CLIparams the "command-line parameter documentation" for more information. */
    inf::EventTree const &get_symtree(inf::EventTree::IO io) const;

    // For inflation constraints

    /*! \brief Returns `true` if the two lists of parties are d-separated
     * \details \p marg_1 and \p marg_2 must be valid lists of inflation parties,
     * i.e., their elements must be between 0 and `inf::Inflation::get_n_parties()-1`.
     * One can use the inf::Inflation::get_party_index() overloads to obtain these lists conveniently. */
    bool are_d_separated(std::vector<Index> const &marg_1, std::vector<Index> const &marg_2) const;

    /*! \brief Returns \f$\parents(\infmarg)\f$, the set of parent sources of the inflation marginal \f$\infmarg \subset \infparties\f$
     * \param marg Must be a valid list of inflation parties, i.e., its elements must be between 0 and `inf::Inflation::get_n_parties()-1`.
     * One can use the inf::Inflation::get_party_index() overloads to obtain such a list conveniently. */
    std::set<inf::Inflation::Source> get_parents(std::vector<Index> const &marg) const;

    /*! \brief Returns `true` if the list of party is an injectable set, denoted \f$\injsets\f$ in the paper
     * \param marg Must be a valid list of inflation parties, i.e., its elements must be between 0 and `inf::Inflation::get_n_parties()-1`.
     * One can use the inf::Inflation::get_party_index() overloads to obtain such a list conveniently. */
    bool is_injectable_set(std::vector<Index> const &marg) const;

  private:
    /*! \brief This inf::TargetDistr specifies the underlying inf::Network as well as the distribution symmetries (see inf::Inflation::UseDistrSymmetries) */
    inf::TargetDistr::ConstPtr const m_target_distr;
    /*! \brief The inflation size \f$\infsize = (\infsize_0,\infsize_1,\infsize_2)\f$ specifying the number of copies of each source */
    inf::Inflation::Size const m_size;
    /*! \brief Whether or not the distribution symmetries were incorporated in the inflation symmetry group, see inf::Inflation::UseDistrSymmetries */
    inf::Inflation::UseDistrSymmetries const m_use_distr_symmetries;

    /*! \brief The list of explicit inf::Inflation::Parties, this is the inverse of `m_party_map` */
    std::vector<inf::Inflation::Party> m_parties;
    /*! \brief This map is used in `inf::Inflation::get_party_index(inf::Inflation::Party const&) const` */
    std::map<inf::Inflation::Party, Index> m_party_map;
    /*! \brief This map is used in `inf::Inflation::get_party_index(std::string const&) const` */
    std::map<std::string, Index> m_name_map;
    /*! \brief This list is used in `inf::Inflation::get_party_name()` */
    std::vector<std::string> m_party_names;
    /*! \brief This initializes `m_parties`, `m_party_map`, `m_name_map`, `m_party_names` */
    void init_parties();

    /*! \brief The inflation party symmetries coming from source relabelings */
    std::vector<inf::PartySym> m_source_induced_syms;
    /*! \brief Initializes `m_source_induced_syms` */
    void init_source_induced_syms();

    /*! \brief The inflation symmetries, accounting for the target distribution's applicable symmetries and the source symmetries
     * \details See inf::Inflation::UseDistrSymmetries fore more details */
    inf::Symmetry::Group m_inflation_symmetries;
    /*! \brief Initializes `m_inflation_symmetries`, accounting for the target distribution's applicable symmetries and the source symmetries
        \param distribution_sym_group The symmetries of the target distribution to use (although, see inf::Inflation::UseDistrSymmetries) */
    void init_inflation_symmetries(inf::Symmetry::Group const &distribution_sym_group);
    /*! \brief Extract the symmetries of the distribution that are applicable to the inflation based on its size.
     * \details See the definition of \f$\partygroup\f$ in the paper.
        For instance, for the 2x2x2 inflation, all network party symmetries are applicable. But for the 2x3x4 inflation, no network party symmetries are applicable.
        \param distribution_sym_group The distribution symmetries (only the inf::PartySym part matters here)
        \param size The (possibly symmetry-breaking) size of the inflation, determining which network symmetries can be lifted to inflation symmetries
        \return The applicable symmetry group (implies a copy, but it's fairly few elements anyway) */
    static inf::Symmetry::Group get_applicable_symmetries(inf::Symmetry::Group const &distribution_sym_group,
                                                          inf::Inflation::Size const &size);
    /*! \brief The number \f$|\infparties|\f$ of parties of the inflation */
    Index m_n_parties;

    /*! \brief Stores the symmetrized inflation events as a tree, see inf::Inflation::get_symtree()
     * \details This is a `mutable` member because the caching mechanism requires initializing the event tree
     * on `const inf::Inflation` instances. */
    mutable inf::EventTree::UniquePtr m_symtree;

    /*! \brief Used for inf::Inflation::get_random_event()
        \details This is a `mutable` member since, conceptually, inf::Inflation::get_random_event() does not
        modify the inf::Inflation instance and hence ought to be a `const` method.
        However, this modifies the state of the random number generator. */
    mutable util::RNG<inf::Outcome> m_rng;
};

} // namespace inf
