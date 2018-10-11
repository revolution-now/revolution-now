/****************************************************************
**non-copyable.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Simple way to classes to specify and enforce
*              their copy/move semantics.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace rn {

// This is a base class used  for  classes  that  should  not  be
// copied but can be move constructed.
struct movable_only {
  movable_only() = default;

  movable_only( movable_only const& ) = delete;
  movable_only& operator=( movable_only const& ) = delete;

  movable_only( movable_only&& ) = default;
  movable_only& operator=( movable_only&& ) = default;
};

// This is a base class used  for  classes  that  should  not  be
// moved but can be copy constructed.
struct copyable_only {
  copyable_only() = default;

  copyable_only( copyable_only const& ) = default;
  copyable_only& operator=( copyable_only const& ) = default;

  copyable_only( copyable_only&& ) = delete;
  copyable_only& operator=( copyable_only&& ) = delete;
};

// Class for singletons to inherit from:  no  copying  or moving.
struct singleton : public copyable_only, public movable_only {};

} // namespace rn
