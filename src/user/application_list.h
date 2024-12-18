#pragma once

#include "application.h"

#include <vector>

/*! \file */

namespace user {

/*! \ingroup user
 * \brief Specifies the existing applications/examples
 * \details Modify this function in src/user/application_list.cpp to include additional applications.
 * \return A list of user::Application::Ptr to existing applications/examples */
std::vector<user::Application::Ptr> get_application_list();

} // namespace user
