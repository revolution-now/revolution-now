/****************************************************************
**igame-storage.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: Interface for saving/loading files.
*
*****************************************************************/
#pragma once

// base
#include "base/valid.hpp"

// C++ standard library
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace rn {

/****************************************************************
** IGameStorageQuery
*****************************************************************/
// Interface for querying about saved game files. The idea is
// that subclasses will deal with different formats.
struct IGameStorageQuery {
  virtual ~IGameStorageQuery() = default;

  // Should not include the dot.
  virtual std::string_view extension() const = 0;

  // Attempts to give a one-line short description of the save
  // file for display on the load/save dialog.
  virtual std::string description( fs::path const& p ) const = 0;
};

/****************************************************************
** IGameStorageSave
*****************************************************************/
// Interface for saving games to files. The idea is that sub-
// classes will deal with different formats.
struct IGameStorageSave : public IGameStorageQuery {
  virtual base::valid_or<std::string> store(
      fs::path const& p ) const = 0;
};

/****************************************************************
** IGameStorageLoad
*****************************************************************/
// Interface for loading games to files. The idea is that sub-
// classes will deal with different formats.
struct IGameStorageLoad : public IGameStorageQuery {
  virtual base::valid_or<std::string> load(
      fs::path const& p ) const = 0;
};

} // namespace rn
