/****************************************************************
**rcl-game-storage.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: IGameStorage* implementations for the rcl format.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "igame-storage.hpp"
#include "maybe.hpp"
#include "save-game.hpp"

namespace rn {

struct SS;
struct SSConst;

enum class e_savegame_verbosity;

/****************************************************************
** RclGameStorageQuery
*****************************************************************/
struct RclGameStorageQueryImpl {
  std::string_view extension_impl() const;

  std::string description_impl( fs::path const& p ) const;
};

struct RclGameStorageQuery : public IGameStorageQuery,
                             private RclGameStorageQueryImpl {
 public: // IGameStorageQuery
  std::string_view extension() const override {
    return extension_impl();
  }

  std::string description( fs::path const& p ) const override {
    return description_impl( p );
  }
};

/****************************************************************
** RclGameStorageSave
*****************************************************************/
struct RclGameStorageSave : public IGameStorageSave,
                            private RclGameStorageQueryImpl {
  struct options {
    // When this is nothing it will use the default value
    // specified in the config files.
    maybe<e_savegame_verbosity> verbosity;
  };

  RclGameStorageSave( SSConst const& ss, options opts = {} )
    : ss_( ss ), opts_( opts ) {}

 public: // IGameStorageSave
  base::valid_or<std::string> store(
      fs::path const& p ) const override;

 public: // IGameStorageQuery
  std::string_view extension() const override {
    return extension_impl();
  }

  std::string description( fs::path const& p ) const override {
    return description_impl( p );
  }

 private:
  SSConst const& ss_;
  options        opts_ = {};
};

/****************************************************************
** RclGameStorageLoad
*****************************************************************/
struct RclGameStorageLoad : public IGameStorageLoad,
                            private RclGameStorageQueryImpl {
  struct options {};

  RclGameStorageLoad( SS& ss, options opts = {} )
    : ss_( ss ), opts_( opts ) {}

 public: // IGameStorageLoad
  base::valid_or<std::string> load(
      fs::path const& p ) const override;

 public: // IGameStorageQuery
  std::string_view extension() const override {
    return extension_impl();
  }

  std::string description( fs::path const& p ) const override {
    return description_impl( p );
  }

 private:
  SS&     ss_;
  options opts_ = {};
};

} // namespace rn
