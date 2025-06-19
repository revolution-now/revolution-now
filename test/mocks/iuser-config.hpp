/****************************************************************
**iuser-config.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-17.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/iuser-config.hpp"

// mock
#include "src/mock/mock.hpp"

namespace rn {

/****************************************************************
** MockIEngine
*****************************************************************/
struct MockIUserConfig : IUserConfig {
  MOCK_METHOD( config_user_t const&, read, (), ( const ) );
  MOCK_METHOD( bool, modify,
               (base::function_ref<IUserConfig::WriteFn>), () );
  MOCK_METHOD( base::valid_or<std::string>, flush, (), () );
};

static_assert( !std::is_abstract_v<MockIUserConfig> );

} // namespace rn
