/****************************************************************
**terminal.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-25.
*
* Description: Backend for lua terminal.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"

namespace rn::term {

void log( std::string const& msg );
void log( std::string&& msg );

ND expect<> run_cmd( std::string const& cmd );

void clear();

// idx zero is most recent.
OptCRef<Str> line( int idx );

// idx zero is most recent.
OptCRef<Str> history( int idx );

// Given a fragment of Lua this will return a vector of all pos-
// sible (immediate) completions. If it returns an empty vector
// then that means the fragment is invalid (i.e., it is not a
// prefix of any valid completion).
Vec<Str> autocomplete( std::string_view fragment );

// Will keep autocompleting so long as there is a single result,
// until the result converges and stops changing.
Vec<Str> autocomplete_iterative( std::string_view fragment );

} // namespace rn::term
