/****************************************************************
**unit-flag.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-29.
*
* Description: Some logic related to rendering the flag
*              next to a unit.
*
*****************************************************************/
#include "unit-flag.hpp"

// Revolution Now
#include "unit-mgr.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/nation.hpp"
#include "config/natives.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/unit.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

// This includes the border.
constexpr gfx::size kFlagIconSize{ .w = 14, .h = 14 };

constexpr int kStackedFlagSeparation = 2;

gfx::pixel background_color_for_nation( e_nation nation ) {
  return nation_obj( nation ).flag_color;
}

gfx::pixel char_color_for_orders( unit_orders const& orders ) {
  bool is_greyed = false;
  SWITCH( orders ) {
    CASE( none ) { break; }
    CASE( sentry ) {
      is_greyed = true;
      break;
    }
    CASE( fortified ) {
      is_greyed = true;
      break;
    }
    CASE( fortifying ) { break; }
    CASE( road ) { break; }
    CASE( plow ) { break; }
    CASE( damaged ) { break; }
  }
  return is_greyed
             ? config_gfx.unit_flag_colors
                   .unit_flag_text_color_greyed
             : config_gfx.unit_flag_colors.unit_flag_text_color;
}

char char_value_for_orders( unit_orders const& orders ) {
  SWITCH( orders ) {
    CASE( none ) { return '-'; }
    CASE( sentry ) { return 'S'; }
    CASE( fortified ) { return 'F'; }
    CASE( fortifying ) { return 'F'; }
    CASE( road ) { return 'R'; }
    CASE( plow ) { return 'P'; }
    CASE( damaged ) {
      int const turns_left = damaged.turns_until_repair;
      // The number can be larger than 9, i.e. it can have more
      // than one digit which we cannot display on the flag. So
      // we will do what the OG does and display a + sign in that
      // case.
      return ( turns_left > 9 ) ? '+' : ( '0' + turns_left );
    }
  }
}

UnitFlagContents flag_char_info_from_orders(
    unit_orders const& orders ) {
  return UnitFlagContents::character{
      .value = char_value_for_orders( orders ),
      .color = char_color_for_orders( orders ),
  };
}

UnitFlagContents flag_char_info_for_privateer() {
  return UnitFlagContents::icon{ .tile = e_tile::privateer_x };
}

UnitFlagContents flag_char_info_for_strategy() {
  // TODO
  return UnitFlagContents::character{
      .value = '?',
      .color =
          config_gfx.unit_flag_colors.unit_flag_text_color };
}

gfx::pixel outline_color_from_background_color(
    gfx::pixel background_color ) {
  static unordered_map<gfx::pixel, gfx::pixel> shaded_cache;
  if( !shaded_cache.contains( background_color ) )
    shaded_cache[background_color] =
        background_color.shaded( 2 );
  return shaded_cache[background_color];
}

