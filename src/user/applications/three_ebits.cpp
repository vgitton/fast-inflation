#include "three_ebits.h"
#include "../../inf/inf_problem/feas_pb.h"
#include "../../util/debug.h"
#include "../../util/logger.h"

void user::y_basis::run() {
    inf::Outcome const n_out = 4;
    inf::Network::ConstPtr network = inf::Network::create_triangle(n_out);

    inf::EventTensor d(network->get_n_parties(), network->get_n_outcomes());

    std::vector<std::vector<std::vector<Num>>> ng_matrix;

    bool const use_actual_probabilities = true;

    if (use_actual_probabilities) {
        d.set_denom(128);
        // Case c = 0
        ng_matrix.push_back({{0, 1, 1, 0},
                             {1, 4, 0, 1},
                             {1, 0, 4, 9},
                             {0, 9, 1, 0}});

        // Case c = 1
        ng_matrix.push_back({{1, 4, 0, 9},
                             {4, 1, 1, 4},
                             {0, 1, 1, 0},
                             {1, 4, 0, 1}});

        // Case c = 2
        ng_matrix.push_back({{1, 0, 4, 1},
                             {0, 1, 1, 0},
                             {4, 1, 1, 4},
                             {9, 0, 4, 1}});

        // Case c = 3
        ng_matrix.push_back({{0, 1, 9, 0},
                             {9, 4, 0, 1},
                             {1, 0, 4, 1},
                             {0, 1, 1, 0}});
    } else {
        // Fake values to test nonlocality behavior
        Num one, four, nine;
        // NONLOCAL
        one = 1;
        four = 2;
        nine = 20;
        // ?
        // one = 1;
        // four = 2;
        // nine = 3;

        // Case c = 0
        ng_matrix.push_back({{0, one, one, 0},
                             {one, four, 0, one},
                             {one, 0, four, nine},
                             {0, nine, one, 0}});

        // Case c = 1
        ng_matrix.push_back({{one, four, 0, nine},
                             {four, one, one, four},
                             {0, one, one, 0},
                             {one, four, 0, one}});

        // Case c = 2
        ng_matrix.push_back({{one, 0, four, one},
                             {0, one, one, 0},
                             {four, one, one, four},
                             {nine, 0, four, one}});

        // Case c = 3
        ng_matrix.push_back({{0, one, nine, 0},
                             {nine, four, 0, one},
                             {one, 0, four, one},
                             {0, one, one, 0}});
    }

    Num fake_denom = 0;

    for (inf::Outcome a : util::Range(n_out)) {
        for (inf::Outcome b : util::Range(n_out)) {
            for (inf::Outcome c : util::Range(n_out)) {
                d.get_num({a, b, c}) = ng_matrix[c][a][b];
                fake_denom += ng_matrix[c][a][b];
            }
        }
    }

    if (not use_actual_probabilities)
        d.set_denom(fake_denom);

    inf::TargetDistr::ConstPtr p = std::make_shared<inf::TargetDistr>(
        "Y Basis Three-Ebit Solution",
        "Y",
        network,
        d);

    util::logger << *p;

    // p->log_orbits(false);
    //
    // for (inf::Symmetry const &sym : p->get_sym_group()) {
    //     util::logger << sym << util::cr;
    // }

    // Probably quite long, 8k constraints...
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         "q(A00,B00,C00, A11,A12,B11,B21,C11) = p(A,B,C)q(A11,A12,B11,B21,C11)"});

    // Inconclusive, opt: 7m20s, fw: 2m58s, tot: 10m24s
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11"},
    //         {"A00", "B10,B11, C01,C11"},
    //     });

    // Inconclusive, opt: 6m37s, fw: 3m45s, tot: 10m29s
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11, C11"},
    //         {"A00", "B10,B11, C01,C11"},
    //     });

    // Inconclusive, opt: 7m13s, fw: 4m54s, tot: 12m14s
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11, C11"},
    //         {"A00", "A11, B10,B11, C01,C11"},
    //     });

    // Inconclusive, opt: 6m08s, fw: 15m48s, tot: 22m19s
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11,A12, B01,B11,B21, C11"},
    //         {"A00", "A11, B10,B11, C01,C11"},
    //     });

    // Inconclusive, opt: 26m26s, fw: 57m27s, tot: 1h24m
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11, C11"},
    //         {"A00", "A11,A12, B10,B11, C01,C11"},
    //     });

    // Inconclusive, opt: 28m29s, fw: 1h35m, tot: 2h04m
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11, C11"},
    //         {"A00", "B10,B11,B20,B21, C01,C11"},
    //     });

    // Inconclusive, opt: 25m03s, fw: 1h54m, tot: 2h20m
    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 3})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,B11,C11", ""},
            {"C00", "A10,A11,A12, B01,B11,B21"},
            {"A00", "B10,B11,B20,B21, C01,C11"},
        });

    inf::FeasProblem feas_problem(p, get_feas_options());

    inf::FeasProblem::Status const status = feas_problem.get_feasibility();
    (void)status;
}

