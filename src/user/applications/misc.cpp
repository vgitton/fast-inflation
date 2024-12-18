#include "misc.h"

#include "../../util/debug.h"
#include "../../util/frac.h"
#include "../../util/logger.h"
// For the algo_unknown_test
#include "../../util/file_stream.h"
#include "../../util/math.h"
#include "../../util/misc.h"
#include "../../util/product_range.h"
#include <algorithm>
#include <fstream>
#include <iomanip> // For the Blended FW test
// ---
#include "../../inf/constraints/constraint_parser.h"
#include "../../inf/constraints/constraint_set.h"
#include "../../inf/constraints/dual_vector.h"
#include "../../inf/constraints/marginal.h"
#include "../../inf/events/event_tree.h"
#include "../../inf/events/tree_splitter.h"
#include "../../inf/frank_wolfe/fully_corrective_fw.h"
#include "../../inf/frank_wolfe/pairwise_fw.h"
#include "../../inf/inf_problem/inflation.h"
#include "../../inf/inf_problem/tree_filler.h"
#include "../../inf/optimization/optimizer.h"

// For marginal tests
#include "ejm.h"
#include "srb.h"

// #include<cstdlib>
#include <memory>

namespace user {

//! \cond ignore these internal functions
Num dummy_eval_dual_vector_on_event(
    inf::DualVector const &dual_vector,
    inf::Event const &e) {
    inf::Inflation const &infl = *dual_vector.get_inflation();

    ASSERT_EQUAL(infl.get_n_parties(), e.size())

    Index n_alpha = infl.get_size()[0];
    Index n_beta = infl.get_size()[1];
    Index n_gamma = infl.get_size()[2];

    Num alt_score = 0;
    for (Index alpha1 : util::Range(n_alpha)) {
        for (Index alpha2 : util::Range(alpha1 + 1, n_alpha)) {
            for (Index beta1 : util::Range(n_beta)) {
                for (Index beta2 : util::Range(n_beta)) {
                    if (beta1 == beta2)
                        continue;
                    for (Index gamma1 : util::Range(n_gamma)) {
                        for (Index gamma2 : util::Range(n_gamma)) {
                            if (gamma1 == gamma2)
                                continue;

                            alt_score += dual_vector.get_event_tensor().get_num({e[infl.get_party_index(
                                                                                     inf::Inflation::Party(0, {beta1, gamma1}))],
                                                                                 e[infl.get_party_index(
                                                                                     inf::Inflation::Party(1, {gamma1, alpha1}))],
                                                                                 e[infl.get_party_index(
                                                                                     inf::Inflation::Party(2, {alpha1, beta1}))],
                                                                                 e[infl.get_party_index(
                                                                                     inf::Inflation::Party(0, {beta2, gamma2}))],
                                                                                 e[infl.get_party_index(
                                                                                     inf::Inflation::Party(1, {gamma2, alpha2}))],
                                                                                 e[infl.get_party_index(
                                                                                     inf::Inflation::Party(2, {alpha2, beta2}))]});
                        }
                    }
                }
            }
        }
    }

    return alt_score;
}

Num dummy_eval_dual_vector_on_distribution(
    inf::DualVector const &dual_vector,
    inf::TargetDistr const &d) {
    Num score = 0;
    for (inf::Event const &e1 : d.get_event_range()) {
        for (inf::Event const &e2 : d.get_event_range()) {
            score += d.get_event_tensor().get_num(e1) * d.get_event_tensor().get_num(e2) * dual_vector.get_event_tensor().get_num({e1[0], e1[1], e1[2], e2[0], e2[1], e2[2]});
        }
    }
    return score;
}
//! \endcond

} // namespace user

