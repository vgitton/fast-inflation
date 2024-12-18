#include "ejm.h"
#include "../../inf/constraints/constraint_parser.h"
#include "../../inf/constraints/dual_vector.h"
#include "../../inf/inf_problem/feas_pb.h"
#include "../../inf/inf_problem/vis_pb.h"
#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/misc.h"
#include "misc.h"

inf::Network::ConstPtr user::get_ejm_network() {
    return inf::Network::create_triangle(4); // 4 outcomes
}

inf::TargetDistr::ConstPtr user::get_symmetric_distribution(Index s111,
                                                            Index s112,
                                                            Index s123,
                                                            Index s_denom) {
    HARD_ASSERT_EQUAL(s111 + s112 + s123, s_denom)

    inf::Network::ConstPtr network = user::get_ejm_network();

    inf::EventTensor d(network->get_n_parties(), network->get_n_outcomes());

    // Smallest common multiple (4,36,24) = (2^2, 2^2*3^2, 2^3*3) = 2^3*3^2 = 72
    Num const d_denom = s_denom * 72;
    // Have:
    // 1/4  = 18/72
    // 1/36 =  2/72
    // 1/24 =  3/72

    d.set_denom(d_denom);

    for (inf::Event const &e : network->get_event_range()) {
        if (e[0] == e[1] and e[1] == e[2]) {
            d.get_num(e) = s111 * 18;
        } else if (e[0] != e[1] and e[1] != e[2] and e[2] != e[0]) {
            d.get_num(e) = s123 * 3;
        } else {
            d.get_num(e) = s112 * 2;
        }
    }
    return std::make_shared<inf::TargetDistr>(
        "(s111,s112,s123) = (" + util::str(s111) + "," + util::str(s112) + "," + util::str(s123) + ") / " + util::str(s_denom),
        "EJM",
        network,
        d);
}

inf::TargetDistr::ConstPtr user::get_noisy_pureejm(Num vis, Num vis_denom) {
    HARD_ASSERT_LT(vis, vis_denom + 1)

    inf::Network::ConstPtr network = user::get_ejm_network();

    inf::EventTensor d(network->get_n_parties(), network->get_n_outcomes());

    // Mixed:
    // p(a,b,c) = 1/64 = 3/192
    // Pure EJM:
    // p(000) = 1/8 = 24/192, p(012) = 1/48 = 4/192
    // Common denom: 192 * vis_denom
    Num const d_denom = vis_denom * 192;

    d.set_denom(d_denom);

    for (inf::Event const &e : network->get_event_range()) {
        // 111
        if (e[0] == e[1] and e[1] == e[2]) {
            d.get_num(e) = (vis_denom - vis) * 3 + vis * 24;
        }
        // 123
        else if (e[0] != e[1] and e[1] != e[2] and e[2] != e[0]) {
            d.get_num(e) = (vis_denom - vis) * 3 + vis * 4;
        }
        // 112
        else {
            d.get_num(e) = (vis_denom - vis) * 3 + vis * 0;
        }
    }

    std::string const name = "Noisy purified EJM with vis " + inf::VisProblem::visibility_to_str(vis, vis_denom);

    return std::make_shared<inf::TargetDistr>(name, "EJM", network, d);
}

inf::TargetDistr::ConstPtr user::get_ejm_distribution() {
    inf::Network::ConstPtr network = user::get_ejm_network();

    inf::EventTensor d(network->get_n_parties(), network->get_n_outcomes());
    Num denom = 256;
    d.set_denom(denom);

    for (inf::Event const &e : network->get_event_range()) {
        if (e[0] == e[1] and e[1] == e[2]) {
            d.get_num(e) = 25;
        } else if (e[0] != e[1] and e[1] != e[2] and e[2] != e[0]) {
            d.get_num(e) = 5;
        } else {
            d.get_num(e) = 1;
        }
    }

    return std::make_shared<inf::TargetDistr>("Elegant Joint Measurement", "EJM", network, d);
}

// Applications

