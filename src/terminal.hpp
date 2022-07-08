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
#include "expect.hpp"
#include "maybe.hpp"

// C++ standard library
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace lua {
struct state;
}

namespace rn {

struct Terminal {
  Terminal( lua::state& st );
  ~Terminal();

  // This function is thread safe.
  void log( std::string_view msg );

  valid_or<std::string> run_cmd( std::string const& cmd );

  // This function is thread safe.
  void clear();

  // idx zero is most recent. This function is thread safe.
  maybe<std::string const&> line( int idx );

  // idx zero is most recent.
  maybe<std::string const&> history( int idx );

  void push_history( std::string const& what );

  // Given a fragment of Lua this will return a vector of all
  // possible (immediate) completions. If it returns an empty
  // vector then that means the fragment is invalid (i.e., it is
  // not a prefix of any valid completion).
  std::vector<std::string> autocomplete(
      std::string_view fragment );

  // Will keep autocompleting so long as there is a single re-
  // sult, until the result converges and stops changing.
  std::vector<std::string> autocomplete_iterative(
      std::string_view fragment );

  lua::state& lua_state() { return st_; }

 private:
  void trim();

  lua::state&              st_;
  std::vector<std::string> history_;
  // The g_buffer MUST ONLY be accessed while holding the below
  // mutex because it can be modified by multiple threads by way
  // of the logging framework.
  std::mutex               buffer_mutex_;
  std::vector<std::string> buffer_;
};

} // namespace rn
