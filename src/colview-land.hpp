/****************************************************************
**colview-land.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-05.
*
* Description: Land view UI within the colony view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colview-entities.hpp"

// ss
#include "ss/colony-enums.rds.hpp"
#include "ss/colony.rds.hpp"
#include "ss/dwelling-id.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;

enum class e_tile;

struct ColonyLandView : public ui::View,
                        public ColonySubView,
                        public IDragSource<ColViewObject>,
                        public IDragSink<ColViewObject>,
                        public IDragSinkCheck<ColViewObject> {
  enum class e_render_mode {
    // Three tiles by three tiles, with unscaled tiles and
    // colonists on the land files.
    _3x3,
    // Land is 3x3 with unscaled tiles, and there is a one-tile
    // wood border around it where colonists stay.
    _5x5,
    // Land is 3x3 tiles by each tile is scaled by a factor of
    // two to easily fit both colonists and commodities inline.
    _6x6
  };

  static std::unique_ptr<ColonyLandView> create(
      IEngine& engine, SS& ss, TS& ts, Player& player,
      Colony& colony, e_render_mode mode );

  ColonyLandView( IEngine& engine, SS& ss, TS& ts,
                  Player& player, Colony& colony,
                  e_render_mode mode );

  static Delta size_needed( e_render_mode mode );

  // Implement ui::Object.
  Delta delta() const override;

 private:
  maybe<e_direction> direction_under_cursor( Coord coord ) const;

  // Gets the bounding rectangle for the unit position on the
  // given square (whether there actually is a unit there or
  // not).
  Rect rect_for_unit( e_direction d ) const;

  maybe<UnitId> unit_for_direction( e_direction d ) const;

  maybe<e_outdoor_job> job_for_direction( e_direction d ) const;

  maybe<UnitId> unit_under_cursor( Coord where ) const;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View& view() noexcept override;

  ui::View const& view() const noexcept override;

  // Implement AwaitView.
  wait<base::NoDiscard<bool>> perform_click(
      input::mouse_button_event_t const& event ) override;

  // Implement IDragSink.
  maybe<CanReceiveDraggable<ColViewObject>> can_receive(
      ColViewObject const& o, int,
      Coord const& where ) const override;

  // Implement IDragSinkCheck.
  wait<base::valid_or<DragRejection>> sink_check(
      ColViewObject const&, int, Coord const where ) override;

  // Implement IDragSink.
  wait<> drop( ColViewObject const& o,
               Coord const& where ) override;

  ColonyJob make_job_for_square( e_direction d ) const;

  // Implement IDraggableObjectsView.
  maybe<DraggableObjectWithBounds<ColViewObject>> object_here(
      Coord const& where ) const override;

  struct Draggable {
    e_direction d     = {};
    e_outdoor_job job = {};
  };

  // Implement IDragSource.
  bool try_drag( ColViewObject const&,
                 Coord const& where ) override;

  // Implement IDragSource.
  void cancel_drag() override;

  // Implement IDragSource.
  wait<> disown_dragged_object() override;

  void draw_spread( rr::Renderer& renderer, gfx::rect box,
                    e_tile tile, int quantity,
                    bool is_colony_tile ) const;

  void draw_land_3x3( rr::Renderer& renderer,
                      Coord coord ) const;

  void draw_land_6x6( rr::Renderer& renderer,
                      Coord coord ) const;

  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  e_render_mode mode_;
  maybe<Draggable> dragging_;
  // These are squares that are occupied by colonists from other
  // colonies, either friendly or foreign. In the original game,
  // these tiles would have a red box drawn around them.
  refl::enum_map<e_direction, bool> occupied_red_box_;
  // These will be marked with totem poles indicating that the
  // land is owned by the natives.
  refl::enum_map<e_direction, maybe<DwellingId>>
      native_owned_land_;
};

} // namespace rn
