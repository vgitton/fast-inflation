
#include "../application.h"

namespace user {

/*! \addtogroup misc_tests
 * @{
 * */

/*! \brief Trying to prove the nonlocality of the \f$Y\f$-basis distribution of Nicolas Gisin */
class y_basis : public user::Application {
  public:
    y_basis() : user::Application("y_basis", "Nonlocality test for the Y distribution", false) {}
    void run() override;
};

/*! \brief Tests the symmetrization of inflation problems for the \f$Y\f$-basis distribution of Nicolas Gisin
 * \details This tests the symmetrization mechanisms for a distribution with a symmetry group
 * that does not factorize as a product of a group of outcome symmetries with a group of parties symmetries */
class y_basis_sym : public user::Application {
  public:
    y_basis_sym() : user::Application("y_basis_sym", "Symmetry test for the Y distribution", false) {}
    void run() override;
};

/*! \brief Trying to prove the nonlocality of the PJM distribution of Nicolas Gisin */
class pjm : public user::Application {
  public:
    pjm() : user::Application("pjm", "Nonlocality test for the PJM distribution", false) {}
    void run() override;
};

//! @}

} // namespace user