void user::test_dual_vector_eval(inf::TargetDistr::ConstPtr const &d) {
    util::logger << *(d->get_network()) << util::cr << *d << util::cr;

    // pick random inflation order
    util::RNG<Index> inf_order_rng(2, 3);
    Index n_alpha = inf_order_rng.get_rand();
    Index n_beta = inf_order_rng.get_rand();
    Index n_gamma;
    // Order 3x3x3 is a bit slow for a test
    if (n_alpha == 3 and n_beta == 3) {
        n_gamma = 2;
    } else {
        n_gamma = inf_order_rng.get_rand();
    }

    Num lhs_denom = util::factorial(n_alpha) * util::factorial(n_beta) * util::factorial(n_gamma) / 2;
    Num rhs_denom = util::pow(d->get_event_tensor().get_denom(), Num(2));
    Num gcd = std::gcd(lhs_denom, rhs_denom);
    lhs_denom /= gcd;
    rhs_denom /= gcd;

    // Inflation
    inf::Inflation::ConstPtr inflation = std::make_shared<const inf::Inflation>(
        d, inf::Inflation::Size({n_alpha, n_beta, n_gamma}));
    util::logger << *inflation << util::cr;

    // LPI constraint
    inf::ConstraintSet constraints(inflation, {{"A00,B00,C00", "A11,B11,C11", ""}}, inf::DualVector::StoreBounds::yes);
    constraints.set_target_distribution(*d);

    // setup some dummy coeffs
    inf::Quovec dual_vector_quovec(constraints.get_quovec_size());
    util::RNG<Num> dual_vector_coeff_rng(-10, 10);
    for (Num &coeff : dual_vector_quovec) {
        coeff = dual_vector_coeff_rng.get_rand();
    }
    constraints.set_dual_vector_from_quovec(dual_vector_quovec);
    constraints.log_dual_vectors();

    // Inflation event
    LOG_BEGIN_SECTION("Example inflation event")
    inf::Event e = inflation->get_random_event();
    inflation->log_event(e);
    LOG_END_SECTION
    util::logger << util::cr;

    // Evaluation

    inf::Marginal::EvaluatorSet evaluators = constraints.get_marg_evaluators();
    for (Index i : util::Range(e.size())) {
        evaluators.set_outcome(i, e[i]);
    }
    Num const event_score = evaluators.evaluate_dual_vector();

    LOG_BEGIN_SECTION("LPI constraint evaluation")

    util::logger
        << "Using inf::Marginal::EvaluatorSet::evaluate_dual_vector(): " << util::cr
        << "\t" << event_score << util::cr;

    // Just to extract the inf::DualVector, a bit dirty but otherwise we need to expose too much of the internals of inf::ConstraintSet
    inf::Constraint first_constraint(inflation, {"A00,B00,C00", "A11,B11,C11", {}}, inf::DualVector::StoreBounds::no);
    first_constraint.set_target_distribution(*d);
    first_constraint.set_dual_vector_from_quovec(dual_vector_quovec, 0);

    Num event_score_dummy =
        rhs_denom * user::dummy_eval_dual_vector_on_event(first_constraint.get_dual_vector(), e) - lhs_denom * user::dummy_eval_dual_vector_on_distribution(first_constraint.get_dual_vector(), *d);

    // util::logger << "rhs_denom * dual_vector_on_event = " << rhs_denom * user::dummy_eval_dual_vector_on_event(first_constraint->get_dual_vector(), e) << util::cr;
    // util::logger << "lhs_denom * dual_vector_on_d     = " << lhs_denom * user::dummy_eval_dual_vector_on_distribution(first_constraint->get_dual_vector(), *d) << util::cr;

    util::logger
        << "Using an explicit summation: " << util::cr
        << "\t" << event_score_dummy << util::cr;
    HARD_ASSERT_EQUAL(event_score_dummy, event_score)

    Num event_score_quovec_num = util::inner_product(
        constraints.get_inflation_event_quovec(e),
        dual_vector_quovec);
    Num event_score_quovec = event_score_quovec_num;

    util::logger
        << "Using the quovec representation" << util::cr
        << util::begin_section << constraints.get_inflation_event_quovec(e) << util::cr << util::end_section
        << "of the event:" << util::cr
        << "\t" << event_score_quovec << util::cr;
    HARD_ASSERT_EQUAL(event_score, event_score_quovec)

    util::logger << util::cr;

    LOG_END_SECTION

    util::logger << util::cr;
}

void user::print_event_latex_format(inf::Inflation const &inflation, inf::Event const &event) {
    std::vector<std::string> const names = {"alice", "bob", "charlie"};
    for (Index const party : util::Range<Index>(3)) {
        util::logger << "\\setinfevent" << names[party];
        for (Index const j : util::Range<Index>(2)) {
            for (Index const k : util::Range<Index>(2)) {
                util::logger << "{" << static_cast<int>(event[inflation.get_party_index(inf::Inflation::Party(party, {j, k}))]) << "}";
            }
        }
        util::logger << util::cr;
    }
    util::logger << "\\infeventexpl," << util::cr;
}

void user::frac::run() {
    util::logger << "Testing util::Frac comparisons..." << util::cr;

    util::Frac x(5, 3), y(7, 4), z(-4), zz(-20, 5);

    // x & y
    HARD_ASSERT_TRUE(x < y)
    HARD_ASSERT_TRUE(x <= y)
    HARD_ASSERT_TRUE(y > x)
    HARD_ASSERT_TRUE(y >= x)
    HARD_ASSERT_TRUE((not(x == y)))
    HARD_ASSERT_TRUE((not(y == x)))

    // z & zz
    HARD_ASSERT_TRUE(z == zz)
    HARD_ASSERT_TRUE(zz == z)
    util::logger << z << " is equal to " << zz << util::cr << util::cr;

    // x & z
    HARD_ASSERT_TRUE(z < x)
}

