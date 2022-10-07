/****************************************************************
**colview-entities.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-12.
*
* Description: The various UI sections/entities in Colony view.
*
*****************************************************************/
#include "colview-entities.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "colony-mgr.hpp"
#include "colony.hpp"
#include "colview-buildings.hpp"
#include "colview-land.hpp"
#include "colview-population.hpp"
#include "commodity.hpp"
#include "compositor.hpp"
#include "construction.hpp"
#include "gui.hpp"
#include "interrupts.hpp"
#include "land-production.hpp"
#include "logger.hpp"
#include "missionary.hpp"
#include "on-map.hpp"
#include "plow.hpp"
#include "production.hpp"
#include "render.hpp"
#include "road.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "ts.hpp"
#include "ustate.hpp"
#include "views.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit-composer.hpp"
#include "ss/units.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/unit-type.rds.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/maybe-util.hpp"

using namespace std;

namespace rn {

// Use this as the vtable key function.
void ColonySubView::update_this_and_children() {}

namespace {

/****************************************************************
** Constants
*****************************************************************/
constexpr W kCommodityTileWidth = kCommodityTileSize.w;

/****************************************************************
** Globals
*****************************************************************/
struct ColViewComposited {
  ColonyId                                        id;
  Delta                                           canvas_size;
  unique_ptr<ColonySubView>                       top_level;
  unordered_map<e_colview_entity, ColonySubView*> entities;
};

// FIXME
ColViewComposited g_composition;
ColonyProduction  g_production;

/****************************************************************
** Helpers
*****************************************************************/
Cargo_t to_cargo( ColViewObject_t const& o ) {
  switch( o.to_enum() ) {
    using namespace ColViewObject;
    case e::unit:
      return Cargo::unit{ o.get<ColViewObject::unit>().id };
    case e::commodity:
      return Cargo::commodity{ o.get<commodity>().comm };
  }
}

ColViewObject_t from_cargo( Cargo_t const& o ) {
  return overload_visit<ColViewObject_t>(
      o, //
      []( Cargo::unit u ) {
        return ColViewObject::unit{ .id = u.id };
      },
      []( Cargo::commodity const& c ) {
        return ColViewObject::commodity{ .comm = c.obj };
      } );
}

// Returns whether the action should be rejected.
wait<bool> check_abandon( Colony const& colony, IGui& gui ) {
  if( colony_population( colony ) > 1 ) co_return false;
  YesNoConfig const config{
      .msg = "Shall we abandon this colony, Your Excellency?",
      .yes_label      = "Yes, it is God's will.",
      .no_label       = "Never!  That would be folly.",
      .no_comes_first = true,
  };
  maybe<ui::e_confirm> res =
      co_await gui.optional_yes_no( config );
  co_return ( res != ui::e_confirm::yes );
}

maybe<string> check_seige() {
  // TODO: check if the colony is under seige; in that case
  // colonists are not allowed to move from the fields to the
  // gates.
  return nothing;
}

/****************************************************************
** Entities
*****************************************************************/
class TitleBar : public ui::View, public ColonySubView {
 public:
  static unique_ptr<TitleBar> create( SS& ss, TS& ts,
                                      Colony& colony,
                                      Delta   size ) {
    return make_unique<TitleBar>( ss, ts, colony, size );
  }

  TitleBar( SS& ss, TS& ts, Colony& colony, Delta size )
    : ColonySubView( ss, ts, colony ), size_( size ) {}

  Delta delta() const override { return size_; }

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::title_bar );
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  string title() const {
    auto const& colony = ss_.colonies.colony_for( colony_.id );
    return fmt::format( "{}, population {}", colony.name,
                        colony_population( colony ) );
  }

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override {
    rr::Painter painter = renderer.painter();
    painter.draw_solid_rect( rect( coord ), gfx::pixel::wood() );
    renderer
        .typer( centered( Delta::from_gfx(
                              rr::rendered_text_line_size_pixels(
                                  title() ) ),
                          rect( coord ) ),
                gfx::pixel::banana() )
        .write( title() );
  }

 private:
  Delta size_;
};

class MarketCommodities
  : public ui::View,
    public ColonySubView,
    public IDragSource<ColViewObject_t>,
    public IDragSourceUserInput<ColViewObject_t>,
    public IDragSink<ColViewObject_t> {
 public:
  static unique_ptr<MarketCommodities> create( SS& ss, TS& ts,
                                               Colony& colony,
                                               W block_width ) {
    return make_unique<MarketCommodities>( ss, ts, colony,
                                           block_width );
  }

  MarketCommodities( SS& ss, TS& ts, Colony& colony,
                     W block_width )
    : ColonySubView( ss, ts, colony ),
      block_width_( block_width ) {}

  Delta delta() const override {
    return Delta{
        block_width_ * SX{ refl::enum_count<e_commodity> },
        1 * 32 };
  }

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::commodities );
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  // Offset within a block that the commodity icon should be dis-
  // played.
  Delta rendered_commodity_offset() const {
    Delta res;
    res.h = 3;
    res.w = ( block_width_ - kCommodityTileWidth ) / 2;
    if( res.w < 0 ) res.w = 0;
    return res;
  }

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override {
    rr::Painter painter = renderer.painter();
    auto        comm_it = refl::enum_values<e_commodity>.begin();
    auto        label   = CommodityLabel::quantity{ 0 };
    Coord       pos     = coord;
    auto const& colony  = ss_.colonies.colony_for( colony_.id );
    for( int i = 0; i < kNumCommodityTypes; ++i ) {
      auto rect =
          Rect::from( pos, Delta{ .w = block_width_, .h = 32 } );
      painter.draw_empty_rect(
          rect, rr::Painter::e_border_mode::in_out,
          gfx::pixel::black() );
      label.value = colony.commodities[*comm_it];
      // When we drag a commodity we want the effect to be that
      // the commodity icon is still drawn (because it is a kind
      // of label for buckets), but we want the quantity to
      // render as zero to reflect the fact that the player has
      // removed those from the colony store.
      if( *comm_it == dragging_.member( &Commodity::type ) )
        label.value = 0;
      render_commodity_annotated(
          renderer,
          rect.upper_left() + rendered_commodity_offset(),
          *comm_it, label );
      pos.x += block_width_;
      comm_it++;
    }
  }

  int quantity_of( e_commodity type ) const {
    return colony_.commodities[type];
  }

