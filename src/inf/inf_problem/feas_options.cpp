#include "feas_options.h"
#include "../../util/logger.h"

// Options

inf::FeasOptions::FeasOptions()
    : m_inflation_size{},
      m_constraint_set_description{},
      m_search_mode(inf::Optimizer::SearchMode::tree_search),
      m_use_distr_symmetries(inf::Inflation::UseDistrSymmetries::yes),
      m_stop_mode(inf::Optimizer::StopMode::opt),
      m_fw_algo(inf::FrankWolfe::Algo::fully_corrective),
      m_store_bounds(inf::DualVector::StoreBounds::yes),
      m_n_threads(1),
      m_symtree_io(inf::EventTree::IO::none) {}

void inf::FeasOptions::log() const {
    LOG_BEGIN_SECTION("inf::FeasOptions")

    util::logger << "Inflation size: " << m_inflation_size << util::cr
                 << "Constraints:" << util::cr;
    for (inf::Constraint::Description const &constraint_description : m_constraint_set_description) {
        util::logger << "    " << inf::Constraint::pretty_description(constraint_description) << util::cr;
    }
    util::logger << "Options:" << util::cr << "    ";
    inf::Optimizer::log(m_search_mode);
    util::logger << util::cr << "    ";
    inf::Inflation::log(m_use_distr_symmetries);
    util::logger << util::cr << "    ";
    inf::Optimizer::log(m_stop_mode);
    util::logger << util::cr << "    ";
    inf::FrankWolfe::log(m_fw_algo);
    util::logger << util::cr << "    ";
    inf::DualVector::log(m_store_bounds);
    util::logger << util::cr << "    ";
    util::logger << util::begin_comment << "n_threads = " << util::end_comment;
    util::logger << m_n_threads;
    util::logger << util::cr << "    ";
    inf::EventTree::log(m_symtree_io);
    util::logger << util::cr;

    LOG_END_SECTION
}

// Setters

inf::FeasOptions &inf::FeasOptions::set(inf::Inflation::Size const &inflation_size) {
    m_inflation_size = inflation_size;
    return *this;
}

inf::FeasOptions &inf::FeasOptions::set(inf::ConstraintSet::Description const &constraint_set_description) {
    m_constraint_set_description = constraint_set_description;
    return *this;
}

inf::FeasOptions &inf::FeasOptions::set(inf::Optimizer::SearchMode search_mode) {
    m_search_mode = search_mode;
    return *this;
}

inf::FeasOptions &inf::FeasOptions::set(inf::Inflation::UseDistrSymmetries use_distr_symmetries) {
    m_use_distr_symmetries = use_distr_symmetries;
    return *this;
}

inf::FeasOptions &inf::FeasOptions::set(inf::Optimizer::StopMode stop_mode) {
    m_stop_mode = stop_mode;
    return *this;
}

inf::FeasOptions &inf::FeasOptions::set(inf::FrankWolfe::Algo fw_algo) {
    m_fw_algo = fw_algo;
    return *this;
}

inf::FeasOptions &inf::FeasOptions::set(inf::DualVector::StoreBounds store_bounds) {
    m_store_bounds = store_bounds;
    return *this;
}

inf::FeasOptions &inf::FeasOptions::set_n_threads(Index n_threads) {
    m_n_threads = n_threads;
    return *this;
}

inf::FeasOptions &inf::FeasOptions::set(inf::EventTree::IO symtree_io) {
    m_symtree_io = symtree_io;
    return *this;
}

// Getters

inf::Inflation::Size inf::FeasOptions::get_inflation_size() const {
    return m_inflation_size;
}

inf::ConstraintSet::Description const &inf::FeasOptions::get_constraint_set_description() const {
    return m_constraint_set_description;
}

inf::Optimizer::SearchMode inf::FeasOptions::get_search_mode() const {
    return m_search_mode;
}

inf::Inflation::UseDistrSymmetries inf::FeasOptions::get_use_distr_symmetries() const {
    return m_use_distr_symmetries;
}

inf::Optimizer::StopMode inf::FeasOptions::get_stop_mode() const {
    return m_stop_mode;
}

inf::FrankWolfe::Algo inf::FeasOptions::get_fw_algo() const {
    return m_fw_algo;
}

inf::DualVector::StoreBounds inf::FeasOptions::get_store_bounds() const {
    return m_store_bounds;
}

Index inf::FeasOptions::get_n_threads() const {
    return m_n_threads;
}

inf::EventTree::IO inf::FeasOptions::get_symtree_io() const {
    return m_symtree_io;
}
