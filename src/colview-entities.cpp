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
#include "colony.hpp"
#include "commodity.hpp"
#include "cstate.hpp"
#include "gfx.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "terrain.hpp"
#include "text.hpp"
#include "ustate.hpp"
#include "views.hpp"
#include "waitable-coro.hpp"
#include "window.hpp"

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

maybe<IColViewDragSinkConfirm const&>
IColViewDragSink::drag_confirm() const {
  return base::maybe_dynamic_cast<
      IColViewDragSinkConfirm const&>( *this );
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
constexpr Delta kCommodityTileSize  = Delta{ 16_w, 16_h };
constexpr Scale kCommodityTileScale = Scale{ 16_sx, 16_sy };

constexpr W kCommodityTileWidth = 16_w;

/****************************************************************
** Globals
*****************************************************************/
struct ColViewComposited {
  ColonyId                                        id;
  Delta                                           screen_size;
  unique_ptr<ColonySubView>                       top_level;
  unordered_map<e_colview_entity, ColonySubView*> entities;
};

ColViewComposited g_composition;

/****************************************************************
** Helpers
*****************************************************************/
ColonyId colony_id() { return g_composition.id; }

Colony& colony() { return colony_from_id( colony_id() ); }

Cargo to_cargo( ColViewObject_t const& o ) {
  switch( o.to_enum() ) {
    using namespace ColViewObject;
    case e::unit: return o.get<ColViewObject::unit>().id;
    case e::commodity: return o.get<commodity>().comm;
  }
}

/****************************************************************
** Entities
*****************************************************************/
class TitleBar : public ui::View, public ColonySubView {
public:
  Delta delta() const override { return size_; }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::title_bar;
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  string title() const {
    auto const& colony = colony_from_id( colony_id() );
    return fmt::format( "{}, population {}", colony.name(),
                        colony.population() );
  }

  void draw( Texture& tx, Coord coord ) const override {
    render_fill_rect( tx, Color::wood(), rect( coord ) );
    Texture const& name = render_text(
        font::standard(), Color::banana(), title() );
    name.copy_to( tx, centered( name.size(), rect( coord ) ) );
  }

  static unique_ptr<TitleBar> create( Delta size ) {
    return make_unique<TitleBar>( size );
  }

  TitleBar( Delta size ) : size_( size ) {}

private:
  Delta size_;
};

class MarketCommodities : public ui::View,
                          public ColonySubView,
                          public IColViewDragSource {
public:
  Delta delta() const override {
    return Delta{
        block_width_ * SX{ enum_traits<e_commodity>::count },
        1_h * 32_sy };
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
    res.h = 3_h;
    res.w = ( block_width_ - kCommodityTileWidth ) / 2_sx;
    if( res.w < 0_w ) res.w = 0_w;
    return res;
  }

  void draw( Texture& tx, Coord coord ) const override {
    auto  comm_it = enum_traits<e_commodity>::values.begin();
    auto  label   = CommodityLabel::quantity{ 0 };
    Coord pos     = coord;
    auto const& colony = colony_from_id( colony_id() );
    for( int i = 0; i < kNumCommodityTypes; ++i ) {
      auto rect = Rect::from( pos, Delta{ 32_h, block_width_ } );
      render_rect( tx, Color::black(), rect );
      label.value = colony.commodity_quantity( *comm_it );
      // When we drag a commodity we want the effect to be that
      // the commodity icon is still drawn (because it is a kind
      // of label for buckets), but we want the quantity to
      // render as zero to reflect the fact that the player has
      // removed those from the colony store.
      if( *comm_it == draggable_.member( &Commodity::type ) )
        label.value = 0;
      render_commodity_annotated(
          tx, *comm_it,
          rect.upper_left() + rendered_commodity_offset(),
          label );
      pos += block_width_;
      comm_it++;
    }
  }

  static unique_ptr<MarketCommodities> create( W block_width ) {
    return make_unique<MarketCommodities>( block_width );
  }

  int quantity_of( e_commodity type ) const {
    return colony().commodity_quantity( type );
  }

  maybe<ColViewObjectWithBounds> object_here(
      Coord const& coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    auto sprite_scale = Scale{ SX{ block_width_._ }, SY{ 32 } };
    auto box_upper_left =
        ( coord / sprite_scale ) * sprite_scale;
    auto idx = ( coord / sprite_scale - Coord{} ).w._;
    UNWRAP_CHECK( type, commodity_from_index( idx ) );
    int quantity = quantity_of( type );
    if( quantity == 0 ) return nothing;
    return ColViewObjectWithBounds{
        .obj    = ColViewObject::commodity{ Commodity{
            .type = type, .quantity = quantity } },
        .bounds = Rect::from(
            box_upper_left + rendered_commodity_offset(),
            Delta{ 1_w, 1_h } * kCommodityTileScale ) };
  }

  bool try_drag( ColViewObject_t const& o ) override {
    UNWRAP_CHECK( [c], o.get_if<ColViewObject::commodity>() );
    CHECK( c.quantity > 0 );
    draggable_ = c;
    return true;
  }

  void cancel_drag() override { draggable_ = nothing; }

  void disown_dragged_object() override {
    CHECK( draggable_ );
    e_commodity type = draggable_->type;
    int         new_quantity =
        quantity_of( type ) - draggable_->quantity;
    CHECK( new_quantity >= 0 );
    colony().set_commodity_quantity( type, new_quantity );
  }

  MarketCommodities( W block_width )
    : block_width_( block_width ) {}

private:
  W                block_width_;
  maybe<Commodity> draggable_;
};

class PopulationView : public ui::View, public ColonySubView {
public:
  Delta delta() const override { return size_; }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::population;
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  void draw( Texture& tx, Coord coord ) const override {
    render_rect( tx, Color::black(),
                 rect( coord ).with_inc_size() );
    auto const& colony = colony_from_id( colony_id() );
    unordered_map<UnitId, ColonyJob_t> const& units_jobs =
        colony.units_jobs();
    auto unit_pos = coord + 16_h;
    for( auto const& [unit_id, job] : units_jobs ) {
      render_unit( tx, unit_id, unit_pos, /*with_icon=*/false );
      unit_pos += 24_w;
    }
  }

  static unique_ptr<PopulationView> create( Delta size ) {
    return make_unique<PopulationView>( size );
  }

  PopulationView( Delta size ) : size_( size ) {}

private:
  Delta size_;
};

class CargoView : public ui::View,
                  public ColonySubView,
                  public IColViewDragSink {
public:
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
    if( c.y > 0_y + g_tile_delta.h ) return nothing;
    int slot_idx =
        ( c / g_tile_scale ).distance_from_origin().w._;
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
        Coord{} + g_tile_delta.w * SX{ slot };
    bool is_open =
        holder_.has_value() &&
        slot < unit_from_id( *holder_ ).desc().cargo_slots;
    return pair{ is_open,
                 Rect::from( slot_upper_left, g_tile_delta ) };
  }

  void draw( Texture& tx, Coord coord ) const override {
    render_rect( tx, Color::black(),
                 rect( coord ).with_inc_size() );
    auto unit = holder_.fmap( unit_from_id );
    for( int idx{ 0 }; idx < max_slots_drawable(); ++idx ) {
      UNWRAP_CHECK( info, slot_rect_from_idx( idx ) );
      auto [is_open, relative_rect] = info;
      Rect rect = relative_rect.as_if_origin_were( coord );
      if( !is_open ) {
        render_fill_rect( tx, Color::wood(), rect );
        continue;
      }
      // if( g_drag_state.has_value() ) {
      //   if_get( g_drag_state->object,
      //           OldWorldDraggableObject::cargo_commodity,
      //           cc ) {
      //     if( cc.slot == slot ) continue;
      //   }
      // }

      // FIXME: need to deduplicate this logic with that in
      // the Old World view.
      render_fill_rect( tx, Color::wood().highlighted( 4 ),
                        rect );
      render_rect( tx, Color::wood(), rect );
      CargoHold const& hold = unit->cargo();
      switch( auto& v = hold[idx]; v.to_enum() ) {
        case CargoSlot::e::empty: break;
        case CargoSlot::e::overflow: break;
        case CargoSlot::e::cargo: {
          auto& cargo = v.get<CargoSlot::cargo>();
          overload_visit(
              cargo.contents,
              [&]( UnitId id ) {
                // if( !g_drag_state ||
                //     g_drag_state->object !=
                //         OldWorldDraggableObject_t{
                //             OldWorldDraggableObject::unit{
                //                 id } } )
                render_unit( tx, id, rect.upper_left(),
                             /*with_icon=*/false );
              },
              [&]( Commodity const& c ) {
                render_commodity_annotated(
                    tx, c,
                    rect.upper_left() +
                        kCommodityInCargoHoldRenderingOffset );
              } );
          break;
        }
      }
    }
  }

  static unique_ptr<CargoView> create( Delta size ) {
    return make_unique<CargoView>( size );
  }

  CargoView( Delta size ) : size_( size ) {}

  void set_unit( maybe<UnitId> unit ) { holder_ = unit; }

  maybe<ColViewObject_t> can_receive(
      ColViewObject_t const& o, e_colview_entity from,
      Coord const& where ) const override {
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

    // We are dragging from another entity.
    if( !holder_ ) return nothing;
    maybe<pair<bool, int>> slot_info =
        slot_idx_from_coord( where );
    if( !slot_info.has_value() ) return nothing;
    auto [is_open, slot_idx] = *slot_info;
    if( !is_open ) return nothing;
    auto& unit = unit_from_id( *holder_ );
    switch( o.to_enum() ) {
      using namespace ColViewObject;
      case e::unit: {
        UnitId id = o.get<ColViewObject::unit>().id;
        if( !unit.cargo().fits_somewhere( id ) ) return nothing;
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

  void drop( ColViewObject_t const& o,
             Coord const&           where ) override {
    CHECK( holder_ );
    auto& cargo_hold = unit_from_id( *holder_ ).cargo();
    Cargo cargo      = to_cargo( o );
    CHECK( cargo_hold.fits_somewhere( cargo ) );
    UNWRAP_CHECK( slot_info, slot_idx_from_coord( where ) );
    auto [is_open, slot_idx] = slot_info;
    overload_visit(
        cargo, //
        [this, slot_idx = slot_idx]( UnitId id ) {
          ustate_change_to_cargo_somewhere(
              *holder_, id, /*starting_slot=*/slot_idx );
        },
        [this, slot_idx = slot_idx]( Commodity const& c ) {
          add_commodity_to_cargo( c, *holder_, slot_idx,
                                  /*try_other_slots=*/true );
        } );
  }

private:
  // FIXME: this gets reset whenever we recomposite. We need to
  // either put this in a global place, or not recreate all of
  // these view objects each time we recomposite (i.e., reuse
  // them).
  maybe<UnitId> holder_;
  Delta         size_;
};

class UnitsAtGateColonyView : public ui::View,
                              public ColonySubView {
public:
  Delta delta() const override { return size_; }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::units_at_gate;
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  void draw( Texture& tx, Coord coord ) const override {
    render_rect( tx, Color::black(),
                 rect( coord ).with_inc_size() );
    for( auto [unit_id, unit_pos] : positioned_units_ ) {
      Coord draw_pos = unit_pos.as_if_origin_were( coord );
      render_unit( tx, unit_id, draw_pos, /*with_icon=*/true );
      if( selected_ == unit_id )
        render_rect( tx, Color::green(),
                     Rect::from( draw_pos, g_tile_delta ) );
    }
  }

  static unique_ptr<UnitsAtGateColonyView> create(
      CargoView* cargo_view, Delta size ) {
    return make_unique<UnitsAtGateColonyView>( cargo_view,
                                               size );
  }

  UnitsAtGateColonyView( CargoView* cargo_view, Delta size )
    : cargo_view_( cargo_view ), size_( size ) {
    update_unit_layout();
  }

  // Implement AwaitableView.
  waitable<> perform_click( Coord pos ) override {
    CHECK( pos.is_inside( rect( {} ) ) );
    for( auto [unit_id, unit_pos] : positioned_units_ ) {
      if( pos.is_inside(
              Rect::from( unit_pos, g_tile_delta ) ) ) {
        co_await click_on_unit( unit_id );
      }
    }
  }

private:
  void set_selected_unit( maybe<UnitId> id ) {
    selected_ = id;
    cargo_view_->set_unit( id );
  }

  waitable<> click_on_unit( UnitId id ) {
    auto& unit = unit_from_id( id );
    if( selected_ != id ) {
      set_selected_unit( id );
      // The first time we select a unit, just select it, but
      // don't pop up the orders menu until the second click.
      // This should make a more polished feel for the UI, and
      // also allow viewing a ship's cargo without popping up the
      // orders menu.
      co_return;
    }
    vector<e_unit_orders> possible_orders;
    if( unit.desc().ship )
      possible_orders = { e_unit_orders::none,
                          e_unit_orders::sentry };
    else
      possible_orders = { e_unit_orders::none,
                          e_unit_orders::sentry,
                          e_unit_orders::fortified };
    e_unit_orders new_orders =
        co_await ui::select_box_enum<e_unit_orders>(
            "Change unit orders to:", possible_orders );
    CHECK( new_orders != e_unit_orders::fortified ||
           !unit.desc().ship );
    switch( new_orders ) {
      case e_unit_orders::none: unit.clear_orders(); break;
      case e_unit_orders::sentry: unit.sentry(); break;
      case e_unit_orders::fortified: unit.fortify(); break;
    }
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

  void update_unit_layout() {
    auto const& colony   = colony_from_id( colony_id() );
    auto const& units    = units_from_coord( colony.location() );
    auto        unit_pos = Coord{} + 16_h;
    positioned_units_.clear();
    maybe<UnitId> first_with_cargo;
    for( auto unit_id : units ) {
      positioned_units_.push_back(
          { .id = unit_id, .pos = unit_pos } );
      unit_pos += 32_w;
      if( !first_with_cargo.has_value() &&
          unit_from_id( unit_id ).desc().cargo_slots > 0 )
        first_with_cargo = unit_id;
    }
    if( selected_.has_value() && !units.contains( *selected_ ) )
      set_selected_unit( nothing );
    if( !selected_.has_value() )
      set_selected_unit( first_with_cargo );
  }

  CargoView* cargo_view_;
  Delta      size_;
};

class ProductionView : public ui::View, public ColonySubView {
public:
  Delta delta() const override { return size_; }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::production;
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  void draw( Texture& tx, Coord coord ) const override {
    render_rect( tx, Color::black(),
                 rect( coord ).with_inc_size() );
  }

  static unique_ptr<ProductionView> create( Delta size ) {
    return make_unique<ProductionView>( size );
  }

  ProductionView( Delta size ) : size_( size ) {}

private:
  Delta size_;
};

class LandView : public ui::View, public ColonySubView {
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
    return Delta{ 32_w, 32_h } * Scale{ side_length_in_squares };
  }

  Delta delta() const override { return size_needed( mode_ ); }

  maybe<e_colview_entity> entity() const override {
    return e_colview_entity::land;
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  void draw_land_3x3( Texture& tx, Coord coord ) const {
    auto const& colony       = colony_from_id( colony_id() );
    Coord       world_square = colony.location();
    for( auto local_coord : Rect{ 0_x, 0_y, 3_w, 3_h } ) {
      auto render_square = world_square +
                           local_coord.distance_from_origin() -
                           Delta{ 1_w, 1_h };
      render_terrain_square( tx, render_square,
                             ( local_coord * g_tile_scale )
                                 .as_if_origin_were( coord ) );
    }
    for( auto local_coord : Rect{ 0_x, 0_y, 3_w, 3_h } ) {
      auto render_square = world_square +
                           local_coord.distance_from_origin() -
                           Delta{ 1_w, 1_h };
      auto maybe_col_id = colony_from_coord( render_square );
      if( !maybe_col_id ) continue;
      render_colony( tx, *maybe_col_id,
                     ( local_coord * g_tile_scale )
                             .as_if_origin_were( coord ) -
                         Delta{ 6_w, 6_h } );
    }
  }

  void draw_land_6x6( Texture& tx, Coord coord ) const {
    land_tx_.fill();
    draw_land_3x3( land_tx_, Coord{} );
    auto dst_rect = Rect::from( coord, delta() );
    land_tx_.copy_to( tx, /*src=*/nothing, dst_rect );
  }

  void draw( Texture& tx, Coord coord ) const override {
    switch( mode_ ) {
      case e_render_mode::_3x3:
        draw_land_3x3( tx, coord );
        break;
      case e_render_mode::_5x5:
        render_fill_rect( tx, Color::wood(), rect( coord ) );
        draw_land_3x3( tx, coord + g_tile_delta );
        break;
      case e_render_mode::_6x6:
        draw_land_6x6( tx, coord );
        break;
    }
  }

  static unique_ptr<LandView> create( e_render_mode mode ) {
    return make_unique<LandView>(
        mode,
        Texture::create( Delta{ 3_w, 3_h } * g_tile_scale ) );
  }

  LandView( e_render_mode mode, Texture land_tx )
    : mode_( mode ), land_tx_( std::move( land_tx ) ) {}

private:
  e_render_mode   mode_;
  mutable Texture land_tx_;
};

/****************************************************************
** Compositing
*****************************************************************/
struct CompositeColSubView : public ui::InvisibleView,
                             public ColonySubView {
  CompositeColSubView(
      Delta size, std::vector<ui::OwningPositionedView> views )
    : ui::InvisibleView( size, std::move( views ) ) {
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

  // Implement AwaitableView.
  waitable<> perform_click( Coord pos ) override {
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedView pos_view = at( i );
      if( !pos.is_inside( pos_view.rect() ) ) continue;
      return ptrs_[i]->perform_click(
          pos.with_new_origin( pos_view.coord ) );
    }
    return make_waitable<>();
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

  vector<ColonySubView*> ptrs_;
};

void recomposite( ColonyId id, Delta screen_size ) {
  CHECK( colony_exists( id ) );
  g_composition.id          = id;
  g_composition.screen_size = screen_size;

  g_composition.top_level = nullptr;
  g_composition.entities.clear();
  vector<ui::OwningPositionedView> views;

  Rect  screen_rect = Rect::from( Coord{}, screen_size );
  Coord pos;
  Delta available;

  // [Title Bar] ------------------------------------------------
  auto title_bar =
      TitleBar::create( Delta{ 10_h, screen_size.w } );
  g_composition.entities[e_colview_entity::title_bar] =
      title_bar.get();
  pos = Coord{};
  Y const title_bar_bottom =
      title_bar->rect( pos ).bottom_edge();
  views.push_back(
      ui::OwningPositionedView( std::move( title_bar ), pos ) );

  // [MarketCommodities] ----------------------------------------
  W comm_block_width = screen_rect.delta().w /
                       SX{ enum_traits<e_commodity>::count };
  comm_block_width =
      std::clamp( comm_block_width, kCommodityTileSize.w, 32_w );
  auto market_commodities =
      MarketCommodities::create( comm_block_width );
  g_composition.entities[e_colview_entity::commodities] =
      market_commodities.get();
  pos = centered_bottom( market_commodities->delta(),
                         screen_rect );
  auto const market_commodities_top = pos.y;
  views.push_back( ui::OwningPositionedView(
      std::move( market_commodities ), pos ) );

  // [Middle Strip] ---------------------------------------------
  Delta   middle_strip_size{ screen_size.w, 32_h + 32_h + 16_h };
  Y const middle_strip_top =
      market_commodities_top - middle_strip_size.h;

  // [Population] -----------------------------------------------
  auto population_view =
      PopulationView::create( middle_strip_size.with_width(
          middle_strip_size.w / 3_sx ) );
  g_composition.entities[e_colview_entity::population] =
      population_view.get();
  pos = Coord{ 0_x, middle_strip_top };
  X const population_right_edge =
      population_view->rect( pos ).right_edge();
  views.push_back( ui::OwningPositionedView(
      std::move( population_view ), pos ) );

  // [Cargo] ----------------------------------------------------
  auto cargo_view = CargoView::create(
      middle_strip_size.with_width( middle_strip_size.w / 3_sx )
          .with_height( 32_h ) );
  g_composition.entities[e_colview_entity::cargo] =
      cargo_view.get();
  pos = Coord{ population_right_edge,
               middle_strip_top + 32_h + 16_h };
  X const cargo_right_edge =
      cargo_view->rect( pos ).right_edge();
  auto* p_cargo_view = cargo_view.get();
  views.push_back(
      ui::OwningPositionedView( std::move( cargo_view ), pos ) );

  // [Units at Gate outside colony] -----------------------------
  auto units_at_gate_view = UnitsAtGateColonyView::create(
      p_cargo_view, middle_strip_size.with_width(
                        middle_strip_size.w / 3_sx * 2_sx ) );
  g_composition.entities[e_colview_entity::units_at_gate] =
      units_at_gate_view.get();
  pos = Coord{ population_right_edge, middle_strip_top };
  views.push_back( ui::OwningPositionedView(
      std::move( units_at_gate_view ), pos ) );

  // [Production] -----------------------------------------------
  auto production_view =
      ProductionView::create( middle_strip_size.with_width(
          middle_strip_size.w / 3_sx ) );
  g_composition.entities[e_colview_entity::production] =
      production_view.get();
  pos = Coord{ cargo_right_edge, middle_strip_top };
  views.push_back( ui::OwningPositionedView(
      std::move( production_view ), pos ) );

  // [LandView] -------------------------------------------------
  available = Delta{ screen_size.w,
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
  auto land_view = LandView::create( land_view_mode );
  g_composition.entities[e_colview_entity::land] =
      land_view.get();
  pos = g_composition.entities[e_colview_entity::title_bar]
            ->view()
            .rect( Coord{} )
            .lower_right() -
        land_view->delta().w;
  views.push_back(
      ui::OwningPositionedView( std::move( land_view ), pos ) );

  // [Finish] ---------------------------------------------------
  auto invisible_view = std::make_unique<CompositeColSubView>(
      screen_rect.delta(), std::move( views ) );
  invisible_view->set_delta( screen_rect.delta() );
  g_composition.top_level = std::move( invisible_view );

  for( auto e : enum_traits<e_colview_entity>::values ) {
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
    drag::State<ColViewObject_t> const& state, Texture& tx ) {
  Coord sprite_upper_left = state.where - state.click_offset;
  using namespace ColViewObject;
  // Render the dragged item.
  overload_visit(
      state.object,
      [&]( unit const& o ) {
        render_unit( tx, o.id, sprite_upper_left,
                     /*with_icon=*/false );
      },
      [&]( commodity const& o ) {
        render_commodity( tx, o.comm.type, sprite_upper_left );
      } );
  // Render any indicators on top of it.
  switch( state.indicator ) {
    using e = drag::e_status_indicator;
    case e::none: break;
    case e::bad: {
      auto const& status_tx = render_text( "X", Color::red() );
      copy_texture( status_tx, tx, sprite_upper_left );
      break;
    }
    case e::good: {
      auto const& status_tx = render_text( "+", Color::green() );
      copy_texture( status_tx, tx, sprite_upper_left );
      if( state.user_requests_input ) {
        auto const& mod_tx  = render_text( "?", Color::green() );
        auto        mod_pos = state.where;
        mod_pos.y -= mod_tx.size().h;
        copy_texture( mod_tx, tx, mod_pos - state.click_offset );
      }
      break;
    }
  }
}
void set_colview_colony( ColonyId id ) {
  auto new_id   = id;
  auto new_size = main_window_logical_size();
  recomposite( new_id, new_size );
}

} // namespace rn
