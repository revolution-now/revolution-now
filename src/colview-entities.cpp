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
** Globals
*****************************************************************/
struct ColViewComposited {
  ColonyId                                      id;
  UPtr<ui::View>                                top_level;
  FlatMap<e_colview_entity, ColViewEntityView*> entities;
};

ColViewComposited g_composition;

/****************************************************************
** Entities
*****************************************************************/
class MarketCommodities : public ColViewEntityView {
public:
  static constexpr W single_layer_blocks_width  = 16_w;
  static constexpr H single_layer_blocks_height = 1_h;

  // Commodities will be 24x24 + 8 pixels for text.
  static constexpr auto sprite_scale = Scale{ 32 };

  static constexpr W single_layer_width =
      single_layer_blocks_width * sprite_scale.sx;
  static constexpr H single_layer_height =
      single_layer_blocks_height * sprite_scale.sy;

  static constexpr Delta k_rendered_commodity_offset{ 8_w, 3_h };

public:
  Delta delta() const override {
    return Delta{ single_layer_width, single_layer_height };
  }

  void draw( Texture& tx, Coord coord ) const override {
    auto        comm_it = values<e_commodity>.begin();
    auto        label   = CommodityLabel::quantity{ 0 };
    Coord       pos     = coord;
    auto const& colony  = colony_from_id( id_ );
    for( int i = 0; i < 16; ++i ) {
      auto rect = Rect::from( pos, Delta{ 32_w, 32_h } );
      render_rect( tx, Color::black(), rect );
      label.value = colony.commodity_quantity( *comm_it );
      render_commodity_annotated(
          tx, *comm_it++,
          rect.upper_left() + k_rendered_commodity_offset,
          label );
      pos += 1_w * sprite_scale.sx;
    }
  }

  static Delta size_needed() {
    return Delta{ single_layer_width, single_layer_height };
  }

  static UPtr<MarketCommodities> create( ColonyId id ) {
    return make_unique<MarketCommodities>( id );
  }

  e_colview_entity entity_id() const override {
    return e_colview_entity::commodities;
  }

  Opt<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nullopt;
    auto box_upper_left =
        ( coord / sprite_scale ) * sprite_scale;
    auto idx        = ( coord / sprite_scale - Coord{} ).w._;
    auto maybe_type = commodity_from_index( idx );
    if( !maybe_type ) return nullopt;
    return ColViewObjectUnderCursor{
        .obj =
            ColViewDraggableObject::market_commodity{
                .type = *maybe_type },
        .bounds = Rect::from( box_upper_left +
                                  k_rendered_commodity_offset,
                              Delta{ 1_w, 1_h } * Scale{ 16 } )
                      .as_if_origin_were( coord ) };
  }

private:
  using ColViewEntityView::ColViewEntityView;
};

/****************************************************************
** Compositing
*****************************************************************/
void recomposite( ColonyId id ) {
  CHECK( g_composition.id != id );
  CHECK( colony_exists( id ) );
  g_composition.id = id;

  g_composition.top_level = nullptr;
  g_composition.entities.clear();
  vector<ui::OwningPositionedView> views;

  Rect  screen_rect = main_window_logical_rect();
  Coord pos;

  auto market_commodities = MarketCommodities::create( id );
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
  if( id == g_composition.id ) return;
  recomposite( id );
}

} // namespace rn
