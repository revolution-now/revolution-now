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
#include "drag-drop.hpp"
#include "input.hpp"
#include "view.hpp"
#include "wait.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/vocab.hpp"

namespace rn {

struct SS;
struct SSConst;
struct TS;

struct Colony;
struct ColonyProduction;
struct IEngine;
struct IGui;
struct Player;
struct TerrainState;
struct UnitsState;

/****************************************************************
** Macros
*****************************************************************/
#define CONVERT_ENTITY( to, from_entity )                       \
  UNWRAP_CHECK( to, refl::enum_from_integral<e_colview_entity>( \
                        from_entity ) );

/****************************************************************
** Interfaces
*****************************************************************/
// TODO: Keep this generic and move it into the ui namespace
// eventually, and dedupe it with the one in harbor view. Also,
// need to figure out how its methods override (or not) the
// methods in ui::Object that accept the same events.
struct AwaitView {
  virtual ~AwaitView() = default;

  virtual wait<base::NoDiscard<bool>> perform_key(
      input::key_event_t const& ) {
    co_return false; // not handled.
  }

  virtual wait<base::NoDiscard<bool>> perform_click(
      input::mouse_button_event_t const& ) {
    co_return false; // not handled.
  }
};

class ColonySubView
  : public IDraggableObjectsView<ColViewObject>,
    public AwaitView {
 public:
  ColonySubView( IEngine& engine, SS& ss, TS& ts, Player& player,
                 Colony& colony )
    : engine_( engine ),
      ss_( ss ),
      ts_( ts ),
      player_( player ),
      colony_( colony ) {}

  // All ColonySubView's will also be unspecified subclassess of
  // ui::View.
  virtual ui::View& view() noexcept             = 0;
  virtual ui::View const& view() const noexcept = 0;

  // Implement IDraggableObjectsView.
  virtual maybe<PositionedDraggableSubView<ColViewObject>>
  view_here( Coord ) override {
    return PositionedDraggableSubView<ColViewObject>{ this,
                                                      Coord{} };
  }

  // Implement IDraggableObjectsView.
  virtual maybe<DraggableObjectWithBounds<ColViewObject>>
  object_here( Coord const& /*where*/ ) const override {
    return nothing;
  }

  // This will update any internal state held inside the view
  // that needs to be recomputed if any state external to the
  // view changes. Note: this assumes that the screen size has
  // not been changed. If the screen size has been changed, then
  // we need to do a full recompositing instead. This is not sup-
  // posed to be called directly, but should only be called via
  // the update_colony_view method.
  virtual void update_this_and_children();

 protected:
  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  Player& player_;
  Colony& colony_;
};

// The pointer returned from these will be invalidated if
// set_colview_colony is called with a new colony id.
ColonySubView& colview_top_level();

// FIXME: global state.
ColonyProduction const& colview_production();

void update_colony_view( SSConst const& ss,
                         Colony const& colony );

void update_production( SSConst const& ss,
                        Colony const& colony );

// Must be called before any other method in this module.
void set_colview_colony( IEngine& engine, SS& ss, TS& ts,
                         Player& player, Colony& colony );

void colview_drag_n_drop_draw(
    SS& ss, rr::Renderer& renderer,
    DragState<ColViewObject> const& state,
    Coord const& canvas_origin );

// TODO: temporary.
inline auto const BROWN_COLOR =
    gfx::pixel::parse_from_hex( "f1cf81" ).value().shaded( 14 );

} // namespace rn