//! \cond
namespace user {

// We use this function to print out the first few and the last of the symmetrized
// inflation events in the paper. This is designed to work only for the 222 inflation.
void print_some_events(inf::Inflation const &inflation) {
    for (Index const size : inflation.get_size())
        HARD_ASSERT_EQUAL(size, 2)

    util::logger << " %%%%%%%%%%%%%%%%%%%%% LaTeX PRINTING %%%%%%%%%%%%%%%%%%%%% " << util::cr;

    std::vector<std::string> const names = {"alice", "bob", "charlie"};
    util::logger << "Ordering:" << util::cr;
    util::logger << "(";
    for (Index const party : util::Range(inflation.get_n_parties())) {
        if (party > 0)
            util::logger << ",";
        util::logger << "\\infevent(\\"
                     << names[inflation.get_parties()[party].first]
                     << "{" << inflation.get_parties()[party].second[0]
                     << inflation.get_parties()[party].second[1] << "})";
    }
    util::logger << ")" << util::cr << util::cr;

    inf::EventTree const &tree = inflation.get_symtree(inf::EventTree::IO::none);

    inf::EventTree::NodePos::Queue queue = tree.get_root_children_queue();
    Index leaf_count = 0;
    Index const n_repr = 8;
    Index repr_index = n_repr - 1;
    std::vector<inf::Event> event_reprs(n_repr, inf::Event(inflation.get_n_parties()));
    inf::Event working_event(inflation.get_n_parties());

    while (not queue.empty()) {

        inf::EventTree::NodePos const node_pos = util::pop_back(queue);
        working_event[node_pos.depth] = tree.get_node(node_pos).outcome;

        if (node_pos.depth < inflation.get_n_parties() - 1)
            tree.add_children_to_queue(queue, node_pos);
        else {
            if (leaf_count == 0 or leaf_count >= tree.get_n_leaves() - (n_repr - 1)) {
                event_reprs[repr_index] = working_event;
                --repr_index;
            }
            leaf_count += 1;
        }
    }

    // Print the events
    for (inf::Event const &event : event_reprs) {
        // Print the event
        inflation.log_event(event);
        // Print the event in latex
        user::print_event_latex_format(inflation, event);
    }

    util::logger << "Orbit sample:" << util::cr << util::cr;

    // Print the orbits
    for (Index const &i : util::Range<Index>(2)) {
        inf::Event const &event_repr = event_reprs[i];
        inflation.log_event(event_repr);
        user::print_event_latex_format(inflation, event_repr);

        std::set<inf::Event> orbit;

        for (inf::Symmetry const &symmetry : inflation.get_inflation_symmetries()) {
            symmetry.act_on_event(event_repr, working_event);
            orbit.insert(working_event);
        }

        Index j = 0;
        std::set<inf::Event>::const_iterator orbit_it = orbit.begin();
        while (j < 4 and j < orbit.size()) {
            inflation.log_event(*orbit_it);
            user::print_event_latex_format(inflation, *orbit_it);
            ++orbit_it;
            ++j;
        }

        util::logger << "..." << util::cr;

        inflation.log_event(*(--orbit.end()));
        user::print_event_latex_format(inflation, *(--orbit.end()));
    }

    util::logger << " %%%%%%%%%%%%%%%%%%%%% END LaTeX PRINTING %%%%%%%%%%%%%%%%%%%%% " << util::cr;
}

} // namespace user
//! \endcond

