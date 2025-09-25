/****************************************************************
**omni.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: A plane that is always on the top of the stack.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <memory>

namespace rn {

struct IEngine;
struct IPlane;
struct IMenuServer;

/****************************************************************
** Mouse cursor.
*****************************************************************/
enum class e_mouse_cursor {
  standard,
  go_to,
};

/****************************************************************
** OmniPlane
*****************************************************************/
struct OmniPlane {
  OmniPlane( IEngine& engine, IMenuServer& menu_server );
  ~OmniPlane();

  void set_mouse_cursor( e_mouse_cursor cursor );
  e_mouse_cursor get_mouse_cursor() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  IPlane& impl();
};

} // namespace rn
