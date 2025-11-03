/****************************************************************
**terminal.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-25.
*
* Description: Backend for the game terminal.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "expect.hpp"
#include "maybe.hpp"

// luapp
#include "luapp/error.hpp"

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
  Terminal();
  ~Terminal();

  // This function is thread safe.
  void log( std::string_view msg );

  lua::lua_valid run_cmd( lua::state& state,
                          std::string const& cmd );

  // This function is thread safe.
  void clear();

  // idx zero is most recent. This function is thread safe.
  maybe<std::string const&> line( int idx );

  // idx zero is most recent.
  maybe<std::string const&> history( int idx );

  void push_history( std::string const& what );

 private:
  void trim();

  std::vector<std::string> history_;
  // The g_buffer MUST ONLY be accessed while holding the below
  // mutex because it can be modified by multiple threads by way
  // of the logging framework.
  std::mutex buffer_mutex_;
  std::vector<std::string> buffer_;
};

} // namespace rn
