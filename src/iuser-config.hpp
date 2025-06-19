/****************************************************************
**iuser-config.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-17.
*
* Description: Interface for accessing the per-user config.
*
*****************************************************************/
#pragma once

// base
#include "base/function-ref.hpp"
#include "base/valid.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct config_user_t;

/****************************************************************
** IUserConfig
*****************************************************************/
struct IUserConfig {
  virtual ~IUserConfig() = default;

  virtual config_user_t const& read() const = 0;

  using WriteFn = void( config_user_t& ) const;

  // Runs the function to modify the settings IN-MEMORY only,
  // then checks validation. If validation fails then it returns
  // false without applying the changes. Otherwise it returns
  // true and applies the changes.
  //
  // NOTE: Although this call technically should be followed up
  // by a call to `flush` if the change should be persisted, the
  // program (at the time of writing) flushes when the engine
  // closes.
  [[nodiscard]] virtual bool modify(
      base::function_ref<WriteFn> fn ) = 0;

  // Try to save the current settings to the user settings file.
  virtual base::valid_or<std::string> flush() = 0;
};

} // namespace rn