void user::event_sym::run() {
    inf::TargetDistr::ConstPtr const ejm = user::get_ejm_distribution();
    inf::Inflation const inflation(ejm, inf::Inflation::Size{2, 2, 2});
    inf::Event event(inflation.get_n_parties());

    // Alice: \setinfeventalice{0}{0}{1}{2}
    event[inflation.get_party_index({0, {0, 0}})] = 0;
    event[inflation.get_party_index({0, {0, 1}})] = 0;
    event[inflation.get_party_index({0, {1, 0}})] = 1;
    event[inflation.get_party_index({0, {1, 1}})] = 2;
    // Bob: \setinfeventbob{1}{1}{2}{3}
    event[inflation.get_party_index({1, {0, 0}})] = 1;
    event[inflation.get_party_index({1, {0, 1}})] = 1;
    event[inflation.get_party_index({1, {1, 0}})] = 2;
    event[inflation.get_party_index({1, {1, 1}})] = 3;
    // Charlie: \setinfeventcharlie{0}{1}{2}{3}
    event[inflation.get_party_index({2, {0, 0}})] = 0;
    event[inflation.get_party_index({2, {0, 1}})] = 1;
    event[inflation.get_party_index({2, {1, 0}})] = 2;
    event[inflation.get_party_index({2, {1, 1}})] = 3;

    inf::Event const input_event = event;

    util::logger << "Input event: " << util::cr;
    inflation.log_event(event);
    user::print_event_latex_format(inflation, event);

    // Outcome sym = (01)
    inf::OutcomeSym const outcome_sym({1, 0, 2, 3});
    util::logger << "Applying the outcome symmetry " << outcome_sym
                 << " (exchange outcomes " << inflation.get_network()->outcome_to_str(0)
                 << " and " << inflation.get_network()->outcome_to_str(1)
                 << ") yields:" << util::cr;
    outcome_sym.act_on_event(event, event);
    inflation.log_event(event);
    user::print_event_latex_format(inflation, event);

    // Party sym = (01)
    inf::PartySym const party_sym({1, 0, 2}, false);
    util::logger << "Applying the party symmetry " << party_sym << " (exchange Alice and Bob) yields:" << util::cr;
    inf::PartySym const party_sym_action = inflation.network_party_to_inf_party_sym(party_sym);
    {
        inf::Event transformed_event(inflation.get_n_parties());
        party_sym_action.act_on_list(event, transformed_event);
        event = transformed_event;
    }
    inflation.log_event(event);
    user::print_event_latex_format(inflation, event);

    // Source sym = ( (01), (01), id)
    std::vector<std::vector<Index>> const source_sym = {{1, 0}, {1, 0}, {0, 1}};
    util::logger << "Applying the source symmetry " << source_sym << " (exchange alpha0 and alpha1, exchange beta0 and beta1) yields:" << util::cr;
    inf::PartySym const source_sym_action = inflation.source_sym_to_party_sym(source_sym);
    {
        inf::Event transformed_event(inflation.get_n_parties());
        source_sym_action.act_on_list(event, transformed_event);
        event = transformed_event;
    }
    inflation.log_event(event);
    user::print_event_latex_format(inflation, event);

    util::logger << "Alternatively, we can directly act on the input event with the inflation symmetry (source,party,outcome):" << util::cr;
    inf::Symmetry const full_sym(source_sym_action.get_composition_after(party_sym_action), outcome_sym);
    inf::Event other_event(inflation.get_n_parties());
    full_sym.act_on_event(input_event, other_event);
    inflation.log_event(other_event);
    user::print_event_latex_format(inflation, other_event);

    HARD_ASSERT_EQUAL(event, other_event)

    util::logger << util::cr;
}

namespace user {

//! \cond ignore these internal functions
void test_solve_fcfw(inf::FullyCorrectiveFW &fw, double expected_s) {
    double precision = 1e-7;

    inf::FrankWolfe::Solution sol = fw.solve();

    HARD_ASSERT_LT(std::abs(sol.s - expected_s), precision);

    util::logger << util::cr;
}
//! \endcond

} // namespace user

void user::fully_corrective_fw::run() {
    Index const row_dimension = 3;

    inf::FullyCorrectiveFW fw(row_dimension, get_inf_cli().get_n_threads());

    util::logger << "Appending row" << util::cr;
    fw.memorize_event_and_quovec(inf::Event{}, std::vector<Num>({4, 0, 0}), 4.0);
    util::logger << "Test solve" << util::cr;
    user::test_solve_fcfw(fw, 1.0);

    util::logger << "Appending row" << util::cr;
    fw.memorize_event_and_quovec({}, {0, 4, 0}, 4.0);
    util::logger << "Test solve" << util::cr;
    user::test_solve_fcfw(fw, 0.707106781);

    util::logger << "Appending row" << util::cr;
    fw.memorize_event_and_quovec({}, {0, 0, 4}, 4.0);
    util::logger << "Test solve" << util::cr;
    user::test_solve_fcfw(fw, 0.577350269);

    fw.memorize_event_and_quovec({}, {2, 2, 0}, 4.0);
    fw.memorize_event_and_quovec({}, {2, 2, 0}, 4.0);
    fw.memorize_event_and_quovec({}, {2, 0, 2}, 4.0);
    fw.memorize_event_and_quovec({}, {0, 2, 2}, 4.0);
    fw.memorize_event_and_quovec({}, {2, 1, 1}, 4.0);
    fw.memorize_event_and_quovec({}, {1, 2, 1}, 4.0);
    fw.memorize_event_and_quovec({}, {1, 1, 2}, 4.0);

    user::test_solve_fcfw(fw, 0.577350269);

    // Extra test related to minimizing the distance

    inf::FullyCorrectiveFW other_fw(2, get_inf_cli().get_n_threads());
    other_fw.memorize_event_and_quovec({}, {1, 0}, 1.0);
    other_fw.memorize_event_and_quovec({}, {-1, 2}, 1.0);
    user::test_solve_fcfw(other_fw, 0.707106781);
}

