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
#include "commodity.hpp"
#include "compositor.hpp"
#include "construction.hpp"
#include "cstate.hpp"
#include "gui.hpp"
#include "land-production.hpp"
#include "logger.hpp"
#include "map-updater.hpp"
#include "on-map.hpp"
#include "plow.hpp"
#include "production.hpp"
#include "render-terrain.hpp"
#include "render.hpp"
#include "road.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "ts.hpp"
#include "ustate.hpp"
#include "views.hpp"
#include "window.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
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

/****************************************************************
** Casting
*****************************************************************/
// These are used to test views for supported interfaces and to
// then get references to the views as those interfaces.

maybe<IColViewDragSourceUserInput const&>
IColViewDragSource::drag_user_input() const {
  return base::maybe_dynamic_cast<
      IColViewDragSourceUserInput const&>( *this );
}

maybe<IColViewDragSinkCheck const&>
IColViewDragSink::drag_check() const {
  return base::maybe_dynamic_cast<IColViewDragSinkCheck const&>(
      *this );
}

maybe<IColViewDragSource&> ColonySubView::drag_source() {
  return base::maybe_dynamic_cast<IColViewDragSource&>( *this );
}

maybe<IColViewDragSink&> ColonySubView::drag_sink() {
  return base::maybe_dynamic_cast<IColViewDragSink&>( *this );
}

