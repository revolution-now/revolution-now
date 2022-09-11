/****************************************************************
**plane-stack.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: A (quasi) stack for storing the list of active
*              planes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "plane-stack.rds.hpp"

// Revolution Now
#include "input.hpp" // FIXME

// refl
#include "refl/enum-map.hpp"
#include "refl/ext.hpp"

// base
#include "base/macros.hpp"

// C++ standard library
#include <list>
#include <vector>

#define PLANE_ACCESSOR_DECL( type, name ) \
  type&       name();                     \
  type const& name() const;

namespace rr {
struct Renderer;
}

namespace rn {

/****************************************************************
** Forward Decls
*****************************************************************/
struct input_event;
struct IMapUpdater;
struct Plane;

struct OmniPlane;
struct ConsolePlane;
struct WindowPlane;
struct MainMenuPlane;
struct MenuPlane;
struct PanelPlane;
struct ILandViewPlane;
struct MapEditPlane;
struct ColonyPlane;
struct NewHarborPlane;

/****************************************************************
** PlaneGroup
*****************************************************************/
// These are pointers instead of maybe-ref because we want to be
// able to copy assign these.
struct PlaneGroup {
  OmniPlane*      omni;
  ConsolePlane*   console;
  WindowPlane*    window;
  MainMenuPlane*  main_menu;
  MenuPlane*      menu;
  PanelPlane*     panel;
  ILandViewPlane* land_view;
  MapEditPlane*   map_edit;
  ColonyPlane*    colony;
  NewHarborPlane* new_harbor;
};

maybe<Plane&> plane_pointer( PlaneGroup const& group,
                             e_plane           plane );

/****************************************************************
** Planes
*****************************************************************/
struct Planes {
  Planes();

  static constexpr int kNumPlanes = refl::enum_count<e_plane>;

 private:
  struct [[nodiscard]] popper {
    popper( Planes& planes ) : planes_( planes ) {}
    NO_COPY_NO_MOVE( popper );
    ~popper();
    Planes& planes_;
  };

 public:
  // To add a new plane group, first call this followed by back()
  // to get the PlaneGroup, then call push on the plane group.
  popper new_group();

  // Same as above, but initializes the new group by copying the
  // current top group if it exists.
  popper new_copied_group();

  PlaneGroup&       back();
  PlaneGroup const& back() const;

  void draw( rr::Renderer& renderer ) const;

  PLANE_ACCESSOR_DECL( OmniPlane, omni );
  PLANE_ACCESSOR_DECL( ConsolePlane, console );
  PLANE_ACCESSOR_DECL( WindowPlane, window );
  PLANE_ACCESSOR_DECL( MainMenuPlane, main_menu );
  PLANE_ACCESSOR_DECL( MenuPlane, menu );
  PLANE_ACCESSOR_DECL( PanelPlane, panel );
  PLANE_ACCESSOR_DECL( ILandViewPlane, land_view );
  PLANE_ACCESSOR_DECL( MapEditPlane, map_edit );
  PLANE_ACCESSOR_DECL( ColonyPlane, colony );
  PLANE_ACCESSOR_DECL( NewHarborPlane, new_harbor );

  // This will call the advance_state method on each plane to up-
  // date any state that it has. It will only be called on frames
  // that are enabled and visible.
  void advance_state();

  e_input_handled send_input( input::event_t const& event );

 private:
  // We want pointer stability here so that we can get a refer-
  // ence to the top group and then add a new one.
  std::list<PlaneGroup> groups_;

  std::vector<Plane*> active_planes() const;

  enum class e_drag_send_mode { normal, raw, motion };

  struct DragState {
    Plane*           plane;
    e_drag_send_mode mode = e_drag_send_mode::normal;
  };

  maybe<DragState> drag_state_;
};

} // namespace rn