  maybe<DraggableObjectWithBounds<ColViewObject_t>> object_here(
      Coord const& coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    auto sprite_scale =
        Delta{ .w = SX{ block_width_ }, .h = SY{ 32 } };
    auto box_upper_left =
        ( coord / sprite_scale ) * sprite_scale;
    auto idx = ( coord / sprite_scale - Coord{} ).w;
    UNWRAP_CHECK( type, commodity_from_index( idx ) );
    int quantity = quantity_of( type );
    // NOTE: we don't enforce that the quantity be greater than
    // zero here, instead we do that in try_drag. That way we can
    // still recognize what is under the cursor even if there is
    // zero quantity of it.
    return DraggableObjectWithBounds<ColViewObject_t>{
        .obj    = ColViewObject::commodity{ Commodity{
               .type = type, .quantity = quantity } },
        .bounds = Rect::from(
            box_upper_left + rendered_commodity_offset(),
            Delta{ .w = 1, .h = 1 } * kCommodityTileSize ) };
  }

  bool try_drag( ColViewObject_t const& o,
                 Coord const&           where ) override {
    UNWRAP_CHECK( [c], o.get_if<ColViewObject::commodity>() );
    if( c.quantity == 0 ) return false;
    // Sanity checks.
    UNWRAP_CHECK( here, object_here( where ) );
    UNWRAP_CHECK( comm_at_source,
                  here.obj.get_if<ColViewObject::commodity>() );
    Commodity dragged_c = comm_at_source.comm;
    CHECK( dragged_c.type == c.type );
    // Could be less if the destination has limited space and
    // has edited `o` to be less in quantity than the source.
    CHECK( c.quantity <= dragged_c.quantity );
    // End sanity checks.
    dragging_ = c;
    return true;
  }

  void cancel_drag() override { dragging_ = nothing; }

  wait<> disown_dragged_object() override {
    CHECK( dragging_ );
    e_commodity type = dragging_->type;
    int new_quantity = quantity_of( type ) - dragging_->quantity;
    CHECK( new_quantity >= 0 );
    colony_.commodities[type] = new_quantity;
    co_return;
  }

  maybe<ColViewObject_t> can_receive(
      ColViewObject_t const& o, int /*from_entity*/,
      Coord const&           where ) const override {
    CHECK( where.is_inside( rect( {} ) ) );
    if( o.holds<ColViewObject::commodity>() ) return o;
    return nothing;
  }

  wait<> drop( ColViewObject_t const& o,
               Coord const& /*where*/ ) override {
    UNWRAP_CHECK( [c], o.get_if<ColViewObject::commodity>() );
    int q = colony_.commodities[c.type];
    q += c.quantity;
    colony_.commodities[c.type] = q;
    co_return;
  }

  wait<maybe<ColViewObject_t>> user_edit_object()
      const override {
    CHECK( dragging_ );
    int    min  = 1;
    int    max  = dragging_->quantity;
    string text = fmt::format(
        "What quantity of @[H]{}@[] would you like to move? "
        "({}-{}):",
        lowercase_commodity_display_name( dragging_->type ), min,
        max );
    maybe<int> quantity = co_await ts_.gui.optional_int_input(
        { .msg           = text,
          .initial_value = max,
          .min           = min,
          .max           = max } );
    if( !quantity ) co_return nothing;
    Commodity new_comm = *dragging_;
    new_comm.quantity  = *quantity;
    CHECK( new_comm.quantity > 0 );
    co_return from_cargo( Cargo::commodity{ new_comm } );
  }

 private:
  W                block_width_;
  maybe<Commodity> dragging_;
};

