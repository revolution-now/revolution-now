/****************************************************************
**colview-entities.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-12.
*
* Description: The various UI sections/entities in Colony view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "colview-entities.rds.hpp"

// Revolution Now
#include "cargo.hpp"
#include "colony-id.hpp"
#include "commodity.hpp"
#include "dragdrop.hpp"
#include "input.hpp"
#include "view.hpp"
#include "wait.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/any-util.hpp"

namespace rn {

struct SS;
struct SSConst;
struct TS;

struct Colony;
struct ColonyProduction;
struct IGui;
struct Player;
struct TerrainState;
struct UnitsState;

/****************************************************************
** Macros
*****************************************************************/
// This will take a std::any and extract either a ColViewObject_t
// from it or any of its alternative types, and check-fail if it
// doesn't contain any of them.
#define UNWRAP_DRAGGABLE( v, std_any )                 \
  ColViewObject_t const v =                            \
      base::extract_variant_from_any<ColViewObject_t>( \
          std_any );

#define CONVERT_ENTITY( to, from_entity )                       \
  UNWRAP_CHECK( to, refl::enum_from_integral<e_colview_entity>( \
                        from_entity ) );

/****************************************************************
** Interfaces
*****************************************************************/
// TODO: Keep this generic and move it into the ui namespace
// eventually.
struct AwaitView {
  virtual ~AwaitView() = default;

  virtual wait<> perform_click(
      input::mouse_button_event_t const& ) {
    return make_wait<>();
  }
};

class ColonySubView : public IDraggableObjectsView,
                      public AwaitView {
 public:
  ColonySubView( SS& ss, TS& ts, Colony& colony )
    : ss_( ss ), ts_( ts ), colony_( colony ) {}

  // All ColonySubView's will also be unspecified subclassess of
  // ui::View.
  virtual ui::View&       view() noexcept       = 0;
  virtual ui::View const& view() const noexcept = 0;

  // Implement IDraggableObjectsView.
  virtual maybe<PositionedDraggableSubView> view_here(
      Coord ) override {
    return PositionedDraggableSubView{ this, Coord{} };
  }

  // Implement IDraggableObjectsView.
  virtual maybe<DraggableObjectWithBounds> object_here(
      Coord const& /*where*/ ) const override {
    return nothing;
  }

  // This will update any internal state held inside the view
  // that needs to be recomputed if any state external to the
  // view changes. Note: this assumes that the screen size has
  // not been changed. If the screen size has not been changed,
  // then we need to do a full recompositing instead. This is not
  // supposed to be called directly, but should only be called
  // via the update_colony_view method.
  virtual void update_this_and_children();

 protected:
  SS&     ss_;
  TS&     ts_;
  Colony& colony_;
};

// The pointer returned from these will be invalidated if
// set_colview_colony is called with a new colony id.
ColonySubView& colview_entity( e_colview_entity entity );
ColonySubView& colview_top_level();

// FIXME: global state.
ColonyProduction const& colview_production();

void update_colony_view( SSConst const& ss,
                         Colony const&  colony );

void update_production( SSConst const& ss,
                        Colony const&  colony );

// Must be called before any other method in this module.
void set_colview_colony( SS& ss, TS& ts, Colony& colony );

void colview_drag_n_drop_draw( SS& ss, rr::Renderer& renderer,
                               DragState const& state,
                               Coord const&     canvas_origin );

} // namespace rn
