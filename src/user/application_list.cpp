#include "application_list.h"

// Add headers here
#include "applications/ejm.h"
#include "applications/misc.h"
#include "applications/srb.h"
#include "applications/three_ebits.h"

std::vector<user::Application::Ptr> user::get_application_list() {
    std::vector<user::Application::Ptr> const application_list =
        {
            std::make_shared<user::frac>(),
            std::make_shared<user::event_sym>(),
            std::make_shared<user::fully_corrective_fw>(),
            std::make_shared<user::pairwise_fw>(),
            // std::make_shared<user::redundancy>(),
            std::make_shared<user::event_tensor>(),
            std::make_shared<user::event_tree>(),
            std::make_shared<user::distribution_marginal>(),
            std::make_shared<user::inflation_marginal>(),
            std::make_shared<user::inf_tree_filler>(),
            std::make_shared<user::dual_vector_bounds>(),
            std::make_shared<user::dual_vector_io>(),
            std::make_shared<user::opt_bounds>(),
            std::make_shared<user::opt_time>(),
            std::make_shared<user::file_stream>(),
            std::make_shared<user::tree_splitter>(),
            std::make_shared<user::inf_tree_splitting>(),
            // EJM
            std::make_shared<user::ejm>(),
            std::make_shared<user::ejm_sym>(),
            std::make_shared<user::ejm_eval>(),
            std::make_shared<user::ejm_symtree>(),
            std::make_shared<user::ejm_vis_222_weak>(),
            std::make_shared<user::ejm_vis_222_strong>(),
            std::make_shared<user::ejm_vis_223_weak>(),
            std::make_shared<user::ejm_vis_223_strong>(),
            std::make_shared<user::ejm_vis_224_weak>(),
            std::make_shared<user::ejm_vis_224_weak_bis>(),
            std::make_shared<user::ejm_vis_224_inter_v1>(),
            std::make_shared<user::ejm_vis_224_inter_v2_diag_A>(),
            std::make_shared<user::ejm_vis_224_inter_v2_diag_C>(),
            std::make_shared<user::ejm_find_nl_certificate>(),
            std::make_shared<user::ejm_check_nl_certificate>(),
            // std::make_shared<user::ejm_scan_222>(),
            // std::make_shared<user::ejm_scan_223_i0>(),
            // std::make_shared<user::ejm_scan_223_i1>(),
            // std::make_shared<user::ejm_scan_223_i2>(),
            // std::make_shared<user::ejm_scan_223_i3>(),
            // std::make_shared<user::ejm_scan_223_i4>(),
            // std::make_shared<user::ejm_scan_223_i5>(),
            // std::make_shared<user::ejm_scan_223_i6>(),
            // std::make_shared<user::ejm_scan_223_i7>(),
            // std::make_shared<user::ejm_scan_223_i8>(),
            // std::make_shared<user::ejm_scan_223_i9>(),
            // SRB
            std::make_shared<user::srb>(),
            std::make_shared<user::srb_sym>(),
            std::make_shared<user::srb_eval>(),
            std::make_shared<user::srb_vis_detailed_test>(),
            std::make_shared<user::srb_vis_222_weak>(),
            std::make_shared<user::srb_vis_222_strong>(),
            std::make_shared<user::srb_dual_vector_io>(),
            std::make_shared<user::srb_vis_223_weak>(),
            std::make_shared<user::srb_vis_223_strong>(),
            std::make_shared<user::srb_vis_233_weak>(),
            std::make_shared<user::srb_vis_233_inter>(),
            std::make_shared<user::srb_vis_233_strong>(),
            std::make_shared<user::srb_vis_333>(),
            std::make_shared<user::srb_vis_334>(),
            // Y Basis
            std::make_shared<user::y_basis>(),
            std::make_shared<user::y_basis_sym>(),
            // PJM
            std::make_shared<user::pjm>(),
        };

    return application_list;
}
