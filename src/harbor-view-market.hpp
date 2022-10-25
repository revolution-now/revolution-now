/****************************************************************
**harbor-view-market.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: Market commodities UI element within the harbor
*              view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "drag-drop.hpp"
#include "harbor-view-entities.hpp"
#include "market.rds.hpp"

// base
#include "base/vocab.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborMarketCommodities
*****************************************************************/
struct HarborMarketCommodities
  : public ui::View,
    public HarborSubView,
    public IDragSource<HarborDraggableObject_t>,
    public IDragSourceUserEdit<HarborDraggableObject_t>,
    public IDragSourceCheck<HarborDraggableObject_t>,
    public IDragSink<HarborDraggableObject_t>,
    public IDragSinkCheck<HarborDraggableObject_t> {
  static PositionedHarborSubView<HarborMarketCommodities> create(
      SS& ss, TS& ts, Player& player, Rect canvas );

  HarborMarketCommodities( SS& ss, TS& ts, Player& player,
                           bool stacked );

  // Implement ui::Object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View&       view() noexcept override;
  ui::View const& view() const noexcept override;

  maybe<DraggableObjectWithBounds<HarborDraggableObject_t>>
  object_here( Coord const& where ) const override;

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  // Implement ui::AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& ) override;

  // Implement IDragSource.
  bool try_drag( HarborDraggableObject_t const& a,
                 Coord const&                   where ) override;

  // Implement IDragSource.
  void cancel_drag() override;

  // Implement IDragSourceUserEdit.
  wait<maybe<HarborDraggableObject_t>> user_edit_object()
      const override;

  // Implement IDragSourceCheck.
  wait<base::valid_or<DragRejection>> source_check(
      HarborDraggableObject_t const& a, Coord const ) override;

  // Implement IDragSource.
  wait<> disown_dragged_object() override;

  // Override IDragSource.
  wait<> post_successful_source( HarborDraggableObject_t const&,
                                 Coord const& ) override;

  // Impelement IDragSink.
  maybe<HarborDraggableObject_t> can_receive(
      HarborDraggableObject_t const& a, int from_entity,
      Coord const& where ) const override;

  // Implement IDragSinkCheck.
  wait<base::valid_or<DragRejection>> sink_check(
      HarborDraggableObject_t const& a, int from_entity,
      Coord const ) override;

  // Impelement IDragSink.
  wait<> drop( HarborDraggableObject_t const& a,
               Coord const&                   where ) override;

  bool stacked() const { return stacked_; }

 private:
  // Returns true if the commodity is boycotted and the player
  // did not lift it, i.e. we are blocked.
  wait<base::NoDiscard<bool>> check_boycott( e_commodity type );

  static constexpr W single_layer_blocks_width  = 16;
  static constexpr W double_layer_blocks_width  = 8;
  static constexpr H single_layer_blocks_height = 1;
  static constexpr H double_layer_blocks_height = 2;

  // Commodities will be 24x24 + 8 pixels for text.
  static constexpr auto sprite_scale = Delta{ .w = 32, .h = 32 };
  static inline auto    sprite_delta =
      Delta{ .w = 1, .h = 1 } * sprite_scale;

  static constexpr W single_layer_width =
      single_layer_blocks_width * sprite_scale.w;
  static constexpr W double_layer_width =
      double_layer_blocks_width * sprite_scale.w;
  static constexpr H single_layer_height =
      single_layer_blocks_height * sprite_scale.h;
  static constexpr H double_layer_height =
      double_layer_blocks_height * sprite_scale.h;

  struct Draggable {
    Commodity   comm         = {};
    PriceChange price_change = {};
  };

  maybe<Draggable> dragging_;
  bool             stacked_ = false;
};

} // namespace rn