class CargoView : public ui::View,
                  public ColonySubView,
                  public IDragSource<ColViewObject_t>,
                  public IDragSourceUserInput<ColViewObject_t>,
                  public IDragSink<ColViewObject_t>,
                  public IDragSinkCheck<ColViewObject_t> {
 public:
  static unique_ptr<CargoView> create( SS& ss, TS& ts,
                                       Colony& colony,
                                       Delta   size ) {
    return make_unique<CargoView>( ss, ts, colony, size );
  }

  CargoView( SS& ss, TS& ts, Colony& colony, Delta size )
    : ColonySubView( ss, ts, colony ), size_( size ) {}

  Delta delta() const override { return size_; }

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::cargo );
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  int max_slots_drawable() const {
    return delta().w / g_tile_delta.w;
  }

  // As usual, coordinate must be relative to upper left corner
  // of this view.
  maybe<pair<bool, int>> slot_idx_from_coord(
      Coord const& c ) const {
    if( !c.is_inside( rect( {} ) ) ) return nothing;
    if( c.y > 0 + g_tile_delta.h ) return nothing;
    int slot_idx = ( c / g_tile_delta ).distance_from_origin().w;
    bool is_open =
        holder_.has_value() &&
        slot_idx <
            ss_.units.unit_for( *holder_ ).desc().cargo_slots;
    return pair{ is_open, slot_idx };
  }

  // Returned rect is relative to upper left of this view.
  maybe<pair<bool, Rect>> slot_rect_from_idx( int slot ) const {
    if( slot < 0 ) return nothing;
    if( slot >= max_slots_drawable() ) return nothing;
    Coord slot_upper_left =
        Coord{} + Delta{ .w = g_tile_delta.w * slot };
    bool is_open =
        holder_.has_value() &&
        slot < ss_.units.unit_for( *holder_ ).desc().cargo_slots;
    return pair{ is_open,
                 Rect::from( slot_upper_left, g_tile_delta ) };
  }

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override {
    rr::Painter painter = renderer.painter();
    painter.draw_empty_rect( rect( coord ),
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel::black() );
    auto unit = holder_.fmap(
        [&]( UnitId id ) { return ss_.units.unit_for( id ); } );
    for( int idx{ 0 }; idx < max_slots_drawable(); ++idx ) {
      UNWRAP_CHECK( info, slot_rect_from_idx( idx ) );
      auto [is_open, relative_rect] = info;
      Rect rect = relative_rect.as_if_origin_were( coord );
      if( !is_open ) {
        painter.draw_solid_rect(
            rect.shifted_by( Delta{ .w = 1, .h = 0 } ),
            gfx::pixel::wood() );
        continue;
      }

      // FIXME: need to deduplicate this logic with that in
      // the Old World view.
      painter.draw_solid_rect(
          rect, gfx::pixel::wood().highlighted( 4 ) );
      painter.draw_empty_rect(
          rect, rr::Painter::e_border_mode::in_out,
          gfx::pixel::wood() );
      if( dragging_.has_value() && dragging_->slot == idx )
        // If we're draggin the thing in this slot then don't
        // draw it in there.
        continue;
      CargoHold const& hold = unit->cargo();
      switch( auto& v = hold[idx]; v.to_enum() ) {
        case CargoSlot::e::empty: break;
        case CargoSlot::e::overflow: break;
        case CargoSlot::e::cargo: {
          auto& cargo = v.get<CargoSlot::cargo>();
          overload_visit(
              cargo.contents,
              [&]( Cargo::unit u ) {
                render_unit(
                    renderer, rect.upper_left(),
                    ss_.units.unit_for( u.id ),
                    UnitRenderOptions{ .flag = false } );
              },
              [&]( Cargo::commodity const& c ) {
                render_commodity_annotated(
                    renderer,
                    rect.upper_left() +
                        kCommodityInCargoHoldRenderingOffset,
                    c.obj );
              } );
          break;
        }
      }
    }
  }

  void set_unit( maybe<UnitId> unit ) { holder_ = unit; }

  maybe<ColViewObject_t> can_receive(
      ColViewObject_t const& o, int from_entity,
      Coord const& where ) const override {
    CHECK( where.is_inside( rect( {} ) ) );
    if( !holder_ ) return nothing;
    maybe<pair<bool, int>> slot_info =
        slot_idx_from_coord( where );
    if( !slot_info.has_value() ) return nothing;
    auto [is_open, slot_idx] = *slot_info;
    if( !is_open ) return nothing;
    CONVERT_ENTITY( from_enum, from_entity );
    if( from_enum == e_colview_entity::cargo ) {
      // At this point the player is dragging something from one
      // slot to another in the same cargo, which is guaranteed
      // to always be allowed, since when the drag operation will
      // first remove the cargo from the source slot, then when
      // it is dropped, the drop will succeed so long as there is
      // enough space anywhere in the cargo for that cargo, which
      // there always will be, because the cargo originated from
      // within this same cargo.
      return o;
    }
    // We are dragging from another source, so we must check to
    // see if we have room for what is being dragged.
    auto& unit = ss_.units.unit_for( *holder_ );
    switch( o.to_enum() ) {
      using namespace ColViewObject;
      case e::unit: {
        UnitId id = o.get<ColViewObject::unit>().id;
        // Note that we allow wagon trains to recieve units at
        // this stage as long as they theoretically fit. In the
        // next stage we will reject that and present a message
        // to the user.
        if( !unit.cargo().fits_somewhere( ss_.units,
                                          Cargo::unit{ id } ) )
          return nothing;
        return o;
      }
      case e::commodity:
        Commodity c = o.get<commodity>().comm;
        int       max_quantity =
            unit.cargo().max_commodity_quantity_that_fits(
                c.type );
        c.quantity = clamp( c.quantity, 0, max_quantity );
        if( c.quantity == 0 ) return nothing;
        return commodity{ .comm = c };
    }
  }

  wait<base::valid_or<DragRejection>> sink_check(
      ColViewObject_t const& o, int from_entity,
      Coord const ) const override {
    CHECK( holder_.has_value() );
    if( ss_.units.unit_for( *holder_ ).type() ==
            e_unit_type::wagon_train &&
        o.holds<ColViewObject::unit>() )
      co_return DragRejection{
          .reason =
              "Only ships can hold other units as cargo." };
    CONVERT_ENTITY( from_enum, from_entity );
    switch( from_enum ) {
      case e_colview_entity::units_at_gate:
      case e_colview_entity::cargo:
      case e_colview_entity::commodities: //
        co_return base::valid;
      case e_colview_entity::land:
      case e_colview_entity::buildings: //
        if( co_await check_abandon( colony_, ts_.gui ) )
          // If we're rejecting then that means that the player
          // has opted not to abandon the colony, so there is no
          // need to display a reason message.
          co_return DragRejection{ .reason = nothing };
        if( auto msg = check_seige(); msg.has_value() )
          co_return DragRejection{ .reason = *msg };
        co_return base::valid;
      case e_colview_entity::population:
      case e_colview_entity::title_bar:
      case e_colview_entity::production:
        FATAL( "unexpected source entity." );
    }
  }

  wait<> drop( ColViewObject_t const& o,
               Coord const&           where ) override {
    CHECK( holder_ );
    auto&   cargo_hold = ss_.units.unit_for( *holder_ ).cargo();
    Cargo_t cargo      = to_cargo( o );
    CHECK( cargo_hold.fits_somewhere( ss_.units, cargo ) );
    UNWRAP_CHECK( slot_info, slot_idx_from_coord( where ) );
    auto [is_open, slot_idx] = slot_info;
    overload_visit(
        cargo, //
        [this, slot_idx = slot_idx]( Cargo::unit u ) {
          ss_.units.change_to_cargo_somewhere(
              *holder_, u.id, /*starting_slot=*/slot_idx );
          // Check if we've abandoned the colony, which could
          // happen if we dragged the last unit working in the
          // colony into the cargo hold.
          if( colony_population( colony_ ) == 0 )
            throw colony_abandon_interrupt{};
        },
        [this,
         slot_idx = slot_idx]( Cargo::commodity const& c ) {
          add_commodity_to_cargo( ss_.units, c.obj, *holder_,
                                  slot_idx,
                                  /*try_other_slots=*/true );
        } );
    co_return;
  }

  // Returns the rect that bounds the sprite corresponding to the
  // cargo item covered by the given slot.
  maybe<pair<Cargo_t, Rect>> cargo_item_with_rect(
      int slot ) const {
    maybe<pair<bool, Rect>> slot_rect =
        slot_rect_from_idx( slot );
    if( !slot_rect.has_value() ) return nothing;
    auto [is_open, rect] = *slot_rect;
    if( !is_open ) return nothing;
    maybe<pair<Cargo_t const&, int>> maybe_cargo =
        ss_.units.unit_for( *holder_ )
            .cargo()
            .cargo_covering_slot( slot );
    if( !maybe_cargo ) return nothing;
    auto const& [cargo, same_slot] = *maybe_cargo;
    CHECK( slot == same_slot );
    return pair{
        cargo,
        overload_visit<Rect>(
            cargo, //
            [rect = rect]( Cargo::unit ) { return rect; },
            [rect = rect]( Cargo::commodity const& ) {
              return Rect::from(
                  rect.upper_left() +
                      kCommodityInCargoHoldRenderingOffset,
                  kCommodityTileSize );
            } ) };
  }

  maybe<DraggableObjectWithBounds<ColViewObject_t>> object_here(
      Coord const& where ) const override {
    if( !holder_ ) return nothing;
    maybe<pair<bool, int>> slot_info =
        slot_idx_from_coord( where );
    if( !slot_info ) return nothing;
    auto [is_open, slot_idx] = *slot_info;
    if( !is_open ) return nothing;
    maybe<pair<Cargo_t, Rect>> cargo_with_rect =
        cargo_item_with_rect( slot_idx );
    if( !cargo_with_rect ) return nothing;
    return DraggableObjectWithBounds<ColViewObject_t>{
        .obj    = from_cargo( cargo_with_rect->first ),
        .bounds = cargo_with_rect->second };
  }

  // For this one it happens that we need the coordinate instead
  // of the object, since if the object is a commodity we may not
  // be able to find a unique cargo slot that holds that com-
  // modity if there are more than one.
  bool try_drag( ColViewObject_t const& o,
                 Coord const&           where ) override {
    if( !holder_ ) return false;
    maybe<pair<bool, int>> slot_info =
        slot_idx_from_coord( where );
    if( !slot_info ) return false;
    auto [is_open, slot_idx] = *slot_info;
    if( !is_open ) return false;
    dragging_ = Draggable{ .slot = slot_idx, .object = o };
    return true;
  }

  void cancel_drag() override { dragging_ = nothing; }

  wait<> disown_dragged_object() override {
    CHECK( holder_ );
    CHECK( dragging_ );
    // We need to take the stored object instead of just re-
    // trieving it from the slot, because the stored object might
    // have been edited, e.g. the commodity quantity might have
    // been lowered.
    Cargo_t cargo_to_remove = to_cargo( dragging_->object );
    overload_visit(
        cargo_to_remove,
        [this]( Cargo::unit held ) {
          ss_.units.disown_unit( held.id );
        },
        [this]( Cargo::commodity const& to_remove ) {
          UNWRAP_CHECK(
              existing_cargo,
              ss_.units.unit_for( *holder_ )
                  .cargo()
                  .cargo_starting_at_slot( dragging_->slot ) );
          UNWRAP_CHECK(
              existing_comm,
              existing_cargo.get_if<Cargo::commodity>() );
          Commodity reduced_comm = existing_comm.obj;
          CHECK( reduced_comm.type == existing_comm.obj.type );
          CHECK( reduced_comm.type == to_remove.obj.type );
          reduced_comm.quantity -= to_remove.obj.quantity;
          CHECK( reduced_comm.quantity >= 0 );
          rm_commodity_from_cargo( ss_.units, *holder_,
                                   dragging_->slot );
          if( reduced_comm.quantity > 0 )
            add_commodity_to_cargo( ss_.units, reduced_comm,
                                    *holder_, dragging_->slot,
                                    /*try_other_slots=*/false );
        } );
    co_return;
  }

  wait<maybe<ColViewObject_t>> user_edit_object()
      const override {
    CHECK( dragging_ );
    UNWRAP_CHECK( cargo_and_rect,
                  cargo_item_with_rect( dragging_->slot ) );
    Cargo_t const& cargo = cargo_and_rect.first;
    if( !cargo.holds<Cargo::commodity>() )
      co_return from_cargo( cargo );
    // We have a commodity.
    Cargo::commodity const& comm = cargo.get<Cargo::commodity>();
    int                     min  = 1;
    int                     max  = comm.obj.quantity;
    string                  text = fmt::format(
        "What quantity of @[H]{}@[] would you like to move? "
                         "({}-{}):",
        lowercase_commodity_display_name( comm.obj.type ), min,
        max );
    maybe<int> quantity = co_await ts_.gui.optional_int_input(
        { .msg           = text,
          .initial_value = max,
          .min           = min,
          .max           = max } );
    if( !quantity ) co_return nothing;
    Commodity new_comm = comm.obj;
    new_comm.quantity  = *quantity;
    CHECK( new_comm.quantity > 0 );
    co_return ColViewObject::commodity{ new_comm };
  }

 private:
  struct Draggable {
    int             slot;
    ColViewObject_t object;
  };

  // FIXME: this gets reset whenever we recomposite. We need to
  // either put this in a global place, or not recreate all of
  // these view objects each time we recomposite (i.e., reuse
  // them).
  maybe<UnitId>    holder_;
  Delta            size_;
  maybe<Draggable> dragging_;
};

