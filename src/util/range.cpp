#include "range.h"

template <typename T>
util::Iterator<T>::Iterator(T val)
    : m_val(val) {}

template <typename T>
util::Iterator<T> &util::Iterator<T>::operator++() {
    ++m_val;
    return *this;
}

template <typename T>
T util::Iterator<T>::operator*() const {
    return m_val;
}

template <typename T>
util::Range<T>::Range(T bound)
    : m_first(0),
      m_bound(bound) {}

template <typename T>
util::Range<T>::Range(T first, T bound)
    : m_first(first),
      m_bound(bound) {}

template <typename T>
util::Iterator<T> util::Range<T>::begin() const {
    return util::Iterator(m_first);
}

template <typename T>
util::Iterator<T> util::Range<T>::end() const {
    return util::Iterator(m_bound);
}

template class util::Range<unsigned char>;
template class util::Range<unsigned int>;
template class util::Range<int>;
template class util::Range<Index>;
template class util::Range<Num>;

template class util::Iterator<unsigned char>;
template class util::Iterator<unsigned int>;
template class util::Iterator<int>;
template class util::Iterator<Index>;
template class util::Iterator<Num>;