UnitFlagOffsets offsets_from_unit_type( auto unit_type ) {
  UnitFlagOffsets res;
  // Now we will advance the pixel_coord to put the icon at the
  // location specified in the unit descriptor.
  e_direction const position =
      unit_attr( unit_type ).nat_icon_position;
  static int const S = kStackedFlagSeparation;
  switch( position ) {
    case e_direction::nw:
      res.offset_stacked = { .w = S, .h = S };
      break;
    case e_direction::ne:
      res.offset_first.w = ( 32 - kFlagIconSize.w );
      res.offset_stacked = { .w = -S, .h = S };
      break;
    case e_direction::se:
      res.offset_first.w = ( 32 - kFlagIconSize.w );
      res.offset_first.h = ( 32 - kFlagIconSize.h );
      res.offset_stacked = { .w = -S, .h = -S };
      break;
    case e_direction::sw:
      res.offset_first.h = ( 32 - kFlagIconSize.h );
      res.offset_stacked = { .w = S, .h = -S };
      break;
    case e_direction::w:
      res.offset_first.h = ( 32 - kFlagIconSize.h ) / 2;
      res.offset_stacked = { .w = S, .h = -S };
      break;
    case e_direction::n:
      res.offset_first.w = ( 32 - kFlagIconSize.w ) / 2;
      res.offset_stacked = { .w = 0, .h = S };
      break;
    case e_direction::e:
      res.offset_first.h = ( 32 - kFlagIconSize.h ) / 2;
      res.offset_first.w = ( 32 - kFlagIconSize.w );
      res.offset_stacked = { .w = -S, .h = -S };
      break;
    case e_direction::s:
      res.offset_first.w = ( 32 - kFlagIconSize.w ) / 2;
      res.offset_first.h = ( 32 - kFlagIconSize.h );
      res.offset_stacked = { .w = 0, .h = -S };
      break;
  };
  res.offset_stacked += res.offset_first;
  return res;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
UnitFlagOptions&& UnitFlagOptions::with_flag_count(
    e_flag_count flag_count ) && {
  this->flag_count = flag_count;
  return (UnitFlagOptions&&)*this;
};

UnitFlagRenderInfo euro_unit_type_flag_info(
    e_unit_type unit_type, unit_orders const& orders,
    e_nation nation ) {
  gfx::pixel const background_color =
      background_color_for_nation( nation );
  UnitFlagContents const flag_contents =
      flag_char_info_from_orders( orders );
  gfx::pixel const outline_color =
      outline_color_from_background_color( background_color );
  UnitFlagOffsets const offsets =
      offsets_from_unit_type( unit_type );
  bool const in_front = unit_attr( unit_type ).nat_icon_front;
  bool const stacked  = false;
  return UnitFlagRenderInfo{
      .stacked          = stacked,
      .size             = kFlagIconSize,
      .offsets          = offsets,
      .outline_color    = outline_color,
      .background_color = background_color,
      .contents         = flag_contents,
      .in_front         = in_front };
}

UnitFlagRenderInfo euro_unit_flag_render_info(
    Unit const& unit, maybe<e_nation> viewer,
    UnitFlagOptions const& options ) {
  bool const privateer_X =
      unit.type() == e_unit_type::privateer &&
      viewer.has_value() && *viewer != unit.nation();
  gfx::pixel const background_color =
      privateer_X
          ? config_gfx.unit_flag_colors.privateer_flag_color
          : background_color_for_nation( unit.nation() );
  UnitFlagContents const flag_contents = [&] {
    switch( options.type ) {
      case e_flag_char_type::normal:
        return privateer_X
                   ? flag_char_info_for_privateer()
                   : flag_char_info_from_orders( unit.orders() );
      case e_flag_char_type::strategy:
        return flag_char_info_for_strategy();
    }
  }();
  gfx::pixel const outline_color =
      outline_color_from_background_color( background_color );
  UnitFlagOffsets const offsets =
      offsets_from_unit_type( unit.type() );
  bool const in_front = unit_attr( unit.type() ).nat_icon_front;
  bool const stacked =
      ( options.flag_count == e_flag_count::multiple );
  return UnitFlagRenderInfo{
      .stacked          = stacked,
      .size             = kFlagIconSize,
      .offsets          = offsets,
      .outline_color    = outline_color,
      .background_color = background_color,
      .contents         = flag_contents,
      .in_front         = in_front };
}

UnitFlagRenderInfo native_unit_flag_render_info(
    SSConst const& ss, NativeUnit const& unit,
    UnitFlagOptions const& options ) {
  e_tribe const    tribe_type = tribe_type_for_unit( ss, unit );
  gfx::pixel const background_color =
      config_natives.tribes[tribe_type].flag_color;
  UnitFlagContents const flag_contents = [&] {
    switch( options.type ) {
      case e_flag_char_type::normal:
        return flag_char_info_from_orders( unit_orders::none{} );
      case e_flag_char_type::strategy:
        return flag_char_info_for_strategy();
    }
  }();
  gfx::pixel const outline_color =
      outline_color_from_background_color( background_color );
  UnitFlagOffsets const offsets =
      offsets_from_unit_type( unit.type );
  bool const in_front = unit_attr( unit.type ).nat_icon_front;
  bool const stacked =
      ( options.flag_count == e_flag_count::multiple );
  return UnitFlagRenderInfo{
      .stacked          = stacked,
      .size             = kFlagIconSize,
      .offsets          = offsets,
      .outline_color    = outline_color,
      .background_color = background_color,
      .contents         = flag_contents,
      .in_front         = in_front };
}

UnitFlagRenderInfo native_unit_type_flag_info(
    e_native_unit_type unit_type, e_tribe tribe_type,
    UnitFlagOptions const& options ) {
  gfx::pixel const background_color =
      config_natives.tribes[tribe_type].flag_color;
  UnitFlagContents const flag_contents = [&] {
    switch( options.type ) {
      case e_flag_char_type::normal:
        return flag_char_info_from_orders( unit_orders::none{} );
      case e_flag_char_type::strategy:
        return flag_char_info_for_strategy();
    }
  }();
  gfx::pixel const outline_color =
      outline_color_from_background_color( background_color );
  UnitFlagOffsets const offsets =
      offsets_from_unit_type( unit_type );
  bool const in_front = unit_attr( unit_type ).nat_icon_front;
  bool const stacked =
      ( options.flag_count == e_flag_count::multiple );
  return UnitFlagRenderInfo{
      .stacked          = stacked,
      .size             = kFlagIconSize,
      .offsets          = offsets,
      .outline_color    = outline_color,
      .background_color = background_color,
      .contents         = flag_contents,
      .in_front         = in_front };
}

} // namespace rn