class UnitsAtGateColonyView
  : public ui::View,
    public ColonySubView,
    public IDragSource<ColViewObject_t>,
    public IDragSink<ColViewObject_t>,
    public IDragSinkCheck<ColViewObject_t> {
 public:
  static unique_ptr<UnitsAtGateColonyView> create(
      SS& ss, TS& ts, Colony& colony, CargoView* cargo_view,
      Delta size ) {
    return make_unique<UnitsAtGateColonyView>(
        ss, ts, colony, cargo_view, size );
  }

  UnitsAtGateColonyView( SS& ss, TS& ts, Colony& colony,
                         CargoView* cargo_view, Delta size )
    : ColonySubView( ss, ts, colony ),
      cargo_view_( cargo_view ),
      size_( size ) {
    update_this_and_children();
  }

  Delta delta() const override { return size_; }

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::units_at_gate );
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override {
    rr::Painter painter = renderer.painter();
    painter.draw_empty_rect( rect( coord ).with_inc_size(),
                             rr::Painter::e_border_mode::inside,
                             gfx::pixel::black() );
    for( auto [unit_id, unit_pos] : positioned_units_ ) {
      if( dragging_ == unit_id ) continue;
      Coord draw_pos = unit_pos.as_if_origin_were( coord );
      render_unit(
          renderer, draw_pos, ss_.units.unit_for( unit_id ),
          UnitRenderOptions{
              .flag   = true,
              .shadow = UnitShadow{
                  .color = config_colony.colors
                               .unit_shadow_color_light } } );
      if( selected_ == unit_id )
        painter.draw_empty_rect(
            Rect::from( draw_pos, g_tile_delta ) -
                Delta{ .w = 1, .h = 1 },
            rr::Painter::e_border_mode::in_out,
            gfx::pixel::green() );
    }
  }

  // Implement AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& event ) override {
    if( event.buttons != input::e_mouse_button_event::left_up )
      co_return;
    CHECK( event.pos.is_inside( rect( {} ) ) );
    for( auto [unit_id, unit_pos] : positioned_units_ ) {
      if( event.pos.is_inside(
              Rect::from( unit_pos, g_tile_delta ) ) ) {
        co_await click_on_unit( unit_id );
      }
    }
  }

  maybe<UnitId> contains_unit( Coord const& where ) const {
    for( PositionedUnit const& pu : positioned_units_ )
      if( where.is_inside( Rect::from( pu.pos, g_tile_delta ) ) )
        return pu.id;
    return nothing;
  }

  maybe<DraggableObjectWithBounds<ColViewObject_t>> object_here(
      Coord const& where ) const override {
    for( PositionedUnit const& pu : positioned_units_ ) {
      auto rect = Rect::from( pu.pos, g_tile_delta );
      if( where.is_inside( rect ) )
        return DraggableObjectWithBounds<ColViewObject_t>{
            .obj    = ColViewObject::unit{ .id = pu.id },
            .bounds = rect };
    }
    return nothing;
  }

  maybe<ColViewObject_t> can_receive_unit(
      UnitId       dragged, e_colview_entity /*from*/,
      Coord const& where ) const {
    auto& unit = ss_.units.unit_for( dragged );
    // Player should not be dragging ships or wagons.
    CHECK( unit.desc().cargo_slots == 0 );
    // See if the draga target is over top of a unit.
    maybe<UnitId> over_unit_id = contains_unit( where );
    if( !over_unit_id ) {
      // The player is moving a unit outside of the colony, let's
      // check if the unit is already outside the colony, in
      // which case there is no reason to drag the unit here.
      if( is_unit_on_map( ss_.units, dragged ) ) return nothing;
      // The player is moving the unit outside the colony, which
      // is always allowed, at least for now. If the unit is in
      // the colony (as opposed to cargo) and there is a stockade
      // then we won't allow the population to be reduced below
      // three, but that will be checked in the confirmation
      // stage.
      //
      // FIXME: need to ask the player what this colonist should
      // be after moving it out of the colony, otherwise if a
      // colony has 50 muskets and the last colonist is being re-
      // moved (to abandon the colony) then the player has no way
      // to save those muskets by making the unit into a soldier.
      return ColViewObject::unit{ .id = dragged };
    }
    Unit const& target_unit =
        ss_.units.unit_for( *over_unit_id );
    if( target_unit.desc().cargo_slots == 0 ) return nothing;
    // Check if the target_unit is already holding the dragged
    // unit.
    maybe<UnitId> maybe_holder_of_dragged =
        is_unit_onboard( ss_.units, dragged );
    if( maybe_holder_of_dragged &&
        *maybe_holder_of_dragged == over_unit_id )
      // The dragged unit is already in the cargo of the target
      // unit.
      return nothing;
    // At this point, the unit is being dragged on top of another
    // unit that has cargo slots but is not already being held by
    // that unit, so we need to check if the unit fits.
    if( !target_unit.cargo().fits_somewhere(
            ss_.units, Cargo::unit{ dragged } ) )
      return nothing;
    return ColViewObject::unit{ .id = dragged };
  }

  wait<base::valid_or<DragRejection>> sink_check(
      ColViewObject_t const&, int from_entity,
      Coord const ) const override {
    CONVERT_ENTITY( from_enum, from_entity );
    switch( from_enum ) {
      case e_colview_entity::units_at_gate:
      case e_colview_entity::commodities:
      case e_colview_entity::cargo: //
        co_return base::valid;
      case e_colview_entity::land:
      case e_colview_entity::buildings: //
        if( co_await check_abandon( colony_, ts_.gui ) )
          // If we're rejecting then that means that the player
          // has opted not to abandon the colony, so there is no
          // need to display a reason message.
          co_return DragRejection{ .reason = nothing };
        if( auto msg = check_seige(); msg.has_value() )
          co_return DragRejection{ .reason = *msg };
        co_return base::valid;
      case e_colview_entity::population:
      case e_colview_entity::title_bar:
      case e_colview_entity::production:
        FATAL( "unexpected source entity." );
    }
  }

  maybe<ColViewObject_t> can_cargo_unit_receive_commodity(
      Commodity const& comm, e_colview_entity from,
      UnitId cargo_unit_id ) const {
    Unit const& target_unit =
        ss_.units.unit_for( cargo_unit_id );
    CHECK( target_unit.desc().cargo_slots != 0 );
    // Check if the target_unit is already holding the dragged
    // commodity.
    if( from == e_colview_entity::cargo ) {
      CHECK( selected_.has_value() );
      CHECK(
          ss_.units.unit_for( *selected_ ).desc().cargo_slots >
          0 );
      if( cargo_unit_id == *selected_ )
        // The commodity is already in the cargo of the unit
        // under the mouse.
        return nothing;
    }
    // At this point, the commodity is being dragged on top of a
    // unit that has cargo slots but is not already being held by
    // that unit, so we need to check if the commodity fits.
    int max_q =
        target_unit.cargo().max_commodity_quantity_that_fits(
            comm.type );
    if( max_q == 0 ) return nothing;
    // We may need to adjust the quantity.
    Commodity new_comm = comm;
    new_comm.quantity  = std::min( new_comm.quantity, max_q );
    CHECK( new_comm.quantity > 0 );
    return ColViewObject::commodity{ .comm = new_comm };
  }

  static maybe<UnitTransformationFromCommodityResult>
  transformed_unit_composition_from_commodity(
      Unit const& unit, Commodity const& comm ) {
    vector<UnitTransformationFromCommodityResult> possibilities =
        unit.with_commodity_added( comm );
    adjust_for_independence_status(
        possibilities,
        // FIXME
        /*independence_declared=*/false );

    erase_if( possibilities, []( auto const& xform_res ) {
      for( auto [mod, _] : xform_res.modifier_deltas )
        if( !config_unit_type.composition.modifier_traits[mod]
                 .player_can_grant )
          return true;
      return false; // don't erase.
    } );

    maybe<UnitTransformationFromCommodityResult> res;
    if( possibilities.size() == 1 ) res = possibilities[0];
    return res;
  }

  maybe<ColViewObject_t> can_unit_receive_commodity(
      Commodity const& comm, e_colview_entity /*from*/,
      UnitId           id ) const {
    // We are dragging a commodity over a unit that does not have
    // a cargo hold. This could be valid if we are e.g. giving
    // muskets to a colonist.
    UNWRAP_RETURN( xform_res,
                   transformed_unit_composition_from_commodity(
                       ss_.units.unit_for( id ), comm ) );
    return ColViewObject::commodity{
        .comm = with_quantity( comm, xform_res.quantity_used ) };
  }

  maybe<ColViewObject_t> can_receive_commodity(
      Commodity const& comm, e_colview_entity from,
      Coord const& where ) const {
    maybe<UnitId> over_unit_id = contains_unit( where );
    if( !over_unit_id ) return nothing;
    Unit const& target_unit =
        ss_.units.unit_for( *over_unit_id );
    if( target_unit.desc().cargo_slots != 0 )
      return can_cargo_unit_receive_commodity( comm, from,
                                               *over_unit_id );
    else
      return can_unit_receive_commodity( comm, from,
                                         *over_unit_id );
  }

  maybe<ColViewObject_t> can_receive(
      ColViewObject_t const& o, int from_entity,
      Coord const& where ) const override {
    CONVERT_ENTITY( from_enum, from_entity );
    CHECK( where.is_inside( rect( {} ) ) );
    if( !where.is_inside( rect( {} ) ) ) return nothing;
    return overload_visit(
        o, //
        [&]( ColViewObject::unit const& unit ) {
          return can_receive_unit( unit.id, from_enum, where );
        },
        [&]( ColViewObject::commodity const& comm ) {
          return can_receive_commodity( comm.comm, from_enum,
                                        where );
        } );
  }

  wait<> drop( ColViewObject_t const& o,
               Coord const&           where ) override {
    maybe<UnitId> target_unit = contains_unit( where );
    overload_visit(
        o, //
        [&]( ColViewObject::unit const& unit ) {
          if( target_unit ) {
            ss_.units.change_to_cargo_somewhere(
                /*new_holder=*/*target_unit,
                /*held=*/unit.id );
          } else {
            unit_to_map_square_non_interactive(
                ss_, ts_, unit.id, colony_.location );
            // This is not strictly necessary, but as a conve-
            // nience to the user, clear the orders, otherwise it
            // would be sentry'd, which is probably not what the
            // player wants.
            ss_.units.unit_for( unit.id ).clear_orders();
            // Check if we've abandoned the colony.
            if( colony_population( colony_ ) == 0 )
              throw colony_abandon_interrupt{};
          }
        },
        [&]( ColViewObject::commodity const& comm ) {
          CHECK( target_unit );
          Unit& unit = ss_.units.unit_for( *target_unit );
          if( unit.desc().cargo_slots > 0 ) {
            add_commodity_to_cargo( ss_.units, comm.comm,
                                    *target_unit,
                                    /*slot=*/0,
                                    /*try_other_slots=*/true );
          } else {
            // We are dragging a commodity over a unit that does
            // not have a cargo hold. This could be valid if we
            // are e.g. giving muskets to a colonist.
            Commodity const& dropping_comm = comm.comm;
            UNWRAP_CHECK(
                xform_res,
                transformed_unit_composition_from_commodity(
                    unit, dropping_comm ) );
            CHECK( xform_res.quantity_used ==
                   dropping_comm.quantity );
            unit.change_type( xform_res.new_comp );
            // The unit, being at the colony gate, is actually on
            // the map at the site of this colony. In the event
            // that we are e.g. changing a colonist to a scout
            // (whsch has a sighting radius of two) we should
            // call this function to update the rendered map
            // along with anything else that needs to be done.
            unit_to_map_square_non_interactive(
                ss_, ts_, unit.id(), colony_.location );
          }
        } );
    co_return;
  }

  bool try_drag( ColViewObject_t const& o,
                 Coord const& /*where*/ ) override {
    UNWRAP_CHECK( [id], o.get_if<ColViewObject::unit>() );
    bool is_cargo_unit =
        ss_.units.unit_for( id ).desc().cargo_slots > 0;
    if( is_cargo_unit ) return false;
    dragging_ = id;
    return true;
  }

  void cancel_drag() override { dragging_ = nothing; }

  wait<> disown_dragged_object() override {
    UNWRAP_CHECK( unit_id, dragging_ );
    ss_.units.disown_unit( unit_id );
    co_return;
  }

 private:
  void set_selected_unit( maybe<UnitId> id ) {
    selected_ = id;
    cargo_view_->set_unit( id );
  }

  wait<> click_on_unit( UnitId id ) {
    lg.info( "clicked on unit {}.",
             debug_string( ss_.units, id ) );
    Unit& unit = ss_.units.unit_for( id );
    if( selected_ != id ) {
      set_selected_unit( id );
      // The first time we select a unit, just select it, but
      // don't pop up the orders menu until the second click.
      // This should make a more polished feel for the UI, and
      // also allow viewing a ship's cargo without popping up the
      // orders menu.
      co_return;
    }
    // FIXME: need to replace the two below calls with a more
    // robust (non-string-based) approach.
    ChoiceConfig config{
        .msg     = "What would you like to do?",
        .options = {
            { .key = "orders", .display_name = "Change Orders" },
            { .key = "strip", .display_name = "Strip Unit" } } };
    if( can_bless_missionaries( colony_ ) &&
        unit_can_be_blessed( unit.type_obj() ) )
      config.options.push_back(
          { .key          = "missionary",
            .display_name = "Bless as Missionary" } );
    maybe<string> const mode =
        co_await ts_.gui.optional_choice( config );
    if( mode == "orders" ) {
      ChoiceConfig config{
          .msg     = "Change unit orders to:",
          .options = {
              { .key = "clear", .display_name = "Clear Orders" },
              { .key = "sentry", .display_name = "Sentry" },
              { .key          = "fortify",
                .display_name = "Fortify" } } };
      maybe<string> const new_orders =
          co_await ts_.gui.optional_choice( config );
      if( new_orders == "clear" )
        unit.clear_orders();
      else if( new_orders == "sentry" )
        unit.sentry();
      else if( new_orders == "fortify" ) {
        if( unit.orders() != e_unit_orders::fortified )
          // This will place them in the "fortifying" state and
          // will consume all movement points. The only time we
          // don't want to do this is if the unit's state is
          // "fortified", since then this would represent a pes-
          // simisation.
          unit.start_fortify();
      }
    } else if( mode == "strip" ) {
      // Clear orders just in case it is a pioneer building a
      // road or plowing; that would put the pioneer into an in-
      // consistent state.
      if( unit.orders() == e_unit_orders::road ||
          unit.orders() == e_unit_orders::plow )
        unit.clear_orders();
      strip_unit_to_base_type( unit, colony_ );
    } else if( mode == "missionary" ) {
      // TODO: play blessing tune.
      bless_as_missionary( colony_, unit );
    }
  }

  void update_this_and_children() override {
    auto const& colony = ss_.colonies.colony_for( colony_.id );
    auto const& units  = ss_.units.from_coord( colony.location );
    auto        unit_pos = Coord{} + Delta{ .w = 1, .h = 16 };
    positioned_units_.clear();
    maybe<UnitId> first_with_cargo;
    for( auto unit_id : units ) {
      positioned_units_.push_back(
          { .id = unit_id, .pos = unit_pos } );
      unit_pos.x += 32;
      if( !first_with_cargo.has_value() &&
          ss_.units.unit_for( unit_id ).desc().cargo_slots > 0 )
        first_with_cargo = unit_id;
    }
    if( selected_.has_value() && !units.contains( *selected_ ) )
      set_selected_unit( nothing );
    if( !selected_.has_value() )
      set_selected_unit( first_with_cargo );
  }

  struct PositionedUnit {
    UnitId id;
    Coord  pos; // relative to upper left of this CargoView.
  };

  vector<PositionedUnit> positioned_units_;
  // FIXME: this gets reset whenever we recomposite. We need to
  // either put this in a global place, or not recreate all of
  // these view objects each time we recomposite (i.e., reuse
  // them).
  maybe<UnitId> selected_;

  CargoView*    cargo_view_;
  Delta         size_;
  maybe<UnitId> dragging_;
};

