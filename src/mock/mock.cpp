/****************************************************************
**mock.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-11.
*
* Description: Mocking Framework.
*
*****************************************************************/
#include "mock.hpp"

using namespace ::std;

namespace mock {

/****************************************************************
** Mock Config
*****************************************************************/
MockConfig g_mock_config;

MockConfig::binder::binder( MockConfig config )
  : old_config_( std::move( g_mock_config ) ) {
  g_mock_config = std::move( config );
}

MockConfig::binder::~binder() {
  g_mock_config = std::move( old_config_ );
}

/****************************************************************
** Mocking
*****************************************************************/
namespace detail {

void throw_unexpected_error( string_view msg ) {
  if( g_mock_config.throw_on_unexpected ) {
    throw invalid_argument( string( msg ) );
  } else {
    FATAL( "{}", msg );
  }
}

} // namespace detail

} // namespace mock
