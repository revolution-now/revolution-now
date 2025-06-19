/****************************************************************
**user-config.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-17.
*
* Description: Main implementation of IUserConfig.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "iuser-config.hpp"

// config
#include "config/user.rds.hpp"

// base
#include "base/valid.hpp"

namespace rn {

/****************************************************************
** UserConfig
*****************************************************************/
// Normal game code should not include or refer to this directly;
// instead the IEngine interface should be used.
struct UserConfig : IUserConfig {
  UserConfig();

  // This must be called after construction if we want to try to
  // load from the existing settings file. This will attempt to
  // do the load and, if it succeeds, will subsequently flush to
  // that file. If it does not succeed the it will retain the de-
  // faults and ignore the file on subsequent flushes.
  base::valid_or<std::string> try_bind_to_file(
      std::string const& path );

 public: // IUserConfig
  config_user_t const& read() const override;

  [[nodiscard]] bool modify(
      base::function_ref<WriteFn> fn ) override;

  base::valid_or<std::string> flush() override;

 private:
  // Load in the defaults. This just reads the default values
  // from the in-memory config_user read from the static config
  // files, but not the user level settings file. Thus this can
  // never fail, since static configs should already have been
  // loaded by this point.
  void load_from_defaults();

  struct SettingsFile {
    std::string path;
    config_user_t last_snapshot;
  };

  std::optional<SettingsFile> settings_file_;
  // This class enforces an invariant where config_.validate()
  // must always be true.
  config_user_t config_;
};

} // namespace rn
