#include "srb.h"
#include "../../inf/constraints/constraint_parser.h"
#include "../../inf/inf_problem/feas_pb.h"
#include "../../inf/inf_problem/vis_pb.h"
#include "../../util/debug.h"
#include "../../util/logger.h"
#include "misc.h"

inf::Network::ConstPtr user::get_srb_network() {
    return inf::Network::create_triangle(2); // 2 outcomes
}

inf::TargetDistr::ConstPtr user::get_noisy_srb(Num visibility, Num visibility_denom) {
    Num const max_noise_level = visibility_denom;

    Num const noise_level = max_noise_level - visibility;

    ASSERT_LT(noise_level, max_noise_level + 1)
    ASSERT_LT(-1, noise_level)

    inf::Network::ConstPtr const network = user::get_srb_network();

    inf::EventTensor d(network->get_n_parties(), network->get_n_outcomes());
    Num const denom = max_noise_level * 8;
    d.set_denom(denom);

    for (inf::Event const &e : network->get_event_range()) {
        d.get_num(e) = noise_level;
        if (e[0] == e[1] and e[1] == e[2])
            d.get_num(e) += (max_noise_level - noise_level) * 4;
    }

    return std::make_shared<inf::TargetDistr>(
        "Noisy shared random bit with visibility " + inf::VisProblem::visibility_to_str(visibility, visibility_denom),
        "SRB", network, d);
}

// Applications

void user::srb_sym::run() {
    // Distribution
    Num const noise_level = 3;
    inf::TargetDistr::ConstPtr const d = user::get_noisy_srb(noise_level, 1000);
    util::logger << *(d->get_network()) << util::cr;
    d->log_long();
    util::logger << util::cr;
    HARD_ASSERT_EQUAL(d->get_n_orbits(), 2);

    std::vector<inf::Inflation::Size> const inf_sizes({{2, 2, 2},
                                                       {2, 2, 3},
                                                       {2, 3, 4}});
    // We indicated the lower bound that the number of orbits is at least 64/|G| where G is the symmetry group,
    // the sanity check is that expected_orbit_size[i] >= 64/|G[i]|
    std::vector<Index> const expected_orbit_size({
        8,  // 64/(2*2*6) = 2.6666
        14, // 64/(2*2*2) = 8
        20  // 64/(2*2)   = 16
    });

    for (Index const i : util::Range(inf_sizes.size())) {
        util::logger << "Inflation size: " << inf_sizes[i] << util::cr;

        inf::Inflation::ConstPtr inflation = std::make_shared<inf::Inflation>(d, inf_sizes[i]);

        std::vector<Index> const marginal_parties = inf::ConstraintParser::parse_inflation_marginal(*inflation, "A00,B00,C00,A11,B11,C11");
        inf::Marginal const marginal(inflation, marginal_parties, inf::Constraint::get_constraint_group(*inflation, marginal_parties, {}), inf::DualVector::StoreBounds::no);
        inf::DualVector const dual_vector(marginal, inf::DualVector::BoundType::lower);
        dual_vector.log_long();
        util::logger << util::cr;
        HARD_ASSERT_EQUAL(dual_vector.get_n_orbits_no_unknown(), expected_orbit_size[i]);
    }
}

void user::srb_eval::run() {
    Num const visibility = 3;
    inf::TargetDistr::ConstPtr const d = user::get_noisy_srb(visibility, 1000);

    user::test_dual_vector_eval(d);
}

void user::srb_vis_222_weak::run() {
    Num const srb_max_visibility = 100000;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 2})
        .set(inf::ConstraintSet::Description{
            // *** All the below are equivalent ***
            // {"A00,B00,C00", "A11,B11,C11", ""},
            // {"A00,B00", "A11,B11,C11"},
            // {"A00,B00", "A11,B11,C11", ""},
            {"A00,B00,C00", "A11,B11,C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_srb,
                           0, srb_max_visibility, srb_max_visibility,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

    // Elie finds that the critical visibility is 2\sqrt{3} - 3 = 46.4101615%, so this is perfect
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 46411)

    util::logger << util::cr;
}

void user::srb_vis_222_strong::run() {
    Num const srb_max_visibility = 100000;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 2})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,B11,C11", ""},
            // This one can be imposed but doesn't help
            // {"A01", "B01", "C01", ""},
            {"A00", "A11,B10,B11,C01,C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_srb,
                           0, srb_max_visibility, srb_max_visibility,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

    // Alex, Nicolas Gisin etc find that the critical visibility is \sqrt{2}-1 = 41.4213562%, so perfect
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 41422)

    util::logger << util::cr;
}