namespace util {

template <typename T>
void log_mathematica(T const &t) {
    std::cout << std::fixed << std::setprecision(6) << t;
}
// Need to forward declare the specialization for it to be accessible in there
template <typename T>
void log_mathematica(std::vector<T> const &list);
template <typename T, typename U, typename V>
void log_mathematica(std::tuple<T, U, V> const &tuple) {
    std::cout << "{";
    util::log_mathematica(std::get<0>(tuple));
    std::cout << ", ";
    util::log_mathematica(std::get<1>(tuple));
    std::cout << ", ";
    util::log_mathematica(std::get<2>(tuple));
    std::cout << "}";
}
template <typename T>
void log_mathematica(std::vector<T> const &list) {
    std::cout << "{";
    bool first = true;
    for (T const &item : list) {
        if (not first)
            std::cout << ", ";
        first = false;
        util::log_mathematica(item);
    }
    std::cout << "}";
}

template <typename T>
void log_mathematica(std::string const &name, T const &value) {
    std::cout << name << " = ";
    util::log_mathematica(value);
    std::cout << ";\n";
}

} // namespace util

void user::pairwise_fw::run() {
    Index const dimension = 2;

    inf::PairwiseFW blender(dimension);
    blender.set_store_iterates(true);

    std::vector<std::vector<Num>> vertices = {
        // {-2,  0},
        // {+2,  0},
        // {-1,  1},
        // {+1,  1}
        {-3, 3},
        {3, 3},
        {1, 2},
        {-2, 1},
        {0, 1},
        {3, 1},
        {-3, -1},
        {-1, -1},
        {2, -1},
        {0, 4}};

    std::vector<Num> initial_vertex = vertices.back();

    blender.memorize_event_and_quovec({}, initial_vertex, 1.0);

    Index iter = 0;
    while (true) {
        ++iter;
        if (iter == 101) {
            util::logger << "Reached the max number of iterations. Exiting." << util::cr;
            break;
        }

        inf::FrankWolfe::Solution sol = blender.solve();
        // util::logger << sol;

        if (not sol.valid) {
            util::logger << "We got close to zero! Exiting." << util::cr;
            break;
        }

        std::vector<Num> const *fw_vertex = nullptr;
        for (std::vector<Num> const &vertex : vertices) {
            std::vector<double> vertex_double(dimension);
            for (Index i : util::Range(dimension)) {
                vertex_double[i] = static_cast<double>(1000000 * vertex[i]);
            }

            if (util::inner_product(vertex_double, sol.vec) < 0) {
                fw_vertex = &vertex;
                break;
            }
        }

        if (fw_vertex == nullptr) {
            util::logger << "Found a separating hyperplane! Exiting." << util::cr;
            break;
        }

        blender.memorize_event_and_quovec({}, *fw_vertex, 1.0);
    }

    util::log_mathematica("vertices", vertices);
    util::log_mathematica("iterates", blender.get_iterates());
}

void user::event_tensor::run() {
    inf::EventTensor t(3, 4);

    t.get_num({0, 0, 0}) = 6;
    t.get_num({1, 1, 0}) = 15;
    t.get_num({1, 0, 3}) = 30;

    t.set_denom(60);

    inf::EventTensor t_simplified(3, 4);
    t_simplified = t;
    t_simplified.simplify();

    for (inf::Event const &e : util::ProductRange(3, inf::Outcome(4))) {
        util::logger << e << " : t -> " << t.get_frac(e)
                     << ", t_simplified -> " << t_simplified.get_frac(e) << util::cr;

        HARD_ASSERT_TRUE(t.get_frac(e) == t_simplified.get_frac(e))
    }

    util::logger << util::cr;
}