class ProductionView : public ui::View, public ColonySubView {
 public:
  static unique_ptr<ProductionView> create( SS& ss, TS& ts,
                                            Colony& colony,
                                            Delta   size ) {
    return make_unique<ProductionView>( ss, ts, colony, size );
  }

  ProductionView( SS& ss, TS& ts, Colony& colony, Delta size )
    : ColonySubView( ss, ts, colony ), size_( size ) {}

  Delta delta() const override { return size_; }

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_colview_entity::production );
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override {
    rr::Painter painter = renderer.painter();
    painter.draw_empty_rect( rect( coord ).with_inc_size(),
                             rr::Painter::e_border_mode::inside,
                             gfx::pixel::black() );
    SCOPED_RENDERER_MOD_ADD(
        painter_mods.repos.translation,
        gfx::to_double(
            gfx::size( coord.distance_from_origin() ) ) );
    rr::Typer typer = renderer.typer( Coord{ .x = 2, .y = 2 },
                                      gfx::pixel::black() );
    typer.write( "Hammers:      {}\n", colony_.hammers );
    typer.write( "Construction: " );
    if( colony_.construction.has_value() ) {
      typer.write( "{}\n",
                   construction_name( *colony_.construction ) );
      typer.write( "right-click to buy.\n" );
    } else {
      typer.write( "nothing\n" );
    }
  }

  // Implement AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& event ) override {
    CHECK( event.pos.is_inside( rect( {} ) ) );
    if( event.buttons ==
        input::e_mouse_button_event::right_up ) {
      maybe<RushConstruction> const invoice =
          rush_construction_cost( ss_, colony_ );
      if( !invoice.has_value() )
        // This can happen if either the colony is not building
        // anything or if it is building something that it al-
        // ready has.
        co_return;
      UNWRAP_CHECK( player,
                    ss_.players.players[colony_.nation] );
      co_await rush_construction_prompt( player, colony_,
                                         ts_.gui, *invoice );
      co_return;
    }
    co_await select_colony_construction( ss_, colony_, ts_.gui );
  }

 private:
  Delta size_;
};

