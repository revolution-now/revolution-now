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
struct HarborStatusBar;

enum class e_tile;

/****************************************************************
** e_harbor_market_scale
*****************************************************************/
enum class e_harbor_market_scale {
  slim,
  medium,
  wide,
};

/****************************************************************
** HarborMarketCommodities
*****************************************************************/
struct HarborMarketCommodities
  : public ui::View,
    public HarborSubView,
    public IDragSource<HarborDraggableObject>,
    public IDragSourceUserEdit<HarborDraggableObject>,
    public IDragSourceCheck<HarborDraggableObject>,
    public IDragSink<HarborDraggableObject>,
    public IDragSinkCheck<HarborDraggableObject> {
  struct Layout {
    using CommRect = refl::enum_map<e_commodity, gfx::rect>;
    e_harbor_market_scale scale = {};
    // All relative to nw of view.
    gfx::rect left_sign;
    gfx::rect exit_sign;
    CommRect plates;
    e_tile left_sign_tile  = {};
    e_tile exit_sign_tile  = {};
    e_tile comm_plate_tile = {};
    e_tile slash_tile      = {};
    gfx::size slash_size;
    CommRect panel_inner_rect; // Area where comm is rendered.
    CommRect comm_icon;
    CommRect boycott_render_rect; // red x.
    CommRect bid_ask;
    int bid_ask_padding = {};
    gfx::rect exit_text;
  };

  static PositionedHarborSubView<HarborMarketCommodities> create(
      SS& ss, TS& ts, Player& player, Rect canvas,
      HarborStatusBar& harbor_status_bar );

  HarborMarketCommodities( SS& ss, TS& ts, Player& player,
                           HarborStatusBar& harbor_status_bar,
                           Layout const& layout );

  // Implement ui::Object.
  Delta delta() const override;

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override;

  ui::View& view() noexcept override;
  ui::View const& view() const noexcept override;

  maybe<DraggableObjectWithBounds<HarborDraggableObject>>
  object_here( Coord const& where ) const override;

  // Implement ui::Object.
  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

  // Implement ui::AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& ) override;

  wait<bool> perform_key(
      input::key_event_t const& event ) override;

  // Implement IDragSource.
  bool try_drag( HarborDraggableObject const& a,
                 Coord const& where ) override;

  // Implement IDragSource.
  void cancel_drag() override;

  // Implement IDragSourceUserEdit.
  wait<maybe<HarborDraggableObject>> user_edit_object()
      const override;

  // Implement IDragSourceCheck.
  wait<base::valid_or<DragRejection>> source_check(
      HarborDraggableObject const& a, Coord const ) override;

  // Implement IDragSource.
  wait<> disown_dragged_object() override;

  // Override IDragSource.
  wait<> post_successful_source( HarborDraggableObject const&,
                                 Coord const& ) override;

  // Impelement IDragSink.
  maybe<CanReceiveDraggable<HarborDraggableObject>> can_receive(
      HarborDraggableObject const& a, int from_entity,
      Coord const& where ) const override;

  // Implement IDragSinkCheck.
  wait<base::valid_or<DragRejection>> sink_check(
      HarborDraggableObject const& a, int from_entity,
      Coord const ) override;

  // Impelement IDragSink.
  wait<> drop( HarborDraggableObject const& a,
               Coord const& where ) override;

  auto scale() const { return layout_.scale; }

 public: // API
  wait<> unload_one();
  wait<> unload_all();

 private:
  // Returns true if the commodity is boycotted and the player
  // did not lift it, i.e. we are blocked.
  wait<base::NoDiscard<bool>> check_boycott( e_commodity type );

  // For managing the status bar.
  void send_invoice_msg_to_status_bar(
      Invoice const& invoice ) const;
  void send_purchase_info_to_status_bar(
      e_commodity comm ) const;
  void send_no_afford_msg_to_status_bar(
      Commodity const& comm ) const;
  void send_error_to_status_bar( std::string const& err ) const;
  void clear_status_bar_msg() const;

  wait<> unload_impl( UnitId unit_id, Commodity comm, int slot );

  wait<> sell( Commodity const& comm ) const;

  maybe<UnitId> get_active_unit() const;

  struct Draggable {
    Commodity comm  = {};
    Invoice invoice = {};
  };
  maybe<Draggable> dragging_;

  HarborStatusBar& harbor_status_bar_;
  Layout const layout_;
};

} // namespace rn