void user::ejm_sym::run() {
    inf::TargetDistr::ConstPtr d = user::get_ejm_distribution();
    util::logger << *(d->get_network()) << util::cr;
    d->log_long();
    util::logger << util::cr;
    HARD_ASSERT_EQUAL(d->get_n_orbits(), 3);

    std::vector<inf::Inflation::Size> inf_sizes({{2, 2, 2},
                                                 {2, 2, 3},
                                                 {2, 3, 4}});
    // Indicated the lower bound that the number of orbits is at least 4096/|G| where G is the symmetry group,
    // the sanity check is that expected_orbit_size[i] >= 4096/|G[i]|
    std::vector<Index> expected_orbit_size({
        33, // 4096/(2*24*6) = 14.222
        67, // 4096/(2*24*2) = 42.6666
        107 // 4096/(2*24)   = 85.3333333
    });

    for (Index i : util::Range(inf_sizes.size())) {
        util::logger << "Inflation size: " << inf_sizes[i] << util::cr;

        inf::Inflation::ConstPtr inflation = std::make_shared<inf::Inflation>(
            d, inf_sizes[i]);

        if (i == 0) {
            // Only do this for the 2x2x2 inflation
            user::print_some_events(*inflation);
        }

        std::vector<Index> const marginal_parties = inf::ConstraintParser::parse_inflation_marginal(*inflation, "A00,B00,C00,A11,B11,C11");
        inf::Marginal marginal(inflation, marginal_parties, inf::Constraint::get_constraint_group(*inflation, marginal_parties, {}), inf::DualVector::StoreBounds::no);
        inf::DualVector dual_vector(marginal, inf::DualVector::BoundType::lower);
        dual_vector.log_long();
        util::logger << util::cr;
        HARD_ASSERT_EQUAL(dual_vector.get_n_orbits_no_unknown(), expected_orbit_size[i]);
    }
}

void user::ejm_eval::run() {
    inf::TargetDistr::ConstPtr d = user::get_ejm_distribution();

    user::test_dual_vector_eval(d);
}

void user::ejm_symtree::run() {
    inf::TargetDistr::ConstPtr d = user::get_ejm_distribution();

    inf::Inflation inflation(d, {2, 2, 4});

    inf::EventTree const &symtree = inflation.get_symtree(get_feas_options()->get_symtree_io());
    symtree.log_info();

    util::logger << util::cr;
}

void user::ejm::run() {
    unsigned int const vis_param = get_inf_cli().get_visibility_param();
    if (vis_param > 512)
        THROW_ERROR("Expected the --vis command line argument to be between 0 and 512")

    // vis_param == 383 and above is nonlocal
    inf::TargetDistr::ConstPtr d = user::get_noisy_pureejm(vis_param, 512);
    util::logger << *(d->get_network())
                 << util::cr << *d << util::cr;

    // The ejm_vis_224_inter_v2_diag_A constraints
    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 4})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,A13, B11,B21,B31, C11"},
            {"A00", "B10,B11,B20,B21,B30,B31, C01,C11"},
        });

    inf::FeasProblem feas_problem(d, get_feas_options());

    LOG_BEGIN_SECTION("Feasibility")
    feas_problem.get_feasibility();
    LOG_END_SECTION

    util::logger << util::cr;
}

// VISIBILITY STUDIES

// 2x2x2 ==========================

void user::ejm_vis_222_weak::run() {
    Num const denom = 512;
    Num const min_vis = 384;
    Num const max_vis = 512;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 2})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,B11,C11", ""},
        });

    inf::VisProblem vis_pb(&user::get_noisy_pureejm,
                           min_vis,
                           max_vis,
                           denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 467)

    util::logger << util::cr;
}

void user::ejm_vis_222_strong::run() {
    Num const denom = 512;
    Num const min_vis = 384;
    Num const max_vis = 467;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 2})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,B11,C11", ""},
            {"A00", "A11,B10,B11,C01,C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_pureejm,
                           min_vis,
                           max_vis,
                           denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 431)

    util::logger << util::cr;
}

// 2x2x3 ==========================

void user::ejm_vis_223_weak::run() {
    Num const denom = 512;
    Num const min_vis = 384;
    Num const max_vis = 431;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 3})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12, B11,B21, C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_pureejm,
                           min_vis,
                           max_vis,
                           denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    // Using only the main diag constraint, get 465/512, so a tiny improvement over the 467/512 2x2x2 inflation
    // Using main diag constraint + 2x2x2-inspired LPI constraint, get 435/512

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 416)

    util::logger << util::cr;
}

void user::ejm_vis_223_strong::run() {
    Num const denom = 512;
    Num const min_vis = 384;
    Num const max_vis = 416;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 3})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12, B11,B21, C11"},
            {"A00", "A11,A12, B10,B11,B20,B21, C01,C11"},
            {"C00", "A10,A11,A12, B01,B11,B21, C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_pureejm,
                           min_vis,
                           max_vis,
                           denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 391)

    util::logger << util::cr;
}