void user::y_basis_sym::run() {
    inf::Outcome n_out = 4;
    inf::Network::ConstPtr network = inf::Network::create_triangle(n_out);

    inf::EventTensor d(network->get_n_parties(), network->get_n_outcomes());

    std::vector<std::vector<std::vector<Num>>> ng_matrix;

    bool const use_actual_probabilities = false;

    if (use_actual_probabilities) {
        d.set_denom(128);
        // Case c = 0
        ng_matrix.push_back({{0, 1, 1, 0},
                             {1, 4, 0, 1},
                             {1, 0, 4, 9},
                             {0, 9, 1, 0}});

        // Case c = 1
        ng_matrix.push_back({{1, 4, 0, 9},
                             {4, 1, 1, 4},
                             {0, 1, 1, 0},
                             {1, 4, 0, 1}});

        // Case c = 2
        ng_matrix.push_back({{1, 0, 4, 1},
                             {0, 1, 1, 0},
                             {4, 1, 1, 4},
                             {9, 0, 4, 1}});

        // Case c = 3
        ng_matrix.push_back({{0, 1, 9, 0},
                             {9, 4, 0, 1},
                             {1, 0, 4, 1},
                             {0, 1, 1, 0}});
    } else {
        // Fake values to test nonlocality behavior
        Num one, four, nine;
        // NONLOCAL
        // one = 1;
        // four = 2;
        // nine = 20;
        // INCONCLUSIVE
        // one = 1;
        // four = 2;
        // nine = 3;
        // INCONCLUSIVE
        // one = 20;
        // four = 1;
        // nine = 2;
        // NONLOCAL
        // one = 1;
        // four = 20;
        // nine = 2;
        // NONLOCAL
        // one = 1;
        // four = 10;
        // nine = 2;
        // INCONCLUSIVE
        // one = 1;
        // four = 5;
        // nine = 2;
        // INCONCLUSIVE
        // one = 1;
        // four = 8;
        // nine = 2;
        // INCONCLUSIVE
        // one = 1;
        // four = 9;
        // nine = 2;
        // NONLOCAL
        // one = 2;
        // four = 19;
        // nine = 4;
        // NONLOCAL
        // one = 4;
        // four = 37;
        // nine = 8;
        // INCONCLUSIVE
        one = 8;
        four = 73;
        nine = 16;
        // --> For all the above tests, using or not using the symmetries
        // of the distribution does not change the outcome of the below inf::FeasProblem
        // So the symmetry mechanism also makes sense in this case.

        // Case c = 0
        ng_matrix.push_back({{0, one, one, 0},
                             {one, four, 0, one},
                             {one, 0, four, nine},
                             {0, nine, one, 0}});

        // Case c = 1
        ng_matrix.push_back({{one, four, 0, nine},
                             {four, one, one, four},
                             {0, one, one, 0},
                             {one, four, 0, one}});

        // Case c = 2
        ng_matrix.push_back({{one, 0, four, one},
                             {0, one, one, 0},
                             {four, one, one, four},
                             {nine, 0, four, one}});

        // Case c = 3
        ng_matrix.push_back({{0, one, nine, 0},
                             {nine, four, 0, one},
                             {one, 0, four, one},
                             {0, one, one, 0}});
    }

    Num fake_denom = 0;

    for (inf::Outcome a : util::Range(n_out)) {
        for (inf::Outcome b : util::Range(n_out)) {
            for (inf::Outcome c : util::Range(n_out)) {
                d.get_num({a, b, c}) = ng_matrix[c][a][b];

                fake_denom += ng_matrix[c][a][b];
            }
        }
    }

    if (not use_actual_probabilities)
        d.set_denom(fake_denom);

    inf::TargetDistr::ConstPtr p = std::make_shared<inf::TargetDistr>("Fake Y Basis Three-Ebit Solution",
                                                                      "Y",
                                                                      network,
                                                                      d);

    util::logger << *p;

    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 2});

    inf::FeasProblem::Status status_with_sym, status_without_sym;
    // With symmetry
    {
        (*get_feas_options())
            .set(inf::Inflation::UseDistrSymmetries::yes)
            .set(inf::ConstraintSet::Description{
                {"A00,B00,C00", ""},
                {"A00", "B10,B11"},
            });
        inf::FeasProblem feas_problem(p, get_feas_options());
        status_with_sym = feas_problem.get_feasibility();
    }
    // Without symmetry
    {
        (*get_feas_options())
            .set(inf::Inflation::UseDistrSymmetries::no)
            .set(inf::ConstraintSet::Description{
                {"A00,B00,C00", ""},
                {"A00", "B10,B11"},
                {"A00", "C01,C11"},
                {"B00", "A01,A11"},
                {"B00", "C10,C11"},
                {"C00", "B01,B11"},
                {"C00", "A10,A11"},
            });
        inf::FeasProblem feas_problem(p, get_feas_options());
        status_without_sym = feas_problem.get_feasibility();
    }

    if (status_with_sym != status_without_sym)
        THROW_ERROR("Expected the symmetry acceleration to not change the answer to the mathematical problem!")
}

