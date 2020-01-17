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
#include "screen.hpp"
#include "views.hpp"

// magic-enum
#include "magic_enum.hpp"

using namespace std;

namespace rn {

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
  ColonyId                                      id;
  Delta                                         screen_size;
  UPtr<ui::View>                                top_level;
  FlatMap<e_colview_entity, ColViewEntityView*> entities;
};

ColViewComposited g_composition;

/****************************************************************
** Entities
*****************************************************************/
class MarketCommodities : public ColViewEntityView {
public:
  Delta delta() const override {
    return Delta{
        block_width_ * SX{ values<e_commodity>.size() },
        1_h * 32_sy };
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
    auto        comm_it = values<e_commodity>.begin();
    auto        label   = CommodityLabel::quantity{ 0 };
    Coord       pos     = coord;
    auto const& colony  = colony_from_id( id_ );
    for( int i = 0; i < kNumCommodityTypes; ++i ) {
      auto rect = Rect::from( pos, Delta{ 32_h, block_width_ } );
      render_rect( tx, Color::black(), rect );
      label.value = colony.commodity_quantity( *comm_it );
      render_commodity_annotated(
          tx, *comm_it++,
          rect.upper_left() + rendered_commodity_offset(),
          label );
      pos += block_width_;
    }
  }

  static UPtr<MarketCommodities> create( ColonyId id,
                                         W        block_width ) {
    return make_unique<MarketCommodities>( id, block_width );
  }

  e_colview_entity entity_id() const override {
    return e_colview_entity::commodities;
  }

  Opt<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nullopt;
    auto sprite_scale = Scale{ SX{ block_width_._ }, SY{ 32 } };
    auto box_upper_left =
        ( coord / sprite_scale ) * sprite_scale;
    auto idx        = ( coord / sprite_scale - Coord{} ).w._;
    auto maybe_type = commodity_from_index( idx );
    if( !maybe_type ) return nullopt;
    return ColViewObjectUnderCursor{
        .obj =
            ColViewDraggableObject::market_commodity{
                .type = *maybe_type },
        .bounds =
            Rect::from(
                box_upper_left + rendered_commodity_offset(),
                Delta{ 1_w, 1_h } * kCommodityTileScale )
                .as_if_origin_were( coord ) };
  }

  MarketCommodities( ColonyId id, W block_width )
    : ColViewEntityView( id ), block_width_( block_width ) {}

private:
  W block_width_;
};

/****************************************************************
** Compositing
*****************************************************************/
void recomposite( ColonyId id, Delta screen_size ) {
  CHECK( colony_exists( id ) );
  g_composition.id          = id;
  g_composition.screen_size = screen_size;

  g_composition.top_level = nullptr;
  g_composition.entities.clear();
  vector<ui::OwningPositionedView> views;

  Rect  screen_rect = Rect::from( Coord{}, screen_size );
  Coord pos;

  W comm_block_width =
      screen_rect.delta().w / SX{ values<e_commodity>.size() };
  comm_block_width =
      std::clamp( comm_block_width, kCommodityTileSize.w, 32_w );
  auto market_commodities =
      MarketCommodities::create( id, comm_block_width );
  pos = centered_bottom( market_commodities->delta(),
                         screen_rect );
  views.push_back( ui::OwningPositionedView(
      std::move( market_commodities ), pos ) );

  auto invisible_view = std::make_unique<ui::InvisibleView>(
      screen_rect.delta(), std::move( views ) );
  invisible_view->set_delta( screen_rect.delta() );
  g_composition.top_level = std::move( invisible_view );

  // for( auto e : magic_enum::enum_values<e_colview_entity>() )
  // {
  //  CHECK( g_composition.entities.contains( e ),
  //         "colview entity {} is missing.", e );
  //}
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ColViewEntityView const* colview_entity(
    e_colview_entity entity ) {
  (void)entity;
  return nullptr;
}

ui::View const* colview_top_level() {
  return g_composition.top_level.get();
}

void set_colview_colony( ColonyId id ) {
  auto old_id   = g_composition.id;
  auto old_size = g_composition.screen_size;
  auto new_id   = id;
  auto new_size = main_window_logical_size();
  if( new_id == old_id && new_size == old_size ) return;
  recomposite( new_id, new_size );
}

} // namespace rn