// 2x2x4 ============================

void user::ejm_vis_224_weak::run() {
    Num const denom = 512;
    Num const min_vis = 387;
    Num const max_vis = 388;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 4})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12, B11,B21, C11"},
            {"A00", "A11,A12, B10,B11,B20,B21, C01,C11"},
            {"C00", "A10,A11,A12, B01,B11,B21, C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_pureejm,
                           min_vis,
                           max_vis,
                           denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

    // Using the constraints of the 2x2x3 inflation, obtains 388/512 = 75.78%
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 388)

    util::logger << util::cr;
}

void user::ejm_vis_224_weak_bis::run() {
    Num const denom = 512;
    Num const min_vis = 391;
    Num const max_vis = 392;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 4})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12, B11,B21, C11"},
            {"A00", "B10,B11,B20,B21,B30,B31, C01,C11"},
            {"C00", "A10,A11,A12, B01,B11,B21, C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_pureejm,
                           min_vis,
                           max_vis,
                           denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 392)

    util::logger << util::cr;
}

void user::ejm_vis_224_inter_v1::run() {
    Num const denom = 512;
    Num const min_vis = 395;
    Num const max_vis = 396;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 4})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,A13, B11,B21,B31, C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_pureejm,
                           min_vis,
                           max_vis,
                           denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 396)

    util::logger << util::cr;
}

void user::ejm_vis_224_inter_v2_diag_A::run() {
    Num const denom = 512;
    Num const min_vis = 382;
    Num const max_vis = 383;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 4})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,A13, B11,B21,B31, C11"},
            {"A00", "B10,B11,B20,B21,B30,B31, C01,C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_pureejm,
                           min_vis,
                           max_vis,
                           denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 383)

    util::logger << util::cr;
}

void user::ejm_vis_224_inter_v2_diag_C::run() {
    Num const denom = 512;
    Num const min_vis = 394;
    Num const max_vis = 395;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 4})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,A13, B11,B21,B31, C11"},
            {"C00", "A10,A11,A12,A13, B01,B11,B21,B31, C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_pureejm,
                           min_vis,
                           max_vis,
                           denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 395)

    util::logger << util::cr;
}

// Nonlocality certificates

namespace user {

//! \cond
void ejm_nl_certificate(bool check, inf::FeasOptions::Ptr const &feas_options) {
    inf::TargetDistr::ConstPtr const ejm = user::get_ejm_distribution();
    std::string const filename = "data/ejm_224_nl_certificate";
    std::string const metadata = "Nonlocality certificate for the EJM distribution";

    (*feas_options)
        .set(inf::Inflation::Size{2, 2, 4})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,A13, B11,B21,B31, C11"},
            {"A00", "B10,B11,B20,B21,B30,B31, C01,C11"},
        });

    if (not check) {
        util::logger << "Let us look for a nonlocality certificate for the EJM." << util::cr
                     << "It will be written to " + filename + ".txt." << util::cr;
    } else {
        util::logger << "Let us check a nonlocality certificate for the EJM." << util::cr;
    }
    util::logger << util::cr;

    inf::FeasProblem feas_pb(ejm, feas_options);
    util::logger << util::cr;

    if (not check) {
        inf::FeasProblem::Status status = feas_pb.get_feasibility();
        HARD_ASSERT_TRUE(status == inf::FeasProblem::Status::nonlocal)
        feas_pb.write_dual_vector_to_file(filename, metadata);
    } else {
        inf::FeasProblem::Status status = feas_pb.read_and_check_dual_vector(filename, metadata);
        HARD_ASSERT_TRUE(status == inf::FeasProblem::Status::nonlocal)
    }

    util::logger << util::cr;
}
//! \endcond

} // namespace user

void user::ejm_find_nl_certificate::run() {
    user::ejm_nl_certificate(false, get_feas_options());
}

void user::ejm_check_nl_certificate::run() {
    user::ejm_nl_certificate(true, get_feas_options());
}

