#include "rand.h"
#include "../types.h"

#include <chrono>

template <typename T>
util::RNG<T>::RNG(T min_value, T max_value)
    : m_generator(std::chrono::system_clock::now().time_since_epoch().count()),
      m_distribution(min_value, max_value) {}

template <typename T>
T util::RNG<T>::get_rand() {
    return m_distribution(m_generator);
}

template class util::RNG<unsigned char>;
template class util::RNG<Num>;
template class util::RNG<Index>;