/****************************************************************
** Compositing
*****************************************************************/
struct CompositeColSubView : public ui::InvisibleView,
                             public ColonySubView {
  CompositeColSubView(
      SS& ss, TS& ts, Colony& colony, Delta size,
      std::vector<ui::OwningPositionedView> views,
      Player const&                         player )
    : ui::InvisibleView( size, std::move( views ) ),
      ColonySubView( ss, ts, colony ),
      player_( player ) {
    for( ui::PositionedView v : *this ) {
      auto* col_view = dynamic_cast<ColonySubView*>( v.view );
      CHECK( col_view );
      ptrs_.push_back( col_view );
    }
    CHECK( int( ptrs_.size() ) == count() );
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  // Implement AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& event ) override {
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedView pos_view = at( i );
      if( !event.pos.is_inside( pos_view.rect() ) ) continue;
      input::event_t const shifted_event =
          input::move_mouse_origin_by(
              event, pos_view.coord.distance_from_origin() );
      UNWRAP_CHECK(
          shifted_mouse_button_event,
          shifted_event.get_if<input::mouse_button_event_t>() );
      // Need to co_await so that shifted_event stays alive.
      co_await ptrs_[i]->perform_click(
          shifted_mouse_button_event );
      break;
    }
  }

  maybe<PositionedDraggableSubView<ColViewObject_t>> view_here(
      Coord coord ) override {
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedView pos_view = at( i );
      if( !coord.is_inside( pos_view.rect() ) ) continue;
      maybe<PositionedDraggableSubView<ColViewObject_t>> p_view =
          ptrs_[i]->view_here(
              coord.with_new_origin( pos_view.coord ) );
      if( !p_view ) continue;
      p_view->upper_left =
          p_view->upper_left.as_if_origin_were( pos_view.coord );
      return p_view;
    }
    if( coord.is_inside( rect( {} ) ) )
      return PositionedDraggableSubView<ColViewObject_t>{
          this, Coord{} };
    return nothing;
  }

  // Implement ColonySubView.
  maybe<DraggableObjectWithBounds<ColViewObject_t>> object_here(
      Coord const& coord ) const override {
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedViewConst pos_view = at( i );
      if( !coord.is_inside( pos_view.rect() ) ) continue;
      maybe<DraggableObjectWithBounds<ColViewObject_t>> obj =
          ptrs_[i]->object_here(
              coord.with_new_origin( pos_view.coord ) );
      if( !obj ) continue;
      obj->bounds =
          obj->bounds.as_if_origin_were( pos_view.coord );
      return obj;
    }
    // This view itself has no objects.
    return nothing;
  }

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override { return nothing; }

  void update_this_and_children() override {
    for( ColonySubView* p : ptrs_ )
      p->update_this_and_children();
  }

  vector<ColonySubView*> ptrs_;
  Player const&          player_;
};