namespace {

/****************************************************************
** Constants
*****************************************************************/
constexpr Delta kCommodityTileSize = Delta{ .w = 16, .h = 16 };

constexpr W kCommodityTileWidth = 16;

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

e_tile tile_for_outdoor_job( e_outdoor_job job ) {
  switch( job ) {
    case e_outdoor_job::food: return e_tile::commodity_food;
    case e_outdoor_job::fish: return e_tile::product_fish;
    case e_outdoor_job::sugar: return e_tile::commodity_sugar;
    case e_outdoor_job::tobacco:
      return e_tile::commodity_tobacco;
    case e_outdoor_job::cotton: return e_tile::commodity_cotton;
    case e_outdoor_job::fur: return e_tile::commodity_fur;
    case e_outdoor_job::lumber: return e_tile::commodity_lumber;
    case e_outdoor_job::ore: return e_tile::commodity_ore;
    case e_outdoor_job::silver: return e_tile::commodity_silver;
  }
}

maybe<string> check_abandon( Colony const& colony ) {
  if( colony_population( colony ) == 1 )
    return "This action would abandon the colony, which is not "
           "yet supported.";
  return nothing;
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

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::title_bar;
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

class MarketCommodities : public ui::View,
                          public ColonySubView,
                          public IColViewDragSource,
                          public IColViewDragSourceUserInput,
                          public IColViewDragSink {
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

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::commodities;
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

  maybe<ColViewObjectWithBounds> object_here(
      Coord const& coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    auto sprite_scale =
        Delta{ .w = SX{ block_width_ }, .h = SY{ 32 } };
    auto box_upper_left =
        ( coord / sprite_scale ) * sprite_scale;
    auto idx = ( coord / sprite_scale - Coord{} ).w;
    UNWRAP_CHECK( type, commodity_from_index( idx ) );
    int quantity = quantity_of( type );
    if( quantity == 0 ) return nothing;
    return ColViewObjectWithBounds{
        .obj    = ColViewObject::commodity{ Commodity{
               .type = type, .quantity = quantity } },
        .bounds = Rect::from(
            box_upper_left + rendered_commodity_offset(),
            Delta{ .w = 1, .h = 1 } * kCommodityTileSize ) };
  }

  bool try_drag( ColViewObject_t const& o,
                 Coord const&           where ) override {
    UNWRAP_CHECK( [c], o.get_if<ColViewObject::commodity>() );
    // Sanity checks.
    CHECK( c.quantity > 0 );
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

  void disown_dragged_object() override {
    CHECK( dragging_ );
    e_commodity type = dragging_->type;
    int new_quantity = quantity_of( type ) - dragging_->quantity;
    CHECK( new_quantity >= 0 );
    colony_.commodities[type] = new_quantity;
  }

  maybe<ColViewObject_t> can_receive(
      ColViewObject_t const& o, e_colview_entity /*from*/,
      Coord const&           where ) const override {
    CHECK( where.is_inside( rect( {} ) ) );
    if( o.holds<ColViewObject::commodity>() ) return o;
    return nothing;
  }

  void drop( ColViewObject_t const& o,
             Coord const& /*where*/ ) override {
    UNWRAP_CHECK( [c], o.get_if<ColViewObject::commodity>() );
    int q = colony_.commodities[c.type];
    q += c.quantity;
    colony_.commodities[c.type] = q;
  }

  wait<maybe<ColViewObject_t>> user_edit_object()
      const override {
    CHECK( dragging_ );
    int    min  = 1;
    int    max  = dragging_->quantity;
    string text = fmt::format(
        "What quantity of @[H]{}@[] would you like to move? "
        "({}-{}):",
        commodity_display_name( dragging_->type ), min, max );
    maybe<int> quantity =
        co_await ts_.gui.int_input( { .msg           = text,
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

class PopulationView : public ui::View, public ColonySubView {
 public:
  static unique_ptr<PopulationView> create( SS& ss, TS& ts,
                                            Colony& colony,
                                            Delta   size ) {
    return make_unique<PopulationView>( ss, ts, colony, size );
  }

  PopulationView( SS& ss, TS& ts, Colony& colony, Delta size )
    : ColonySubView( ss, ts, colony ), size_( size ) {}

  Delta delta() const override { return size_; }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::population;
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
    auto const& colony   = ss_.colonies.colony_for( colony_.id );
    vector<UnitId> units = colony_units_all( colony );
    auto           unit_pos = coord + Delta{ .h = 16 };
    for( UnitId unit_id : units ) {
      render_unit( renderer, unit_pos, unit_id,
                   UnitRenderOptions{ .flag = false } );
      unit_pos.x += 24;
    }
  }

 private:
  Delta size_;
};

class CargoView : public ui::View,
                  public ColonySubView,
                  public IColViewDragSource,
                  public IColViewDragSourceUserInput,
                  public IColViewDragSink,
                  public IColViewDragSinkCheck {
 public:
  static unique_ptr<CargoView> create( SS& ss, TS& ts,
                                       Colony& colony,
                                       Delta   size ) {
    return make_unique<CargoView>( ss, ts, colony, size );
  }

  CargoView( SS& ss, TS& ts, Colony& colony, Delta size )
    : ColonySubView( ss, ts, colony ), size_( size ) {}

  Delta delta() const override { return size_; }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::cargo;
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
        slot_idx < unit_from_id( *holder_ ).desc().cargo_slots;
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
        slot < unit_from_id( *holder_ ).desc().cargo_slots;
    return pair{ is_open,
                 Rect::from( slot_upper_left, g_tile_delta ) };
  }

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override {
    rr::Painter painter = renderer.painter();
    painter.draw_empty_rect( rect( coord ),
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel::black() );
    auto unit = holder_.fmap( unit_from_id );
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
                    renderer, rect.upper_left(), u.id,
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
      ColViewObject_t const& o, e_colview_entity from,
      Coord const& where ) const override {
    CHECK( where.is_inside( rect( {} ) ) );
    if( !holder_ ) return nothing;
    maybe<pair<bool, int>> slot_info =
        slot_idx_from_coord( where );
    if( !slot_info.has_value() ) return nothing;
    auto [is_open, slot_idx] = *slot_info;
    if( !is_open ) return nothing;
    if( from == e_colview_entity::cargo ) {
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
    auto& unit = unit_from_id( *holder_ );
    switch( o.to_enum() ) {
      using namespace ColViewObject;
      case e::unit: {
        UnitId id = o.get<ColViewObject::unit>().id;
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

  wait<base::valid_or<IColViewDragSinkCheck::Rejection>> check(
      ColViewObject_t const&, e_colview_entity from,
      Coord const ) const override {
    switch( from ) {
      case e_colview_entity::units_at_gate:
      case e_colview_entity::cargo:
      case e_colview_entity::commodities: //
        co_return base::valid;
      case e_colview_entity::land:
      case e_colview_entity::buildings: //
        if( auto msg = check_abandon( colony_ );
            msg.has_value() )
          co_return IColViewDragSinkCheck::Rejection{ .reason =
                                                          *msg };
        if( auto msg = check_seige(); msg.has_value() )
          co_return IColViewDragSinkCheck::Rejection{ .reason =
                                                          *msg };
        co_return base::valid;
      case e_colview_entity::population:
      case e_colview_entity::title_bar:
      case e_colview_entity::production:
        FATAL( "unexpected source entity." );
    }
  }

  void drop( ColViewObject_t const& o,
             Coord const&           where ) override {
    CHECK( holder_ );
    auto&   cargo_hold = unit_from_id( *holder_ ).cargo();
    Cargo_t cargo      = to_cargo( o );
    CHECK( cargo_hold.fits_somewhere( ss_.units, cargo ) );
    UNWRAP_CHECK( slot_info, slot_idx_from_coord( where ) );
    auto [is_open, slot_idx] = slot_info;
    overload_visit(
        cargo, //
        [this, slot_idx = slot_idx]( Cargo::unit u ) {
          ss_.units.change_to_cargo_somewhere(
              *holder_, u.id, /*starting_slot=*/slot_idx );
        },
        [this,
         slot_idx = slot_idx]( Cargo::commodity const& c ) {
          add_commodity_to_cargo( ss_.units, c.obj, *holder_,
                                  slot_idx,
                                  /*try_other_slots=*/true );
        } );
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
        unit_from_id( *holder_ )
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

  maybe<ColViewObjectWithBounds> object_here(
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
    return ColViewObjectWithBounds{
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

  void disown_dragged_object() override {
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
              unit_from_id( *holder_ )
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
                         commodity_display_name( comm.obj.type ), min, max );
    maybe<int> quantity =
        co_await ts_.gui.int_input( { .msg           = text,
                                      .initial_value = max,
                                      .min           = min,
                                      .max           = max } );
    if( !quantity ) co_return nothing;
    Commodity new_comm = comm.obj;
    new_comm.quantity  = *quantity;
    CHECK( new_comm.quantity > 0 );
    co_return from_cargo( Cargo::commodity{ new_comm } );
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

class UnitsAtGateColonyView : public ui::View,
                              public ColonySubView,
                              public IColViewDragSource,
                              public IColViewDragSink,
                              public IColViewDragSinkCheck {
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
    update();
  }

  Delta delta() const override { return size_; }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::units_at_gate;
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
      Coord draw_pos = unit_pos.as_if_origin_were( coord );
      render_unit( renderer, draw_pos, unit_id,
                   UnitRenderOptions{ .flag = true } );
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

  maybe<ColViewObjectWithBounds> object_here(
      Coord const& where ) const override {
    for( PositionedUnit const& pu : positioned_units_ ) {
      auto rect = Rect::from( pu.pos, g_tile_delta );
      if( where.is_inside( rect ) )
        return ColViewObjectWithBounds{
            .obj    = ColViewObject::unit{ .id = pu.id },
            .bounds = rect };
    }
    return nothing;
  }

  maybe<ColViewObject_t> can_receive_unit(
      UnitId       dragged, e_colview_entity /*from*/,
      Coord const& where ) const {
    auto& unit = unit_from_id( dragged );
    // Player should not be dragging ships or wagons.
    CHECK( unit.desc().cargo_slots == 0 );
    // See if the draga target is over top of a unit.
    maybe<UnitId> over_unit_id = contains_unit( where );
    if( !over_unit_id ) {
      // The player is moving a unit outside of the colony, let's
      // check if the unit is already outside the colony, in
      // which case there is no reason to drag the unit here.
      if( is_unit_on_map( dragged ) ) return nothing;
      // The player is moving the unit outside the colony, which
      // is always allowed, at least for now. If the unit is in
      // the colony (as opposed to cargo) and there is a stockade
      // then we won't allow the population to be reduced below
      // three, but that will be checked in the confirmation
      // stage.
      return ColViewObject::unit{ .id = dragged };
    }
    Unit const& target_unit = unit_from_id( *over_unit_id );
    if( target_unit.desc().cargo_slots == 0 ) return nothing;
    // Check if the target_unit is already holding the dragged
    // unit.
    maybe<UnitId> maybe_holder_of_dragged =
        is_unit_onboard( dragged );
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

  wait<base::valid_or<IColViewDragSinkCheck::Rejection>> check(
      ColViewObject_t const&, e_colview_entity from,
      Coord const ) const override {
    switch( from ) {
      case e_colview_entity::units_at_gate:
      case e_colview_entity::commodities:
      case e_colview_entity::cargo: //
        co_return base::valid;
      case e_colview_entity::land:
      case e_colview_entity::buildings: //
        if( auto msg = check_abandon( colony_ );
            msg.has_value() )
          co_return IColViewDragSinkCheck::Rejection{ .reason =
                                                          *msg };
        if( auto msg = check_seige(); msg.has_value() )
          co_return IColViewDragSinkCheck::Rejection{ .reason =
                                                          *msg };
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
    Unit const& target_unit = unit_from_id( cargo_unit_id );
    CHECK( target_unit.desc().cargo_slots != 0 );
    // Check if the target_unit is already holding the dragged
    // commodity.
    if( from == e_colview_entity::cargo ) {
      CHECK( selected_.has_value() );
      CHECK( unit_from_id( *selected_ ).desc().cargo_slots > 0 );
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
                       unit_from_id( id ), comm ) );
    return ColViewObject::commodity{
        .comm = with_quantity( comm, xform_res.quantity_used ) };
  }

  maybe<ColViewObject_t> can_receive_commodity(
      Commodity const& comm, e_colview_entity from,
      Coord const& where ) const {
    maybe<UnitId> over_unit_id = contains_unit( where );
    if( !over_unit_id ) return nothing;
    Unit const& target_unit = unit_from_id( *over_unit_id );
    if( target_unit.desc().cargo_slots != 0 )
      return can_cargo_unit_receive_commodity( comm, from,
                                               *over_unit_id );
    else
      return can_unit_receive_commodity( comm, from,
                                         *over_unit_id );
  }

  maybe<ColViewObject_t> can_receive(
      ColViewObject_t const& o, e_colview_entity from,
      Coord const& where ) const override {
    CHECK( where.is_inside( rect( {} ) ) );
    if( !where.is_inside( rect( {} ) ) ) return nothing;
    return overload_visit(
        o, //
        [&]( ColViewObject::unit const& unit ) {
          return can_receive_unit( unit.id, from, where );
        },
        [&]( ColViewObject::commodity const& comm ) {
          return can_receive_commodity( comm.comm, from, where );
        } );
  }

  void drop( ColViewObject_t const& o,
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
            TrappingMapUpdater map_updater;
            unit_to_map_square_non_interactive(
                ss_.units, map_updater, unit.id,
                colony_.location );
            // This is not strictly necessary, but as a conve-
            // nience to the user, clear the orders, otherwise it
            // would be sentry'd, which is probably not what the
            // player wants.
            unit_from_id( unit.id ).clear_orders();
          }
        },
        [&]( ColViewObject::commodity const& comm ) {
          CHECK( target_unit );
          Unit& unit = unit_from_id( *target_unit );
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
          }
        } );
  }

  bool try_drag( ColViewObject_t const& o,
                 Coord const& /*where*/ ) override {
    UNWRAP_CHECK( [id], o.get_if<ColViewObject::unit>() );
    bool is_cargo_unit =
        unit_from_id( id ).desc().cargo_slots > 0;
    if( is_cargo_unit ) return false;
    dragging_ = id;
    return true;
  }

  void cancel_drag() override { dragging_ = nothing; }

  void disown_dragged_object() override {
    UNWRAP_CHECK( unit_id, dragging_ );
    ss_.units.disown_unit( unit_id );
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
    string mode = co_await ts_.gui.choice( config );
    if( mode == "orders" ) {
      ChoiceConfig config{
          .msg     = "Change unit orders to:",
          .options = {
              { .key = "clear", .display_name = "Clear Orders" },
              { .key = "sentry", .display_name = "Sentry" },
              { .key          = "fortify",
                .display_name = "Fortify" } } };
      string new_orders = co_await ts_.gui.choice( config );
      if( new_orders == "clear" )
        unit.clear_orders();
      else if( new_orders == "sentry" )
        unit.sentry();
      else if( new_orders == "fortify" )
        unit.fortify();
    } else if( mode == "strip" ) {
      // Clear orders just in case it is a pioneer building a
      // road or plowing; that would put the pioneer into an in-
      // consistent state.
      if( unit.orders() == e_unit_orders::road ||
          unit.orders() == e_unit_orders::plow )
        unit.clear_orders();
      strip_unit_commodities( unit, colony_ );
    }
  }

  void update() override {
    auto const& colony   = ss_.colonies.colony_for( colony_.id );
    auto const& units    = units_from_coord( colony.location );
    auto        unit_pos = Coord{} + Delta{ .w = 1, .h = 16 };
    positioned_units_.clear();
    maybe<UnitId> first_with_cargo;
    for( auto unit_id : units ) {
      positioned_units_.push_back(
          { .id = unit_id, .pos = unit_pos } );
      unit_pos.x += 32;
      if( !first_with_cargo.has_value() &&
          unit_from_id( unit_id ).desc().cargo_slots > 0 )
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

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::production;
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
    SCOPED_RENDERER_MOD_ADD( painter_mods.repos.translation,
                             coord.distance_from_origin() );
    rr::Typer typer = renderer.typer( Coord{ .x = 2, .y = 2 },
                                      gfx::pixel::black() );
    typer.write( "Hammers:      {}\n", colony_.hammers );
    typer.write( "Construction: " );
    if( colony_.construction.has_value() )
      typer.write( "{}\n",
                   construction_name( *colony_.construction ) );
    else
      typer.write( "nothing\n" );
  }

  // Implement AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& event ) override {
    CHECK( event.pos.is_inside( rect( {} ) ) );
    co_await select_colony_construction( colony_, ts_.gui );
  }

 private:
  Delta size_;
};

class LandView : public ui::View,
                 public ColonySubView,
                 public IColViewDragSource,
                 public IColViewDragSink,
                 public IColViewDragSinkCheck {
 public:
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

  static unique_ptr<LandView> create( SS& ss, TS& ts,
                                      Colony&       colony,
                                      Player const& player,
                                      e_render_mode mode ) {
    return make_unique<LandView>( ss, ts, colony, player, mode );
  }

  LandView( SS& ss, TS& ts, Colony& colony, Player const& player,
            e_render_mode mode )
    : ColonySubView( ss, ts, colony ),
      player_( player ),
      mode_( mode ) {}

  static Delta size_needed( e_render_mode mode ) {
    int side_length_in_squares = 3;
    switch( mode ) {
      case e_render_mode::_3x3:
        side_length_in_squares = 3;
        break;
      case e_render_mode::_5x5:
        side_length_in_squares = 5;
        break;
      case e_render_mode::_6x6:
        side_length_in_squares = 6;
        break;
    }
    return Delta{ .w = 32, .h = 32 } *
           Delta{ .w = side_length_in_squares,
                  .h = side_length_in_squares };
  }

  maybe<e_direction> direction_under_cursor(
      Coord coord ) const {
    switch( mode_ ) {
      case e_render_mode::_3x3:
        return Coord{ .x = 1, .y = 1 }.direction_to(
            coord / g_tile_delta );
      case e_render_mode::_5x5: {
        // TODO: this will probably have to be made more sophis-
        // ticated.
        Coord shifted = coord - g_tile_delta;
        if( shifted.x < 0 || shifted.y < 0 ) return nothing;
        return Coord{ .x = 1, .y = 1 }.direction_to(
            shifted / g_tile_delta );
      }
      case e_render_mode::_6x6:
        return Coord{ .x = 1, .y = 1 }.direction_to(
            coord / ( g_tile_delta * Delta{ .w = 2, .h = 2 } ) );
    }
  }

  // Gets the bounding rectangle for the unit position on the
  // given square (whether there actually is a unit there or
  // not).
  Rect rect_for_unit( e_direction d ) const {
    switch( mode_ ) {
      case e_render_mode::_3x3:
        return Rect::from(
            Coord{ .x = 1, .y = 1 }.moved( d ) * g_tile_delta,
            g_tile_delta );
      case e_render_mode::_5x5: {
        NOT_IMPLEMENTED;
      }
      case e_render_mode::_6x6:
        return Rect::from(
            Coord{ .x = 1, .y = 1 }.moved( d ) * g_tile_delta *
                    Delta{ .w = 2, .h = 2 } +
                ( g_tile_delta / Delta{ .w = 2, .h = 2 } ),
            g_tile_delta );
    }
  }

  maybe<UnitId> unit_for_direction( e_direction d ) const {
    Colony& colony = ss_.colonies.colony_for( colony_.id );
    return colony.outdoor_jobs[d].member(
        &OutdoorUnit::unit_id );
  }

  maybe<e_outdoor_job> job_for_direction( e_direction d ) const {
    Colony& colony = ss_.colonies.colony_for( colony_.id );
    return colony.outdoor_jobs[d].member( &OutdoorUnit::job );
  }

  maybe<UnitId> unit_under_cursor( Coord where ) const {
    UNWRAP_RETURN( d, direction_under_cursor( where ) );
    return unit_for_direction( d );
  }

  Delta delta() const override { return size_needed( mode_ ); }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::land;
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  // Implement AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& event ) override {
    CHECK( event.pos.is_inside( rect( {} ) ) );
    maybe<UnitId> unit_id = unit_under_cursor( event.pos );
    if( !unit_id.has_value() ) co_return;

    EnumChoiceConfig     config{ .msg = "Select Occupation",
                                 .choice_required = false };
    maybe<e_outdoor_job> new_job = co_await ts_.gui.enum_choice(
        config, config_colony.outdoors.job_names );
    if( !new_job.has_value() ) co_return;
    Colony& colony = ss_.colonies.colony_for( colony_.id );
    change_unit_outdoor_job( colony, *unit_id, *new_job );
    update_production( ss_, player_, colony_ );
  }

  maybe<ColViewObject_t> can_receive(
      ColViewObject_t const& o, e_colview_entity,
      Coord const&           where ) const override {
    // Verify that the dragged object is a unit.
    maybe<UnitId> unit_id =
        o.get_if<ColViewObject::unit>().member(
            &ColViewObject::unit::id );
    if( !unit_id.has_value() ) return nothing;
    // Check if the unit is a human.
    Unit const& unit = ss_.units.unit_for( *unit_id );
    if( !unit.is_human() ) return nothing;
    // Check if there is a land square under the cursor that is
    // not the center.
    maybe<e_direction> d = direction_under_cursor( where );
    if( !d.has_value() ) return nothing;
    // Check if this is the same unit currently being dragged, if
    // so we'll allow it.
    if( dragging_.has_value() && d == dragging_->d ) return o;
    // Check if there is already a unit on the square.
    if( unit_under_cursor( where ).has_value() ) return nothing;
    // Note that we don't check for water/docks here; that is
    // done in the check function.

    // We're good to go.
    return o;
  }

  wait<base::valid_or<IColViewDragSinkCheck::Rejection>> check(
      ColViewObject_t const&, e_colview_entity,
      Coord const where ) const override {
    Colony const& colony = ss_.colonies.colony_for( colony_.id );
    TerrainState const& terrain_state = ss_.terrain;
    maybe<e_direction>  d = direction_under_cursor( where );
    CHECK( d );
    MapSquare const& square =
        terrain_state.square_at( colony.location.moved( *d ) );

    if( is_water( square ) &&
        !colony_has_building_level(
            colony, e_colony_building::docks ) ) {
      co_return IColViewDragSinkCheck::Rejection{
          .reason =
              "We must build @[H]docks@[] in this colony in "
              "order to work on sea squares." };
    }

    if( square.lost_city_rumor ) {
      co_return IColViewDragSinkCheck::Rejection{
          .reason =
              "We must explore this Lost City Rumor before we "
              "can work this square." };
    }

    co_return base::valid;
  }

  ColonyJob_t make_job_for_square( e_direction d ) const {
    // The original game seems to always choose food.
    return ColonyJob::outdoor{ .direction = d,
                               .job = e_outdoor_job::food };
  }

  void drop( ColViewObject_t const& o,
             Coord const&           where ) override {
    UNWRAP_CHECK( unit_id,
                  o.get_if<ColViewObject::unit>().member(
                      &ColViewObject::unit::id ) );
    Colony& colony = ss_.colonies.colony_for( colony_.id );
    UNWRAP_CHECK( d, direction_under_cursor( where ) );
    ColonyJob_t job = make_job_for_square( d );
    if( dragging_.has_value() ) {
      // The unit being dragged is coming from another square on
      // the land view, so keep its job the same.
      job = ColonyJob::outdoor{ .direction = d,
                                .job       = dragging_->job };
    }
    move_unit_to_colony( ss_.units, colony, unit_id, job );
    CHECK_HAS_VALUE( colony.validate() );
  }

  maybe<ColViewObjectWithBounds> object_here(
      Coord const& where ) const override {
    UNWRAP_RETURN( unit_id, unit_under_cursor( where ) );
    UNWRAP_RETURN( d, direction_under_cursor( where ) );
    return ColViewObjectWithBounds{
        .obj    = ColViewObject::unit{ .id = unit_id },
        .bounds = rect_for_unit( d ) };
  }

  struct Draggable {
    e_direction   d   = {};
    e_outdoor_job job = {};
  };

  bool try_drag( ColViewObject_t const&,
                 Coord const& where ) override {
    UNWRAP_CHECK( d, direction_under_cursor( where ) );
    UNWRAP_CHECK( job, job_for_direction( d ) );
    dragging_ = Draggable{ .d = d, .job = job };
    return true;
  }

  void cancel_drag() override { dragging_ = nothing; }

  void disown_dragged_object() override {
    UNWRAP_CHECK( draggable, dragging_ );
    UNWRAP_CHECK( unit_id, unit_for_direction( draggable.d ) );
    Colony& colony = ss_.colonies.colony_for( colony_.id );
    remove_unit_from_colony( ss_.units, colony, unit_id );
  }

  void draw_land_3x3( rr::Renderer& renderer,
                      Coord         coord ) const {
    SCOPED_RENDERER_MOD_ADD( painter_mods.repos.translation,
                             coord.distance_from_origin() );

    TerrainState const& terrain_state = ss_.terrain;
    // FIXME: Should not be duplicating land-view rendering code
    // here.
    rr::Painter painter = renderer.painter();
    auto const& colony  = ss_.colonies.colony_for( colony_.id );
    Coord       world_square = colony.location;
    // Render terrain.
    for( auto local_coord :
         Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) {
      auto render_square = world_square +
                           local_coord.distance_from_origin() -
                           Delta{ .w = 1, .h = 1 };
      render_terrain_square(
          terrain_state, renderer, local_coord * g_tile_delta,
          render_square, TerrainRenderOptions{} );
    }
    // Render irrigation.
    for( auto local_coord :
         Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) {
      auto render_square = world_square +
                           local_coord.distance_from_origin() -
                           Delta{ .w = 1, .h = 1 };
      render_plow_if_present( painter,
                              local_coord * g_tile_delta,
                              terrain_state, render_square );
    }
    // Render roads.
    for( auto local_coord :
         Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) {
      auto render_square = world_square +
                           local_coord.distance_from_origin() -
                           Delta{ .w = 1, .h = 1 };
      render_road_if_present( painter,
                              local_coord * g_tile_delta,
                              terrain_state, render_square );
    }
    // Render colonies.
    for( auto local_coord :
         Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) {
      auto render_square = world_square +
                           local_coord.distance_from_origin() -
                           Delta{ .w = 1, .h = 1 };
      auto maybe_col_id = colony_from_coord( render_square );
      if( !maybe_col_id ) continue;
      render_colony(
          painter,
          local_coord * g_tile_delta - Delta{ .w = 6, .h = 6 },
          *maybe_col_id );
    }
  }

  void draw_land_6x6( rr::Renderer& renderer,
                      Coord         coord ) const {
    {
      SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale, 2.0 );
      draw_land_3x3( renderer, coord );
    }
    // Further drawing should not be scaled.

    // Render units.
    rr::Painter   painter = renderer.painter();
    Colony const& colony = ss_.colonies.colony_for( colony_.id );
    Coord const   center = Coord{ .x = 1, .y = 1 };

    for( auto const& [direction, outdoor_unit] :
         colony.outdoor_jobs ) {
      if( !outdoor_unit.has_value() ) continue;
      if( dragging_.has_value() && dragging_->d == direction )
        continue;
      Coord const square_coord =
          coord + ( center.moved( direction ) * g_tile_delta *
                    Delta{ .w = 2, .h = 2 } )
                      .distance_from_origin();
      Coord const unit_coord =
          square_coord +
          ( g_tile_delta / Delta{ .w = 2, .h = 2 } );
      UnitId const unit_id = outdoor_unit->unit_id;
      Unit const&  unit    = ss_.units.unit_for( unit_id );
      UnitTypeAttributes const& desc = unit_attr( unit.type() );
      render_unit_type(
          painter, unit_coord, desc.type,
          UnitRenderOptions{ .shadow = UnitShadow{} } );
      e_outdoor_job const job    = outdoor_unit->job;
      e_tile const product_tile  = tile_for_outdoor_job( job );
      Coord const  product_coord = square_coord;
      render_sprite( painter, product_coord, product_tile );
      Delta const product_tile_size =
          sprite_size( product_tile );
      SquareProduction const& production =
          colview_production().land_production[direction];
      int const    quantity = production.quantity;
      string const q_str    = fmt::format( "x {}", quantity );
      Coord const  text_coord =
          product_coord + Delta{ .w = product_tile_size.w };
      render_text_markup(
          renderer, text_coord, {},
          TextMarkupInfo{
              .shadowed_text_color   = gfx::pixel::white(),
              .shadowed_shadow_color = gfx::pixel::black() },
          fmt::format( "@[S]{}@[]", q_str ) );
    }
  }

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override {
    rr::Painter painter = renderer.painter();
    switch( mode_ ) {
      case e_render_mode::_3x3:
        draw_land_3x3( renderer, coord );
        break;
      case e_render_mode::_5x5:
        painter.draw_solid_rect( rect( coord ),
                                 gfx::pixel::wood() );
        draw_land_3x3( renderer, coord + g_tile_delta );
        break;
      case e_render_mode::_6x6:
        draw_land_6x6( renderer, coord );
        break;
    }
  }

 private:
  Player const&    player_;
  e_render_mode    mode_;
  maybe<Draggable> dragging_;
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
      UNWRAP_CHECK(
          shifted_event,
          input::move_mouse_origin_by(
              event, pos_view.coord.distance_from_origin() )
              .get_if<input::mouse_button_event_t>() );
      // Need to co_await so that shifted_event stays alive.
      co_await ptrs_[i]->perform_click( shifted_event );
    }
  }

  maybe<PositionedColSubView> view_here( Coord coord ) override {
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedView pos_view = at( i );
      if( !coord.is_inside( pos_view.rect() ) ) continue;
      maybe<PositionedColSubView> p_view = ptrs_[i]->view_here(
          coord.with_new_origin( pos_view.coord ) );
      if( !p_view ) continue;
      p_view->upper_left =
          p_view->upper_left.as_if_origin_were( pos_view.coord );
      return p_view;
    }
    if( coord.is_inside( rect( {} ) ) )
      return PositionedColSubView{ this, Coord{} };
    return nothing;
  }

  // Implement ColonySubView.
  maybe<ColViewObjectWithBounds> object_here(
      Coord const& coord ) const override {
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedViewConst pos_view = at( i );
      if( !coord.is_inside( pos_view.rect() ) ) continue;
      maybe<ColViewObjectWithBounds> obj = ptrs_[i]->object_here(
          coord.with_new_origin( pos_view.coord ) );
      if( !obj ) continue;
      obj->bounds =
          obj->bounds.as_if_origin_were( pos_view.coord );
      return obj;
    }
    // This view itself has no objects.
    return nothing;
  }

  maybe<e_colview_entity> entity() const override {
    return nothing;
  }

  void update() override {
    // FIXME: this seems like it should go somewhere else.
    update_production( ss_, player_, colony_ );
    for( ColonySubView* p : ptrs_ ) p->update();
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
  views.push_back(
      ui::OwningPositionedView( std::move( title_bar ), pos ) );

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
  views.push_back( ui::OwningPositionedView(
      std::move( market_commodities ), pos ) );

  // [Middle Strip] ---------------------------------------------
  Delta   middle_strip_size{ canvas_size.w, 32 + 32 + 16 };
  Y const middle_strip_top =
      market_commodities_top - middle_strip_size.h;

  // [Population] -----------------------------------------------
  auto population_view = PopulationView::create(
      ss, ts, colony,
      middle_strip_size.with_width( middle_strip_size.w / 3 ) );
  g_composition.entities[e_colview_entity::population] =
      population_view.get();
  pos = Coord{ .x = 0, .y = middle_strip_top };
  X const population_right_edge =
      population_view->rect( pos ).right_edge();
  views.push_back( ui::OwningPositionedView(
      std::move( population_view ), pos ) );

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
  views.push_back(
      ui::OwningPositionedView( std::move( cargo_view ), pos ) );

  // [Units at Gate outside colony] -----------------------------
  auto units_at_gate_view = UnitsAtGateColonyView::create(
      ss, ts, colony, p_cargo_view,
      middle_strip_size.with_width( middle_strip_size.w / 3 )
          .with_height( middle_strip_size.h - 32 ) );
  g_composition.entities[e_colview_entity::units_at_gate] =
      units_at_gate_view.get();
  pos =
      Coord{ .x = population_right_edge, .y = middle_strip_top };
  views.push_back( ui::OwningPositionedView(
      std::move( units_at_gate_view ), pos ) );

  // [Production] -----------------------------------------------
  auto production_view = ProductionView::create(
      ss, ts, colony,
      middle_strip_size.with_width( middle_strip_size.w / 3 ) );
  g_composition.entities[e_colview_entity::production] =
      production_view.get();
  pos = Coord{ .x = cargo_right_edge, .y = middle_strip_top };
  views.push_back( ui::OwningPositionedView(
      std::move( production_view ), pos ) );

  // [LandView] -------------------------------------------------
  available = Delta{ canvas_size.w,
                     middle_strip_top - title_bar_bottom };

  H max_landview_height = available.h;

  LandView::e_render_mode land_view_mode =
      LandView::e_render_mode::_6x6;
  if( LandView::size_needed( land_view_mode ).h >
      max_landview_height )
    land_view_mode = LandView::e_render_mode::_5x5;
  if( LandView::size_needed( land_view_mode ).h >
      max_landview_height )
    land_view_mode = LandView::e_render_mode::_3x3;
  auto land_view =
      LandView::create( ss, ts, colony, player, land_view_mode );
  g_composition.entities[e_colview_entity::land] =
      land_view.get();
  pos = g_composition.entities[e_colview_entity::title_bar]
            ->view()
            .rect( Coord{} )
            .lower_right() -
        Delta{ .w = land_view->delta().w };
  X const land_view_left_edge = pos.x;
  views.push_back(
      ui::OwningPositionedView( std::move( land_view ), pos ) );

  // [Buildings] ------------------------------------------------
  Delta buildings_size{
      .w = land_view_left_edge - 0,
      .h = middle_strip_top - title_bar_bottom };
  auto buildings = ColViewBuildings::create(
      ss, ts, colony, buildings_size, player );
  g_composition.entities[e_colview_entity::buildings] =
      buildings.get();
  pos = Coord{ .x = 0, .y = title_bar_bottom };
  views.push_back(
      ui::OwningPositionedView( std::move( buildings ), pos ) );

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
ColonySubView& colview_entity( e_colview_entity ) {
  NOT_IMPLEMENTED;
}

ColonySubView& colview_top_level() {
  return *g_composition.top_level;
}

// FIXME: a lot of this needs to be de-duped with the corre-
// sponding code in old-world-view.
void colview_drag_n_drop_draw(
    rr::Renderer&                       renderer,
    drag::State<ColViewObject_t> const& state,
    Coord const&                        canvas_origin ) {
  Coord sprite_upper_left = state.where - state.click_offset +
                            canvas_origin.distance_from_origin();
  using namespace ColViewObject;
  // Render the dragged item.
  overload_visit(
      state.object,
      [&]( unit const& o ) {
        render_unit( renderer, sprite_upper_left, o.id,
                     UnitRenderOptions{ .flag = false } );
      },
      [&]( commodity const& o ) {
        render_commodity( renderer, sprite_upper_left,
                          o.comm.type );
      } );
  // Render any indicators on top of it.
  switch( state.indicator ) {
    using e = drag::e_status_indicator;
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

void update_production( SSConst const& ss, Player const& player,
                        Colony const& colony ) {
  g_production = production_for_colony( ss.terrain, ss.units,
                                        player, colony );
}

void set_colview_colony( SS& ss, TS& ts, Colony& colony ) {
  UNWRAP_CHECK( player, ss.players.players[colony.nation] );
  update_production( ss, player, colony );
  // TODO: compute squares around this colony that are being
  // worked by other colonies.
  UNWRAP_CHECK( normal, compositor::section(
                            compositor::e_section::normal ) );
  recomposite( ss, ts, colony, normal.delta() );
}

} // namespace rn
