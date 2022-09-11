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
#include "dragdrop.hpp"
#include "harbor-view-entities.hpp"

namespace rn {

struct SS;
struct TS;
struct Player;

/****************************************************************
** HarborMarketCommodities
*****************************************************************/
struct HarborMarketCommodities : public ui::View,
                                 public HarborSubView,
                                 public IDragSource,
                                 public IDragSourceUserInput,
                                 public IDragSourceCheck,
                                 public IDragSink,
                                 public IDragSinkCheck {
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

  maybe<DraggableObjectWithBounds> object_here(
      Coord const& where ) const override;

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord         coord ) const override;

  // Implement IDragSource.
  bool try_drag( std::any const& a,
                 Coord const&    where ) override;

  // Implement IDragSource.
  void cancel_drag() override;

  // Implement IDragSourceUserInput.
  wait<std::unique_ptr<std::any>> user_edit_object()
      const override;

  // Implement IDragSourceCheck.
  wait<base::valid_or<DragRejection>> source_check(
      std::any const& a, Coord const ) const override;

  // Implement IDragSource.
  void disown_dragged_object() override;

  // Override IDragSource.
  wait<> post_successful_source( std::any const&,
                                 Coord const& ) override;

  // Impelement IDragSink.
  maybe<std::any> can_receive(
      std::any const& a, int from_entity,
      Coord const& where ) const override;

  // Implement IDragSinkCheck.
  wait<base::valid_or<DragRejection>> sink_check(
      std::any const& a, int from_entity,
      Coord const ) const override;

  // Impelement IDragSink.
  void drop( std::any const& a, Coord const& where ) override;

  // Override IDragSink.
  wait<> post_successful_sink( std::any const&, int from_entity,
                               Coord const& ) override;

  bool stacked() const { return stacked_; }

 private:
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
    Commodity comm = {};
  };

  maybe<Draggable> dragging_;
  bool             stacked_ = false;
};

} // namespace rn