void recomposite( SS& ss, TS& ts, Colony& colony,
                  Delta const& canvas_size ) {
  UNWRAP_CHECK( player, ss.players.players[colony.nation] );
  lg.trace( "recompositing colony view." );
  g_composition.id          = colony.id;
  g_composition.canvas_size = canvas_size;

  g_composition.top_level = nullptr;
  g_composition.entities.clear();
  vector<ui::OwningPositionedView> views;

  Coord pos;
  Delta available;

  // [Title Bar] ------------------------------------------------
  auto title_bar = TitleBar::create(
      ss, ts, colony, Delta{ .w = canvas_size.w, .h = 10 } );
  g_composition.entities[e_colview_entity::title_bar] =
      title_bar.get();
  pos = Coord{};
  Y const title_bar_bottom =
      title_bar->rect( pos ).bottom_edge();
  views.push_back( ui::OwningPositionedView{
      .view = std::move( title_bar ), .coord = pos } );

  // [MarketCommodities] ----------------------------------------
  W comm_block_width =
      canvas_size.w / SX{ refl::enum_count<e_commodity> };
  comm_block_width =
      std::clamp( comm_block_width, kCommodityTileSize.w, 32 );
  auto market_commodities = MarketCommodities::create(
      ss, ts, colony, comm_block_width );
  g_composition.entities[e_colview_entity::commodities] =
      market_commodities.get();
  pos = centered_bottom( market_commodities->delta(),
                         Rect::from( Coord{}, canvas_size ) );
  auto const market_commodities_top = pos.y;
  views.push_back( ui::OwningPositionedView{
      .view = std::move( market_commodities ), .coord = pos } );

  // [Middle Strip] ---------------------------------------------
  Delta   middle_strip_size{ canvas_size.w, 32 + 32 + 16 };
  Y const middle_strip_top =
      market_commodities_top - middle_strip_size.h;

  // [Population] -----------------------------------------------
  auto population_view = PopulationView::create(
      ss, ts, colony, player,
      middle_strip_size.with_width( middle_strip_size.w / 3 ) );
  g_composition.entities[e_colview_entity::population] =
      population_view.get();
  pos = Coord{ .x = 0, .y = middle_strip_top };
  X const population_right_edge =
      population_view->rect( pos ).right_edge();
  views.push_back( ui::OwningPositionedView{
      .view = std::move( population_view ), .coord = pos } );

  // [Cargo] ----------------------------------------------------
  auto cargo_view = CargoView::create(
      ss, ts, colony,
      middle_strip_size.with_width( middle_strip_size.w / 3 )
          .with_height( 32 ) );
  g_composition.entities[e_colview_entity::cargo] =
      cargo_view.get();
  pos = Coord{ .x = population_right_edge,
               .y = middle_strip_top + 32 + 16 };
  X const cargo_right_edge =
      cargo_view->rect( pos ).right_edge();
  auto* p_cargo_view = cargo_view.get();
  views.push_back( ui::OwningPositionedView{
      .view = std::move( cargo_view ), .coord = pos } );

  // [Units at Gate outside colony] -----------------------------
  auto units_at_gate_view = UnitsAtGateColonyView::create(
      ss, ts, colony, p_cargo_view,
      middle_strip_size.with_width( middle_strip_size.w / 3 )
          .with_height( middle_strip_size.h - 32 ) );
  g_composition.entities[e_colview_entity::units_at_gate] =
      units_at_gate_view.get();
  pos =
      Coord{ .x = population_right_edge, .y = middle_strip_top };
  views.push_back( ui::OwningPositionedView{
      .view = std::move( units_at_gate_view ), .coord = pos } );

  // [Production] -----------------------------------------------
  auto production_view = ProductionView::create(
      ss, ts, colony,
      middle_strip_size.with_width( middle_strip_size.w / 3 ) );
  g_composition.entities[e_colview_entity::production] =
      production_view.get();
  pos = Coord{ .x = cargo_right_edge, .y = middle_strip_top };
  views.push_back( ui::OwningPositionedView{
      .view = std::move( production_view ), .coord = pos } );

  // [ColonyLandView] -------------------------------------------
  available = Delta{ canvas_size.w,
                     middle_strip_top - title_bar_bottom };

  H max_landview_height = available.h;

  ColonyLandView::e_render_mode land_view_mode =
      ColonyLandView::e_render_mode::_6x6;
  if( ColonyLandView::size_needed( land_view_mode ).h >
      max_landview_height )
    land_view_mode = ColonyLandView::e_render_mode::_5x5;
  if( ColonyLandView::size_needed( land_view_mode ).h >
      max_landview_height )
    land_view_mode = ColonyLandView::e_render_mode::_3x3;
  auto land_view = ColonyLandView::create(
      ss, ts, colony, player, land_view_mode );
  g_composition.entities[e_colview_entity::land] =
      land_view.get();
  pos = g_composition.entities[e_colview_entity::title_bar]
            ->view()
            .rect( Coord{} )
            .lower_right() -
        Delta{ .w = land_view->delta().w };
  X const land_view_left_edge = pos.x;
  views.push_back( ui::OwningPositionedView{
      .view = std::move( land_view ), .coord = pos } );

  // [Buildings] ------------------------------------------------
  Delta buildings_size{
      .w = land_view_left_edge - 0,
      .h = middle_strip_top - title_bar_bottom };
  auto buildings = ColViewBuildings::create(
      ss, ts, colony, buildings_size, player );
  g_composition.entities[e_colview_entity::buildings] =
      buildings.get();
  pos = Coord{ .x = 0, .y = title_bar_bottom };
  views.push_back( ui::OwningPositionedView{
      .view = std::move( buildings ), .coord = pos } );

  // [Finish] ---------------------------------------------------
  auto invisible_view = std::make_unique<CompositeColSubView>(
      ss, ts, colony, canvas_size, std::move( views ), player );
  invisible_view->set_delta( canvas_size );
  g_composition.top_level = std::move( invisible_view );

  for( auto e : refl::enum_values<e_colview_entity> ) {
    CHECK( g_composition.entities.contains( e ),
           "colview entity {} is missing.", e );
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ColonySubView& colview_top_level() {
  return *g_composition.top_level;
}

// FIXME: a lot of this needs to be de-duped with the corre-
// sponding code in old-world-view.
void colview_drag_n_drop_draw(
    SS& ss, rr::Renderer& renderer,
    DragState<ColViewObject_t> const& state,
    Coord const&                      canvas_origin ) {
  Coord sprite_upper_left = state.where - state.click_offset +
                            canvas_origin.distance_from_origin();
  using namespace ColViewObject;
  // Render the dragged item.
  overload_visit(
      state.object,
      [&]( unit const& o ) {
        render_unit( renderer, sprite_upper_left,
                     ss.units.unit_for( o.id ),
                     UnitRenderOptions{ .flag = false } );
      },
      [&]( commodity const& o ) {
        render_commodity( renderer, sprite_upper_left,
                          o.comm.type );
      } );
  // Render any indicators on top of it.
  switch( state.indicator ) {
    using e = e_drag_status_indicator;
    case e::none: break;
    case e::bad: {
      rr::Typer typer =
          renderer.typer( sprite_upper_left, gfx::pixel::red() );
      typer.write( "X" );
      break;
    }
    case e::good: {
      rr::Typer typer = renderer.typer( sprite_upper_left,
                                        gfx::pixel::green() );
      typer.write( "+" );
      if( state.user_requests_input ) {
        auto mod_pos = state.where;
        mod_pos.y -=
            H{ rr::rendered_text_line_size_pixels( "?" ).h };
        mod_pos -= state.click_offset;
        auto typer_mod =
            renderer.typer( mod_pos, gfx::pixel::green() );
        typer_mod.write( "?" );
      }
      break;
    }
  }
}

ColonyProduction const& colview_production() {
  return g_production;
}

void update_colony_view( SSConst const& ss,
                         Colony const&  colony ) {
  update_production( ss, colony );
  ColonySubView& top = colview_top_level();
  top.update_this_and_children();
}

void update_production( SSConst const& ss,
                        Colony const&  colony ) {
  g_production = production_for_colony( ss, colony );
}

void set_colview_colony( SS& ss, TS& ts, Colony& colony ) {
  update_production( ss, colony );
  // TODO: compute squares around this colony that are being
  // worked by other colonies.
  UNWRAP_CHECK( normal, compositor::section(
                            compositor::e_section::normal ) );
  recomposite( ss, ts, colony, normal.delta() );
}

} // namespace rn
