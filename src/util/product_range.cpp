#include "product_range.h"

template <typename T>
util::ProductIterator<T>::ProductIterator(std::vector<T> const &bases)
    : m_last_party(bases.size() - 1),
      m_event(bases.size(), T(0)),
      m_last_outcomes(bases),
      m_is_not_done(true) {
    // For convenience in util::ProductIterator::operator++
    for (T &last_outcome : m_last_outcomes) {
        --last_outcome;
    }
}

template <typename T>
util::ProductIterator<T> &util::ProductIterator<T>::operator++() {
    size_t i(0);
    for (; i <= m_last_party; ++i) {
        if (m_event[m_last_party - i] == m_last_outcomes[m_last_party - i]) {
            m_event[m_last_party - i] = 0;
        } else {
            break;
        }
    }

    if (i <= m_last_party) {
        ++(m_event[m_last_party - i]);
    } else {
        m_is_not_done = false;
    }

    return *this;
}

template <typename T>
std::vector<T> const &util::ProductIterator<T>::operator*() const {
    return m_event;
}

template <typename T>
util::ProductRange<T>::ProductRange(size_t n_parties, T base)
    : m_bases(n_parties, base) {}

template <typename T>
util::ProductRange<T>::ProductRange(std::vector<T> const &bases)
    : m_bases(bases) {}

template <typename T>
util::ProductIterator<T> util::ProductRange<T>::begin() const {
    return util::ProductIterator<T>(m_bases);
}

template class util::ProductIterator<unsigned char>;
template class util::ProductIterator<size_t>;
template class util::ProductRange<unsigned char>;
template class util::ProductRange<size_t>;
