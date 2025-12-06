/****************************************************************
**colview-buildings.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Buildings view UI within the colony view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colview-entities.hpp"
#include "spread-render.rds.hpp"

// ss
#include "ss/colony-enums.rds.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;

struct BuildingLayoutSlot {
  gfx::rect bounds;
  TileSpreadRenderPlan product_plan;
  TileSpreadRenderPlan workers_plan;
  gfx::point product_plan_origin;
  gfx::point workers_plan_origin;
  std::map<UnitId, int /*idx*/> units;
};

/****************************************************************
** Buildings
*****************************************************************/
class ColViewBuildings : public ui::View,
                         public ColonySubView,
                         public IDragSink<ColViewObject>,
                         public IDragSinkCheck<ColViewObject>,
                         public IDragSource<ColViewObject> {
 public:
  static std::unique_ptr<ColViewBuildings> create(
      IEngine& engine, SS& ss, TS& ts, Player& player,
      Colony& colony, Delta size );

  struct Layout {
    gfx::size size = {};

    refl::enum_map<e_colony_building_slot, BuildingLayoutSlot>
        slots;
    int unit_shadow_offset = {};
  };

  ColViewBuildings( IEngine& engine, SS& ss, TS& ts,
                    Player& player, Colony& colony,
                    Layout layout )
    : ColonySubView( engine, ss, ts, player, colony ),
      colony_( colony ),
      layout_( std::move( layout ) ) {}

  Delta delta() const override { return layout_.size; }

  // Implement ui::object.
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::buildings );
  }

  // Implement ColonySubView.
  ui::View& view() noexcept override { return *this; }

  // Implement ColonySubView.
  ui::View const& view() const noexcept override {
    return *this;
  }

  // Implement IDragSink.
  maybe<CanReceiveDraggable<ColViewObject>> can_receive(
      ColViewObject const& o, int from_entity,
      Coord const& where ) const override;

  // Implement IDragSink.
  wait<> drop( ColViewObject const& o,
               Coord const& where ) override;

  // Implement IDragSinkCheck.
  wait<base::valid_or<DragRejection>> sink_check(
      ColViewObject const&, int from_entity,
      Coord const ) override;

  // Implement IDragSource.
  bool try_drag( ColViewObject const& o,
                 Coord const& where ) override;

  // Implement IDragSource.
  void cancel_drag() override;

  // Implement IDragSource.
  wait<> disown_dragged_object() override;

  // Implement ColonySubView.
  maybe<DraggableObjectWithBounds<ColViewObject>> object_here(
      Coord const& /*where*/ ) const override;

  // Implement AwaitView.
  wait<base::NoDiscard<bool>> perform_click(
      input::mouse_button_event_t const& event ) override;

  void update_this_and_children() override;

 private:
  Rect visible_rect_for_unit_in_slot(
      e_colony_building_slot slot, int unit_idx ) const;

  Rect sprite_rect_for_unit_in_slot( e_colony_building_slot slot,
                                     int unit_idx ) const;

  maybe<e_colony_building_slot> slot_for_coord(
      Coord where ) const;

 private:
  static Layout create_layout( IEngine& engine,
                               SSConst const& ss, gfx::size sz,
                               Colony const& colony );

  void draw_workers( rr::Renderer& renderer,
                     e_colony_building_slot const slot ) const;

  struct Dragging {
    UnitId id                   = {};
    e_colony_building_slot slot = {};
  };

  Colony& colony_;
  maybe<Dragging> dragging_ = {};
  Layout layout_;
};

} // namespace rn
