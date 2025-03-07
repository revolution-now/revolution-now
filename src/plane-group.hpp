/****************************************************************
**plane-group.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-17.
*
* Description: An IPlaneGroup implementation structured so as to
*              suit typically plane layouts in the game.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "attributes.hpp"
#include "iplane-group.hpp"

// base
#include "base/meta.hpp"

// C++ standard library
#include <vector>

namespace rn {

struct IPlane;

struct OmniPlane;
struct ConsolePlane;
struct WindowPlane;
struct IMenuServer;
struct PanelPlane;
struct ILandViewPlane;

/****************************************************************
** TypedIPlane
*****************************************************************/
struct UntypedIPlane {
  void set_untyped( IPlane* plane );

  bool has_untyped() const { return untyped_ != nullptr; }

  IPlane& untyped() const {
    CHECK( untyped_ != nullptr );
    return *untyped_;
  }

 private:
  IPlane* untyped_ = {};
};

template<typename T>
struct TypedIPlane : UntypedIPlane {
  TypedIPlane() = default;

  operator bool() const {
    return typed_ != nullptr && has_untyped();
  }

  operator T&() const { return typed(); }

  T& typed() const {
    CHECK( typed_ != nullptr );
    return *typed_;
  }

  TypedIPlane& operator=( T& p ATTR_LIFETIMEBOUND ) {
    typed_ = &p;
    set_untyped( &p.impl() );
    CHECK( has_untyped() );
    return *this;
  }

 private:
  T* typed_ = {};
};

/****************************************************************
** Bottom Planes
*****************************************************************/
using BottomPlaneTypes =
    mp::list<IPlane*, TypedIPlane<ILandViewPlane>>;

using BottomPlaneVariant = mp::to_variant_t<BottomPlaneTypes>;

template<typename T>
concept IsTypedBottomType =
    mp::list_contains_v<BottomPlaneTypes, TypedIPlane<T>>;

/****************************************************************
** PlaneGroup
*****************************************************************/
// This holds a group of planes. Order is from top to bottom,
// i.e. [0] is above [1], and header is above midtier.
struct PlaneGroup : IPlaneGroup {
  TypedIPlane<OmniPlane> omni;
  TypedIPlane<ConsolePlane> console;
  TypedIPlane<WindowPlane> window;

  TypedIPlane<IMenuServer> menu;
  TypedIPlane<PanelPlane> panel;

  // This one basically defines which general view we're in.
  BottomPlaneVariant bottom;

  void set_bottom( IPlane& p ATTR_LIFETIMEBOUND );
  IPlane* get_bottom() const;

  bool menus_enabled = false;

  template<IsTypedBottomType T>
  void set_bottom( T& p ATTR_LIFETIMEBOUND ) {
    bottom.emplace<TypedIPlane<T>>() = p;
  }

  template<IsTypedBottomType T>
  T& get_bottom() const {
    TypedIPlane<T> const* res =
        std::get_if<TypedIPlane<T>>( &bottom );
    CHECK( res );
    return res->typed();
  }

 public: // IPlaneGroup
  std::vector<IPlane*> planes() const override;
};

} // namespace rn
