#include "frank_wolfe.h"

#include "fully_corrective_fw.h"
#include "pairwise_fw.h"

#include "../../util/debug.h"
#include "../../util/logger.h"
#include "../../util/math.h"

util::Chrono inf::FrankWolfe::total_fw_chrono(util::Chrono::State::paused);

void inf::FrankWolfe::log(inf::FrankWolfe::Algo algo) {
    util::logger << util::begin_comment << "inf::FrankWolfe::Algo::"
                 << util::end_comment;
    switch (algo) {
    case inf::FrankWolfe::Algo::fully_corrective:
        util::logger << "fully_corrective";
        break;
    case inf::FrankWolfe::Algo::pairwise:
        util::logger << "pairwise";
        break;
    default:
        THROW_ERROR("switch")
    }
}

inf::FrankWolfe::UniquePtr inf::FrankWolfe::get_frank_wolfe(inf::FrankWolfe::Algo algo,
                                                            Index dimension,
                                                            Index n_threads) {
    switch (algo) {
    case inf::FrankWolfe::Algo::fully_corrective:
        return std::make_unique<inf::FullyCorrectiveFW>(dimension, n_threads);
    case inf::FrankWolfe::Algo::pairwise:
        return std::make_unique<inf::PairwiseFW>(dimension);
    default:
        THROW_ERROR("unimplemented")
    }
}

inf::FrankWolfe::Solution::Solution(double s,
                                    std::vector<double> const &vec,
                                    bool valid)
    : s(s),
      vec(vec),
      valid(valid) {}

void inf::FrankWolfe::Solution::log() const {
    LOG_BEGIN_SECTION("inf::FrankWolfe::Solution")

    util::logger << util::begin_comment << "      s = " << util::end_comment << s;
    if (s == 0.0)
        util::logger << util::Color::alt_yellow << " -> FEASIBLE/INCONCLUSIVE" << util::Color::fg;

    util::logger << util::cr
                 << util::begin_comment << "    vec = " << util::end_comment << "(";
    for (Index const i : util::Range(std::min(vec.size(), Index(6)))) {
        if (i > 0)
            util::logger << ", ";
        util::logger << vec[i];
    }
    if (vec.size() > 6)
        util::logger << "...";

    util::logger << ")                   " << util::cr
                 << util::begin_comment << "|vec|^2 = " << util::end_comment
                 << util::inner_product(vec, vec) << util::cr;
    util::logger << util::begin_comment << "  valid = " << util::end_comment
                 << valid << util::cr;

    LOG_END_SECTION
}

inf::FrankWolfe::FrankWolfe(Index dimension)
    : m_dimension(dimension) {
    inf::FrankWolfe::total_fw_chrono.reset();
}

void inf::FrankWolfe::memorize_event_and_quovec(inf::Event const &event,
                                                std::vector<Num> const &quovec,
                                                double denom) {
    ASSERT_EQUAL(quovec.size(), m_dimension)

    std::vector<double> quovec_double(m_dimension);
    for (Index const i : util::Range(m_dimension))
        quovec_double[i] = static_cast<double>(quovec[i]) / denom;

    this->memorize_event_and_quovec_double(event, quovec_double);
}

inf::FrankWolfe::Solution inf::FrankWolfe::time_and_solve() {
    HARD_ASSERT_LT(0, get_n_stored_events())

    inf::FrankWolfe::total_fw_chrono.start();
    inf::FrankWolfe::Solution ret = solve();
    inf::FrankWolfe::total_fw_chrono.pause();

    return ret;
}