// SCANS --------------------------------

void user::scan_symmetric_distributions(inf::FeasOptions::Ptr const &feas_options,
                                        Index const s_denom,
                                        Index const run_index,
                                        Index const n_runs) {
    HARD_ASSERT_LT(run_index, n_runs)

    Index const n_distributions = (s_denom + 1) * (s_denom + 2) / 2;

    Index const start_distribution_index = run_index * n_distributions / n_runs;

    Index const end_distribution_index = static_cast<Index>((run_index + 1) * n_distributions / n_runs - 1);

    LOG_BEGIN_SECTION_FUNC

    inf::TargetDistr::ConstPtr d = user::get_symmetric_distribution(s_denom, 0, 0, s_denom);

    util::logger << "s_denom: " << s_denom << ", number of distributions: " << n_distributions << util::cr;
    util::logger << "run index: " << run_index << ", n_runs: " << n_runs
                 << ", scanning distributions #" << start_distribution_index
                 << " through #" << end_distribution_index << util::cr;

    util::logger << *feas_options;

    inf::FeasProblem::RetainEvents const retain_events = inf::FeasProblem::RetainEvents::yes;
    inf::FeasProblem::log(retain_events);
    util::logger << util::cr;

    inf::FeasProblem feas_problem(d, feas_options);

    std::string csv_results = "\n\n";
    csv_results += "(feas = 0 means nonlocal, feas = 1 means inconclusive)\n";
    csv_results += "----------------------------------\n";
    csv_results += "lambda111,lambda112,lambda123,feas\n";

    Index current_distribution_index = 0;

    LOG_BEGIN_SECTION("Scanning")
    for (Index const s111 : util::Range(s_denom + 1)) {
        for (Index const s112 : util::Range(static_cast<Index>(s_denom - s111 + 1))) {
            bool const out_of_range = (current_distribution_index < start_distribution_index or current_distribution_index > end_distribution_index);

            ++current_distribution_index;

            if (out_of_range)
                continue;

            Index const s123 = static_cast<Index>(s_denom - s111 - s112);

            d = user::get_symmetric_distribution(s111, s112, s123, s_denom);

            feas_problem.update_target_distribution(d, retain_events);

            inf::FeasProblem::Status const feas_status = feas_problem.get_feasibility();

            csv_results += util::str(float(s111) / float(s_denom)) + "," +
                           util::str(float(s112) / float(s_denom)) + "," +
                           util::str(float(s123) / float(s_denom)) + "," +
                           (feas_status == inf::FeasProblem::Status::nonlocal ? "0" : "1") + "\n";
        }
    }
    HARD_ASSERT_EQUAL(current_distribution_index, n_distributions)
    LOG_END_SECTION

    csv_results += "----------------------------------\n";
    util::logger << csv_results;

    LOG_END_SECTION
}

void user::ejm_scan_222::run() {
    Index const s_denom = 40;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 2})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,B11,C11", ""},
        });

    user::scan_symmetric_distributions(get_feas_options(), s_denom, 0, 1);
}

//! \cond

namespace user {

void scan_symmetric_distributions_223(inf::FeasOptions::Ptr const &feas_options, Index const run_index) {
    Index const s_denom = 40;
    Index const n_runs = 10;
    (*feas_options)
        .set(inf::Inflation::Size{2, 2, 3})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,B11,C11", ""},
        });

    user::scan_symmetric_distributions(feas_options, s_denom, run_index, n_runs);
}

} // namespace user

void user::ejm_scan_223_i0::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 0);
}

void user::ejm_scan_223_i1::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 1);
}

void user::ejm_scan_223_i2::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 2);
}

void user::ejm_scan_223_i3::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 3);
}

void user::ejm_scan_223_i4::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 4);
}

void user::ejm_scan_223_i5::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 5);
}

void user::ejm_scan_223_i6::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 6);
}

void user::ejm_scan_223_i7::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 7);
}

void user::ejm_scan_223_i8::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 8);
}

void user::ejm_scan_223_i9::run() {
    user::scan_symmetric_distributions_223(get_feas_options(), 9);
}

//! \endcond
