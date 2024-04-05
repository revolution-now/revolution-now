/****************************************************************
**registrars.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: Loads the testing config.
*
*****************************************************************/
// config modules.
#include "rds/testing.rds.hpp"

// other config files.
#include "config/registrar-helper.hpp"

namespace rds {

INSTANTIATE_CONFIG( rn::test, testing );

} // namespace rds
