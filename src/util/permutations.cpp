#include "permutations.h"
#include <algorithm>

template <typename T>
util::PermIterator<T>::PermIterator(T size)
    : m_perm(size),
      m_is_not_done(true) {
    for (size_t i(0); i < m_perm.size(); ++i) {
        m_perm[i] = static_cast<T>(i);
    }
}

template <typename T>
util::PermIterator<T> &util::PermIterator<T>::operator++() {
    m_is_not_done = std::next_permutation(m_perm.begin(), m_perm.end());
    return *this;
}

template <typename T>
std::vector<T> const &util::PermIterator<T>::operator*() const {
    return m_perm;
}

template <typename T>
util::PermIterator<T> util::Permutations<T>::begin() const {
    return util::PermIterator<T>(m_size);
}

template class util::PermIterator<unsigned char>;
template class util::PermIterator<size_t>;
template class util::Permutations<unsigned char>;
template class util::Permutations<size_t>;