void user::event_tree::run() {
    // Example of the documentation
    {
        util::logger << "The tree described in the documentation is as follows:" << util::cr;
        // Create a tree with depth 3
        inf::EventTree tree(3);
        // Insert some nodes
        tree.insert_node(0, 0, {tree.insert_node(1, 0, {tree.insert_node(2, 0, {}), tree.insert_node(2, 1, {})}), tree.insert_node(1, 1, {tree.insert_node(2, 0, {})})});
        tree.insert_node(0, 1, {tree.insert_node(1, 0, {tree.insert_node(2, 0, {}), tree.insert_node(2, 1, {})}), tree.insert_node(1, 1, {tree.insert_node(2, 1, {})})});
        // Important: do not forget to call this method after setting up the inf::EventTree!
        // There are soft assertions that check that this method was indeed called, though.
        tree.finish_initialization();
        tree.log_flat();
        util::logger << tree;
        tree.log_info();
    }

    // Testing the IO of inf::EventTree::Node
    {
        inf::EventTree::Node node(2, {42, 53});
        std::string const filename = "data/test_nodes";

        {
            util::OutputFileStream stream(filename, util::FileStream::Format::text, "Test metadata");
            stream.io(node);
        }

        util::logger << "node: ";
        util::logger << node.outcome << " -> {";
        for (Index i : util::Range(node.n_children))
            util::logger << node.children[i] << " ";
        util::logger << "}" << util::cr;

        inf::EventTree::Node other_node(0, {});

        {
            util::InputFileStream stream(filename, util::FileStream::Format::text, "Test metadata");
            stream.io(other_node);
        }

        util::logger << "other_node: " << other_node.outcome << " -> {";
        for (Index i : util::Range(other_node.n_children))
            util::logger << other_node.children[i] << " ";
        util::logger << "}" << util::cr;

        if (node == other_node) {
            util::logger << "The two nodes are equal" << util::cr;
        } else {
            util::logger << "The two nodes are not equal" << util::cr;
        }
    }

    Index const depth = 3;

    // Create a dummy tree
    inf::EventTree tree(depth);

    tree.insert_node(0, 0, {tree.insert_node(1, 1, {tree.insert_node(2, 2, {}), tree.insert_node(2, 3, {})}), tree.insert_node(1, 1, {tree.insert_node(2, 2, {}), tree.insert_node(2, 3, {})})});
    tree.insert_node(0, 1, {tree.insert_node(1, 1, {tree.insert_node(2, 1, {}), tree.insert_node(2, 1, {})})});
    tree.insert_node(0, 3, {tree.insert_node(1, 3, {tree.insert_node(2, 3, {})})});

    tree.finish_initialization();

    tree.log_flat();
    util::logger << tree;
    tree.log_info();

    HARD_ASSERT_EQUAL(tree.get_n_leaves(), 7)

    // Testing the inf::EventTree io
    {
        std::string const filename = "data/test_event_tree";

        {
            util::OutputFileStream stream(filename, util::FileStream::Format::text, "metadata");
            stream.io(tree);
        }

        inf::EventTree other_tree(depth);

        {
            util::InputFileStream stream(filename, util::FileStream::Format::text, "metadata");
            stream.io(other_tree);
        }

        util::logger << util::cr << "The tree was saved to disk and read back, giving:" << util::cr;

        other_tree.log_flat();
        util::logger << other_tree;
        other_tree.log_info();

        util::logger << util::cr;

        if (tree != other_tree) {
            THROW_ERROR("The two trees are not equal")
        } else {
            util::logger << "The two trees are equal." << util::cr;
        }
    }

    util::logger << util::cr;
}

void user::inf_tree_filler::run() {
    std::vector<inf::TargetDistr::ConstPtr> distributions;
    distributions.push_back(user::get_ejm_distribution());
    // distributions.push_back(distributions.back());
    distributions.push_back(user::get_noisy_srb(85, 100));
    distributions.push_back(distributions.back());
    distributions.push_back(distributions.back());
    // distributions.push_back(distributions.back());

    // Omitting some tests that are a bit slow to run in debug mode

    std::vector<inf::Inflation::Size> inf_sizes{
        {2, 2, 2},
        // {2,2,3},
        {2, 2, 2},
        {2, 2, 3},
        {2, 3, 3},
        // {3,3,3}
    };
    std::vector<Index> expected_n_leaves{
        15418,
        // 3791705,
        76,
        961,
        9400,
        // 61872
    };

    for (Index i : util::Range(inf_sizes.size())) {
        inf::Inflation::ConstPtr inflation(std::make_shared<inf::Inflation>(
            distributions[i], inf_sizes[i], inf::Inflation::UseDistrSymmetries::yes));

        inf::EventTree const &symtree = inflation->get_symtree(inf::EventTree::IO::none);

        symtree.log_info();

        HARD_ASSERT_EQUAL(symtree.get_n_leaves(), expected_n_leaves[i])
    }

    util::logger << util::cr;
}