void user::srb_dual_vector_io::run() {
    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 2})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,B11,C11", ""},
            {"A00", "A11,B10,B11,C01,C11"},
        });

    Num const vis_denom = 100000;
    Num const vis_certificate = 41422;
    std::string const filename = "data/srb_222_certificate";
    std::string const metadata = "A nonlocality certificate for the Shared Random Bit with visibility " +
                                 inf::VisProblem::visibility_to_str(vis_certificate, vis_denom) +
                                 " obtained with inflation size 2x2x2 and all LPI constraints";

    {
        inf::FeasProblem feas_pb(user::get_noisy_srb(vis_certificate, vis_denom), get_feas_options());
        inf::FeasProblem::Status const status = feas_pb.get_feasibility();
        HARD_ASSERT_TRUE(status == inf::FeasProblem::Status::nonlocal)

        feas_pb.write_dual_vector_to_file(filename, metadata);
    }

    {
        inf::FeasProblem feas_pb(user::get_noisy_srb(0, vis_denom), get_feas_options());

        // Want the new vis to be even to ensure that the bounds for the stored quovec are respected.
        // If one really wanted to test the stored nonlocality certificate for odd visibilities, we would need
        // to implement a mechanism to rescale the quovec components to lower values to ensure that the overflow
        // mechanism is respected, but this is not really needed for our purposes.
        for (Num const vis_offset : util::Range(-5, 5)) {
            Num const vis = vis_certificate + 2 * vis_offset;
            feas_pb.update_target_distribution(user::get_noisy_srb(vis, vis_denom), inf::FeasProblem::RetainEvents::no);
            // Need to read the dual vector after updating the target distribution, since updating the taret distribution
            // resets the dual vector to all zeros
            feas_pb.read_dual_vector_from_file(filename, metadata);

            inf::Optimizer::Solution const sol = feas_pb.minimize_dual_vector();
            bool const nonlocal = sol.get_inflation_event_score() > 0;

            util::logger << "Visibility " << inf::VisProblem::visibility_to_str(vis, vis_denom)
                         << " is ";
            if (nonlocal) {
                util::logger << "nonlocal";
                HARD_ASSERT_LTE(vis_certificate, vis)
            } else {
                util::logger << "inconclusive";
                HARD_ASSERT_LT(vis, vis_certificate)
            }
            util::logger << " based on the certificate" << util::cr;
        }
    }

    util::logger << util::cr;
}

void user::srb_vis_detailed_test::run() {
    Num const srb_max_visibility = 100000;

    std::vector<inf::Optimizer::SearchMode> const search_modes = {
        inf::Optimizer::SearchMode::brute_force,
        inf::Optimizer::SearchMode::tree_search,
    };

    std::vector<inf::FeasProblem::RetainEvents> const retain_events = {
        inf::FeasProblem::RetainEvents::yes,
        inf::FeasProblem::RetainEvents::no,
    };

    std::vector<inf::Inflation::Size> const inf_sizes = {
        {2, 2, 2},
        {2, 2, 3},
        {2, 3, 2},
        {3, 2, 2},
    };

    std::vector<inf::Inflation::UseDistrSymmetries> const use_distr_symmetries_opt = {
        inf::Inflation::UseDistrSymmetries::yes,
        inf::Inflation::UseDistrSymmetries::no,
    };

    std::vector<Index> const param_dims = {
        search_modes.size(),
        retain_events.size(),
        inf_sizes.size(),
        use_distr_symmetries_opt.size(),
    };

    for (std::vector<Index> const &indices : util::ProductRange(param_dims)) {
        inf::Optimizer::SearchMode const search_mode = search_modes[indices[0]];
        inf::FeasProblem::RetainEvents const retain = retain_events[indices[1]];
        inf::Inflation::Size const &inf_size = inf_sizes[indices[2]];
        inf::Inflation::UseDistrSymmetries const use_distr_symmetries = use_distr_symmetries_opt[indices[3]];

        (*get_feas_options())
            .set(inf_size)
            .set(inf::ConstraintSet::Description{{"A00,B00,C00", "A11,B11,C11", ""}})
            .set(search_mode)
            .set(use_distr_symmetries)
            .set(inf::EventTree::IO::none);

        inf::VisProblem vis_pb(&user::get_noisy_srb,
                               0, srb_max_visibility, srb_max_visibility,
                               get_feas_options(),
                               retain);

        Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

        if (inf_size == inf::Inflation::Size({2, 2, 2})) {
            HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 46411)
        } else {
            HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 43979)
        }
    }

    util::logger << util::cr;
}