void user::pjm::run() {
    inf::Outcome const n_out = 4;
    inf::Network::ConstPtr network = inf::Network::create_triangle(n_out);

    std::vector<std::vector<std::vector<Num>>> ng_matrix;

    // Case c = 0
    ng_matrix.push_back({{0, 0, 1, 1},
                         {0, 0, 5, 5},
                         {1, 5, 4, 0},
                         {1, 5, 0, 4}});

    // Case c = 1
    ng_matrix.push_back({{0, 0, 5, 5},
                         {0, 0, 1, 1},
                         {5, 1, 4, 0},
                         {5, 1, 0, 4}});

    // Case c = 2
    ng_matrix.push_back({{1, 5, 4, 0},
                         {5, 1, 4, 0},
                         {4, 4, 1, 1},
                         {0, 0, 1, 1}});

    // Case c = 3
    ng_matrix.push_back({{1, 5, 0, 4},
                         {5, 1, 0, 4},
                         {0, 0, 1, 1},
                         {4, 4, 1, 1}});

    inf::EventTensor d(network->get_n_parties(), network->get_n_outcomes());
    d.set_denom(128);

    for (inf::Outcome a : util::Range(n_out)) {
        for (inf::Outcome b : util::Range(n_out)) {
            for (inf::Outcome c : util::Range(n_out)) {
                d.get_num({a, b, c}) = ng_matrix[c][a][b];
            }
        }
    }
    inf::TargetDistr::ConstPtr p = std::make_shared<inf::TargetDistr>(
        "PJM Distribution",
        "PJM",
        network,
        d);

    util::logger << *p;

    // p->log_orbits(false);
    // for (inf::Symmetry const &sym : p->get_sym_group()) {
    //     util::logger << sym << util::cr;
    // }

    // Inconclusive, opt: 1m13s, fw: 21s412ms, tot: 1m37s
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11"},
    //         {"A00", "B10,B11, C01,C11"},
    //     });

    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11, C11"},
    //         {"A00", "B10,B11, C01,C11"},
    //     });

    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11, C11"},
    //         {"A00", "A11, B10,B11, C01,C11"},
    //     });

    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11,A12, B01,B11,B21, C11"},
    //         {"A00", "A11, B10,B11, C01,C11"},
    //     });

    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11, C11"},
    //         {"A00", "A11,A12, B10,B11, C01,C11"},
    //     });

    // Inconclusive, opt: 5m40s, fw: 10m50s, tot: 16m38s
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11, B01,B11, C11"},
    //         {"A00", "B10,B11,B20,B21, C01,C11"},
    //     });

    // Inconclusive, opt: 6m13s, fw: 20m51s, tot: 27m25s
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,B11,C11", ""},
    //         {"C00", "A10,A11,A12, B01,B11,B21, C11"},
    //         {"A00", "B10,B11,B20,B21, C01,C11"},
    //     });

    // Inconclusive, opt: 2m25s, tot: ~40m
    // (*get_feas_options())
    //     .set(inf::Inflation::Size{2, 2, 3})
    //     .set(inf::ConstraintSet::Description{
    //         {"A00,B00,C00", "A11,A12,B11,B21, C11"},
    //     });

    // Inconclusive, total time unclear but on the order of 10h
    (*get_feas_options())
        .set(inf::Inflation::Size{2, 2, 3})
        .set(inf::ConstraintSet::Description{
            {"A00,B00,C00", "A11,A12,B11,B21, C11"},
            {"A00", "B10,B11,B20,B21, C01,C11"},
        });

    inf::FeasProblem feas_problem(p, get_feas_options());

    inf::FeasProblem::Status const status = feas_problem.get_feasibility();
    (void)status;
}
