/****************************************************************
**catch-common.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-23.
*
* Description: To be included as the last header in each transla-
* tion unit.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "expect.hpp"
#include "fmt-helper.hpp"
#include "maybe.hpp"

// base
#include "base/valid.hpp"

// base-util
#include "base-util/mp.hpp"
#include "base-util/pp.hpp"

// Catch2
#include "catch2/catch.hpp"

// C++ standard library
#include <optional>
#include <utility>

// Use this to teach Catch2 how to convert types to strings that
// are a) non-templated, and b) can be printed using {fmt}.
#define FMT_TO_CATCH( ... )                                  \
  namespace Catch {                                          \
  template<>                                                 \
  struct StringMaker<__VA_ARGS__> {                          \
    static std::string convert( __VA_ARGS__ const& value ) { \
      return fmt::format( "{}", value );                     \
    }                                                        \
  };                                                         \
  }

#define RN_HAS_FMT_FORMAT( type ) ::rn::has_fmt<type>

#define FMT_TO_CATCH_T_IMPL( t_args, type )                    \
  namespace Catch {                                            \
  template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND t_args )>    \
  requires( mp::and_v<PP_MAP_COMMAS( RN_HAS_FMT_FORMAT,        \
                                     EXPAND t_args )> ) struct \
      StringMaker<type<EXPAND t_args>> {                       \
    static std::string convert(                                \
        type<EXPAND t_args> const& value ) {                   \
      return fmt::format( "{}", value );                       \
    }                                                          \
  };                                                           \
  }

// Use this to teach Catch2 how to convert types to strings that
// are a) templated, and b) can be printed using {fmt}.
#define FMT_TO_CATCH_T( t_args, type ) \
  EVAL( FMT_TO_CATCH_T_IMPL( t_args, type ) )

namespace rn {} // namespace rn

/****************************************************************
** Standard Formatting
*****************************************************************/
FMT_TO_CATCH_T( ( T, U ), ::std::pair );
FMT_TO_CATCH_T( ( T ), ::std::reference_wrapper );
FMT_TO_CATCH_T( ( T ), ::std::optional );
FMT_TO_CATCH_T( ( T ), ::base::maybe );
FMT_TO_CATCH_T( ( T, E ), ::base::expect );
FMT_TO_CATCH_T( ( E ), ::base::valid_or );
FMT_TO_CATCH( ::base::valid_t );
FMT_TO_CATCH( ::base::nothing_t );