void user::distribution_marginal::run() {
    std::vector<inf::TargetDistr::MarginalName> marginal_names = {
        {"A", "B", "C"},
        {"A", "B"},
        {"A", "C"},
        {"B", "C"},
        {"A"},
        {"B"},
        {"C"}};

    std::vector<inf::TargetDistr::ConstPtr> distributions = {
        user::get_ejm_distribution(),
        user::get_noisy_srb(9, 10)};

    for (inf::TargetDistr::ConstPtr d : distributions) {
        util::logger << *d << util::cr;

        for (inf::TargetDistr::MarginalName const &name : marginal_names) {
            inf::EventTensor const &marginal = d->get_marginal(name);

            util::logger << "The " << name << " marginal is:" << util::cr;
            marginal.log("p");
            util::logger << util::cr;
        }
    }
}

void user::inflation_marginal::run() {
    inf::TargetDistr::ConstPtr d = user::get_ejm_distribution();

    std::map<std::string, Num> denoms;
    denoms["A00, A11"] = 2 * 3;
    denoms["A00, A01"] = 4 * 3;
    denoms["A00, B00"] = 8 * 3;
    denoms["A00, B00, C01"] = 8 * 3; // Exchaning Alice and Charlie and swapping a source is still a symmetry of the marginal
    denoms["A00, B00, A11, B11, C11"] = 8 * 3;
    denoms["A00, B00, C00, A11, B11, C11"] = 4;

    {
        inf::Inflation::ConstPtr inflation = std::make_shared<inf::Inflation>(d, inf::Inflation::Size{2, 2, 2}, inf::Inflation::UseDistrSymmetries::yes);
        for (auto const &pair : denoms) {
            std::string const &marginal_name = pair.first;
            Num const denom = pair.second;

            std::vector<Index> const marginal_parties = inf::ConstraintParser::parse_inflation_marginal(*inflation, marginal_name);
            inf::Marginal marginal(inflation, marginal_parties, inf::Constraint::get_constraint_group(*inflation, marginal_parties, {}), inf::DualVector::StoreBounds::no);

            util::logger << marginal << util::cr;

            HARD_ASSERT_EQUAL(marginal.get_denom(), denom)
        }
    }

    {
        inf::Inflation::ConstPtr larger_inflation = std::make_shared<inf::Inflation>(d, inf::Inflation::Size{2, 2, 3}, inf::Inflation::UseDistrSymmetries::yes);

        std::vector<Index> const marginal_parties = inf::ConstraintParser::parse_inflation_marginal(*larger_inflation, "A00");
        inf::Marginal marginal(larger_inflation, marginal_parties, inf::Constraint::get_constraint_group(*larger_inflation, marginal_parties, {}), inf::DualVector::StoreBounds::no);

        util::logger << " ===================== " << util::cr;

        util::logger << marginal << util::cr;
    }
}

void user::dual_vector_bounds::run() {
    inf::TargetDistr::ConstPtr d = user::get_ejm_distribution();
    inf::Inflation::ConstPtr inflation = std::make_shared<inf::Inflation>(d, inf::Inflation::Size({2, 2, 2}));

    // Also tested the bound_rules for q(A00,B00,C00,A11)
    std::vector<Index> const marginal_parties = inf::ConstraintParser::parse_inflation_marginal(*inflation, "A00,B00,C00");
    inf::Marginal marginal(inflation,
                           marginal_parties,
                           inf::Constraint::get_constraint_group(*inflation, marginal_parties, {}),
                           inf::DualVector::StoreBounds::yes);

    inf::DualVector dual_vector(marginal, inf::DualVector::BoundType::lower);
    dual_vector.log_orbits(false);

    inf::Quovec quovec(dual_vector.get_n_orbits_no_unknown());
    for (Index i : util::Range(dual_vector.get_n_orbits_no_unknown())) {
        quovec[i] = -10 + i;
    }

    dual_vector.set_from_quovec(quovec, 0);
    dual_vector.log();

    // Orbits are 000, 001, 012, 00?, 01?, 0??, ???
    HARD_ASSERT_EQUAL(dual_vector.get_n_orbits_with_unknown(), 7)
}