void user::srb_vis_223_weak::run() {
    Num const srb_max_visibility = 100000;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 3})
        .set(inf::ConstraintSet::Description{{"A00,B00,C00", "A11,B11,C11", ""}});

    inf::VisProblem vis_pb(&user::get_noisy_srb,
                           0, srb_max_visibility, srb_max_visibility,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 43979)

    util::logger << util::cr;
}

void user::srb_vis_223_strong::run() {
    Num const srb_max_visibility = 100000;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 3})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12, B11,B21, C11"}, // just this one yields 40.273%
            {"A00", "A11,A12, B10,B11,B20,B21, C01,C11"},
            {"C00", "A10,A11,A12, B01,B11,B21, C11"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_srb,
                           0,
                           srb_max_visibility,
                           srb_max_visibility,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

    // Alejandro Pozas-Kerstjens et al find 39.42% here so we're good
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 39415)

    util::logger << util::cr;
}

void user::srb_vis_233_weak::run() {
    Num const srb_max_visibility = 100000;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 3, 3})
        .set(inf::ConstraintSet::Description{{"A00,B00,C00", "A11,B11,C11", ""}});

    inf::VisProblem vis_pb(&user::get_noisy_srb,
                           0, srb_max_visibility, srb_max_visibility,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 43252)

    util::logger << util::cr;
}

void user::srb_vis_233_inter::run() {
    Num const srb_max_visibility = 100000;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 3, 3})
        .set(inf::ConstraintSet::Description{{"A00,B00,C00", "A11,B11,C11", "A22", ""}});

    inf::VisProblem vis_pb(&user::get_noisy_srb,
                           0, srb_max_visibility, srb_max_visibility,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();
    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 42016)

    util::logger << util::cr;
}

void user::srb_vis_233_strong::run() {
    Num const srb_max_visibility = 100000;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 3, 3})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,A21,A22, B11,B21, C11,C12"},         // this one gets 38.441% in 4min, 1315 calls
            {"A00", "A11,A12,A21,A22, B10,B11,B20,B21, C01,C02,C11,C12"}, // this one gets 37.756%, 3203 calls
            {"C00", "A10,A11,A12,A20,A21,A22, B01,B11,B21, C11,C12"},     // this one gets 37.676%, 3822 calls
        });

    inf::VisProblem vis_pb(&user::get_noisy_srb,
                           0, srb_max_visibility, srb_max_visibility,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 37676)

    util::logger << util::cr;
}

void user::srb_vis_333::run() {
    Num const denom = 100000;
    Num const min_vis = 35000;
    Num const max_vis = 37676;

    (*get_feas_options())
        .set(inf::Inflation::Size{3, 3, 3})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,A21,A22, B11,B12,B21,B22, C11,C12,C21,C22"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_srb,
                           min_vis, max_vis, denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 36780)

    util::logger << util::cr;
}

void user::srb_vis_334::run() {
    Num const denom = 100000;
    Num const min_vis = 36625;
    Num const max_vis = 36635;

    (*get_feas_options())
        .set(inf::Inflation::Size{3, 3, 4})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,A21,A22, B11,B12,B21,B22, C11,C12,C21,C22"},
        });

    inf::VisProblem vis_pb(&user::get_noisy_srb,
                           min_vis, max_vis, denom,
                           get_feas_options(),
                           inf::FeasProblem::RetainEvents::yes);

    Num const minimum_nonlocal_visibility = vis_pb.get_minimum_nonlocal_visibility();

    HARD_ASSERT_EQUAL(minimum_nonlocal_visibility, 36629)

    util::logger << util::cr;
}

void user::srb::run() {
    Num const srb_max_visibility = 100000;
    Num const visibility = 37676;
    inf::TargetDistr::ConstPtr const d = user::get_noisy_srb(visibility, srb_max_visibility);
    util::logger
        // << *(d->get_network()) << util::cr
        << *d << util::cr;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 3, 3})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,A21,A22, B11,B21, C11,C12"},         // this one gets 38.441% in 4min, 1315 calls
            {"A00", "A11,A12,A21,A22, B10,B11,B20,B21, C01,C02,C11,C12"}, // this one gets 37.756%, 3203 calls
            {"C00", "A10,A11,A12,A20,A21,A22, B01,B11,B21, C11,C12"},     // this one gets 37.676%, 3822 calls
        });

    inf::FeasProblem feas_problem(d, get_feas_options());

    LOG_BEGIN_SECTION("Feasibility")
    feas_problem.get_feasibility();
    LOG_END_SECTION

    util::logger << util::cr;
}
