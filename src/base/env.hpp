/****************************************************************
**env.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: Environment variables.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "maybe.hpp"

// C++ standard library
#include <string>
#include <string_view>

namespace base {

// Get the value of an environment variable, if available.
maybe<std::string_view> env_var( char const* name );

// If the HOME environment variable is set then return it.
maybe<std::string_view> user_home_folder();

// Get the number of columns in the terminal from which the pro-
// gram was launched, if available.
maybe<int> os_terminal_columns();

// Set an environment variable, always overwriting.
void set_env_var( char const* var_name, char const* value );

// Set an environment variable, never overwriting.
void set_env_var_if_not_set( char const* var_name,
                             char const* value );

void unset_env_var( char const* name );

} // namespace base