void user::dual_vector_io::run() {
    inf::TargetDistr::ConstPtr d = user::get_ejm_distribution();

    // Inflation
    inf::Inflation::Size const inf_size = {2, 2, 2};
    inf::Inflation::ConstPtr inflation = std::make_shared<const inf::Inflation>(d, inf_size);
    util::logger << *inflation << util::cr;

    std::vector<Index> const marginal_parties = inf::ConstraintParser::parse_inflation_marginal(*inflation, "A00,B00,C00,A11,B11,C11");
    inf::Marginal marginal(inflation,
                           marginal_parties,
                           inf::Constraint::get_constraint_group(*inflation, marginal_parties, {}),
                           inf::DualVector::StoreBounds::yes);

    inf::DualVector dual_vector(marginal, inf::DualVector::BoundType::lower);

    inf::Quovec quovec(dual_vector.get_n_orbits_no_unknown());
    for (Index const i : util::Range(dual_vector.get_n_orbits_no_unknown())) {
        quovec[i] = -10 + i;
    }
    dual_vector.set_from_quovec(quovec, 0);
    util::logger << dual_vector;

    util::logger << util::cr << "Writing the dual vector to disk..." << util::cr;
    {
        util::OutputFileStream ofs("data/test_dual_vector_io", util::FileStream::Format::text, "METADATA");
        ofs.io(dual_vector);
        // We need to delete the ofs object to properly write to the file on disk because of buffering etc
    }

    inf::DualVector other_dual_vector(marginal, inf::DualVector::BoundType::lower);
    util::logger << util::cr << "Reading the dual vector from disk..." << util::cr;
    {
        util::InputFileStream ifs("data/test_dual_vector_io", util::FileStream::Format::text, "METADATA");
        ifs.io(other_dual_vector);
    }

    util::logger << other_dual_vector;

    for (Index const i : util::Range(dual_vector.get_event_tensor().get_hash_range()))
        HARD_ASSERT_EQUAL(dual_vector.get_event_tensor().get_num(i), other_dual_vector.get_event_tensor().get_num(i))
}

void user::opt_bounds::run() {
    inf::TargetDistr::ConstPtr d = user::get_noisy_srb(1, 2);

    // Inflation
    inf::Inflation::Size const inf_size = {2, 2, 2};
    inf::Inflation::ConstPtr inflation = std::make_shared<const inf::Inflation>(d, inf_size);
    util::logger << *inflation << util::cr;

    inf::Optimizer::SearchMode const search_mode = inf::Optimizer::SearchMode::tree_search;

    std::vector<inf::DualVector::StoreBounds> store_bound_modes = {
        inf::DualVector::StoreBounds::yes,
        inf::DualVector::StoreBounds::no,
    };

    inf::Quovec dual_vector_quovec;

    std::unique_ptr<Num> previous_score = nullptr;

    for (inf::DualVector::StoreBounds store_bounds : store_bound_modes) {
        inf::DualVector::log(store_bounds);
        util::logger << util::cr << util::cr;

        // LPI constraint
        inf::ConstraintSet::Ptr constraints = std::make_shared<inf::ConstraintSet>(inflation,
                                                                                   inf::ConstraintSet::Description{{"A00,B00,C00", "A11,B11,C11", {}}},
                                                                                   store_bounds);

        constraints->set_target_distribution(*d);

        inf::Optimizer::Ptr optimizer = inf::Optimizer::get_optimizer(search_mode, constraints, inf::EventTree::IO::none, get_feas_options()->get_n_threads());

        // This mechanism sets the dual_vector_quovec during the first pass, but leaves it intact in the second pass so that we can compare
        // the optimization of the same dual_vector
        if (dual_vector_quovec.size() == 0) {
            dual_vector_quovec = inf::Quovec(constraints->get_quovec_size());
            util::RNG<Num> dual_vector_coeff_rng(-10, 10);
            // Setup some dummy coeffs
            for (Num &coeff : dual_vector_quovec)
                coeff = dual_vector_coeff_rng.get_rand();
        }

        constraints->set_dual_vector_from_quovec(dual_vector_quovec);

        inf::Optimizer::Solution sol = optimizer->optimize(inf::Optimizer::StopMode::opt);

        if (previous_score != nullptr) {
            HARD_ASSERT_EQUAL(*previous_score, sol.get_inflation_event_score())
            util::logger << "The inner products are equal irrespective of whether or one stores the dual bounds." << util::cr;
        }

        previous_score = std::make_unique<Num>(sol.get_inflation_event_score());

        util::logger << util::cr;
    }
}

void user::opt_time::run() {
    inf::TargetDistr::ConstPtr d = user::get_ejm_distribution();

    // Setup
    inf::Optimizer::SearchMode const search_mode = inf::Optimizer::SearchMode::tree_search;
    inf::DualVector::StoreBounds const store_bounds = inf::DualVector::StoreBounds::yes;
    util::logger << "Timing an inf::Optimizer initialized with ";
    inf::Optimizer::log(search_mode);
    util::logger << " and ";
    inf::DualVector::log(store_bounds);
    util::logger << util::cr;

    // Inflation
    inf::Inflation::Size inf_size = {2, 2, 3};
    inf::Inflation::ConstPtr inflation = std::make_shared<const inf::Inflation>(d, inf_size);
    util::logger << *inflation << util::cr;

    // LPI constraint
    inf::ConstraintSet::Ptr constraints = std::make_shared<inf::ConstraintSet>(
        inflation,
        inf::ConstraintSet::Description{{"A00,B00,C00", "A11,A12,B11,B21,C11"}},
        store_bounds);
    constraints->set_target_distribution(*d);

    // Optimizer
    inf::Optimizer::Ptr optimizer = inf::Optimizer::get_optimizer(search_mode, constraints, get_feas_options()->get_symtree_io(), get_feas_options()->get_n_threads());

    // Now we time!
    Index n_runs = 20;
    util::Chrono chrono(util::Chrono::State::paused);
    for (Index time_i : util::Range(n_runs)) {
        (void)time_i;

        inf::Quovec dual_vector_quovec(constraints->get_quovec_size());
        util::RNG<Num> dual_vector_coeff_rng(-10, 10);
        // Setup some dummy coeffs
        for (Num &coeff : dual_vector_quovec) {
            coeff = dual_vector_coeff_rng.get_rand();
        }
        constraints->set_dual_vector_from_quovec(dual_vector_quovec);

        chrono.start();
        inf::Optimizer::Solution sol = optimizer->optimize(inf::Optimizer::StopMode::opt);
        (void)sol;
        chrono.pause();
    }

    double s_per_call = chrono.get_seconds() / static_cast<double>(n_runs);
    LOG_VARIABLE(s_per_call)
    double s_per_call_per_leaf = s_per_call / static_cast<double>(inflation->get_symtree(inf::EventTree::IO::none).get_n_leaves());
    LOG_VARIABLE(s_per_call_per_leaf)
}

