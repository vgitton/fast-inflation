#include "user/app_manager.h"

/*! \file */

/*! \ingroup user
    \brief Creates a `user::AppManager` with the command-line arguments `argc, argv`
    \param argc
    \param argv
    \return 0
*/
int main(int argc, char **argv) {

    user::AppManager app_manager(argc, argv);
    (void)app_manager;

    return 0;
}
