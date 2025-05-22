/****************************************************************
**cdr.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-17.
*
* Description: Converts between CDR and Lua data structures.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"
#include "rstring.hpp"
#include "rtable.hpp"

// cdr
#include "cdr/repr.hpp"

namespace lua {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct state;

/****************************************************************
** LuaToCdrConverter
*****************************************************************/
// This is used to do a deep conversion of a CDR data structure
// into a native Lua data structure.  This is not intended to
// be used normally to expose C++ types (even CDR-convertible
// ones) to Lua; for that, Lua should just take a userdata
// reference to a value owned by C++, or it should take a copy of
// the C++ object held via Lua userdata reference. It is not
// normally intended to be given a Lua-native replica of the
// object as this converter does, since that would be too slow
// are memory intensive.
//
// So what is this used for? At the time of writing it is used to
// replicate the global config_* objects into Lua. This seems
// like the simplest approach to exposing those configs to Lua
// given that they are already CDR-convertible and they don't
// change after loading (meaning the conversion only needs to be
// done once).
//
// Other use cases for this may arise, but this probably
// shouldn't be used too often for performance reasons.
struct LuaToCdrConverter {
  LuaToCdrConverter( state& st );

  any convert( cdr::value const& o ) const;

  table convert( cdr::table const& o ) const;

  table convert( cdr::list const& o ) const;

  rstring convert( std::string const& o ) const;

  any convert( cdr::integer_type o ) const;

  any convert( double o ) const;

  any convert( bool o ) const;

  any convert( cdr::null_t const& o ) const;

 private:
  state& st_;
};

} // namespace lua