namespace util {

class TestWritableClass : public util::Serializable, public util::Loggable {
  public:
    TestWritableClass(std::vector<std::vector<Index>> const &A) : m_A(A) {}

    void io(util::FileStream &stream) override {
        util::logger << "In TestWritableClass::io()..." << util::cr;
        stream.io(m_A);
        Index const expected_control = 33;
        stream.write_or_read_and_hard_assert(expected_control);
    }

    void log() const override {
        util::logger << "I'm a TestWritableClass with m_A = " << m_A << util::cr;
    }

    std::vector<std::vector<Index>> const &get_A() const { return m_A; }

  private:
    std::vector<std::vector<Index>> m_A;
};

} // namespace util

void user::file_stream::run() {
    std::string const s = "data/test_file_stream";
    std::vector<std::vector<Index>> test_matrix = {{16, 17}, {18, 19, 20}, {21, 22, 23, 24}};

    std::vector<util::FileStream::Format> formats = {
        util::FileStream::Format::binary,
        util::FileStream::Format::text,
    };

    for (util::FileStream::Format const format : formats) {
        {
            util::OutputFileStream stream(s, format, "METADATA");
            util::TestWritableClass obj(test_matrix);
            util::logger << obj;
            util::logger << "Writing..." << util::cr;
            // This purposedly doesn't compile:
            // stream.io(const_cast<util::TestWritableClass const&>(obj));
            stream.io(obj);
        }

        {
            util::InputFileStream stream(s, format, "METADATA");
            util::TestWritableClass obj({});
            util::logger << "Reading..." << util::cr;
            stream.io(obj);
            util::logger << obj;

            HARD_ASSERT_EQUAL(obj.get_A(), test_matrix)
        }
    }

    util::logger << util::cr;
}

void user::tree_splitter::run() {
    // The example of the documentation
    inf::EventTree tree(3);

    tree.insert_node(0, 0, {
                               tree.insert_node(1, 0, {
                                                          tree.insert_node(2, 0, {}),
                                                      }),
                           });
    tree.insert_node(0, 1, {
                               tree.insert_node(1, 0, {
                                                          tree.insert_node(2, 0, {}),
                                                      }),
                               tree.insert_node(1, 1, {
                                                          tree.insert_node(2, 0, {}),
                                                          tree.insert_node(2, 1, {}),
                                                      }),
                           });
    tree.finish_initialization();

    tree.log_flat();
    util::logger << tree << util::cr;
    Index n_splits = 2;
    inf::TreeSplitter::PathPartitionConstPtr path_partition = inf::TreeSplitter::get_path_partition(tree, n_splits);
    LOG_VARIABLE(*path_partition)
    HARD_ASSERT_EQUAL((*path_partition)[0].size(), 2)
    HARD_ASSERT_EQUAL((*path_partition)[1].size(), 1)
}

void user::inf_tree_splitting::run() {
    inf::Inflation inflation(user::get_ejm_distribution(), {2, 2, 2});

    inf::EventTree const &symtree = inflation.get_symtree(get_feas_options()->get_symtree_io());

    symtree.log_info();

    Index const n_threads = get_feas_options()->get_n_threads();

    if (n_threads == 1) {
        util::logger << "Nothing to do for 1 thread." << util::cr;
        return;
    }

    inf::TreeSplitter::PathPartitionConstPtr path_partition = inf::TreeSplitter::get_path_partition(symtree, n_threads);
    (void)path_partition;

    // Index i_thread = 1;
    // for (std::vector<inf::TreeSplitter::Path> const &paths : path_partition) {
    //
    //     util::logger << "Thread " << i_thread << ":" << util::cr;
    //     for (inf::TreeSplitter::Path const &path : paths) {
    //         util::logger << path << util::cr;
    //     }
    //
    //     ++i_thread;
    // }
}
