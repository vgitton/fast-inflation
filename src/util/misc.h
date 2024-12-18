#include <string>
#include <vector>

namespace util {

/*! \ingroup misc
 * \brief The STL pop_back() function does not return the value of the element popped,
 * this one does. */
template <typename T>
T pop_back(std::vector<T> &container) {
    T ret = container.back();
    container.pop_back();
    return ret;
}

} // namespace util
