/****************************************************************
**views.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-16.
*
* Description: Views for populating windows in the UI.
*
*****************************************************************/
#include "views.hpp"

// Revolution Now
#include "config-files.hpp"
#include "fonts.hpp"
#include "geo-types.hpp"
#include "logging.hpp"
#include "text.hpp"
#include "variant.hpp"

// base-util
#include "base-util/misc.hpp"

// C++ standard library
#include <numeric>

using namespace std;

namespace rn::ui {

namespace {} // namespace

/****************************************************************
** Views
*****************************************************************/
void CompositeView::draw( Texture const& tx,
                          Coord          coord ) const {
  // Draw each of the sub views, by augmenting its origin (which
  // is relative to the origin of the parent by the origin that
  // we have been given.
  for( auto [view, view_coord] : *this )
    view->draw( tx, coord + ( view_coord - Coord() ) );
}

Delta CompositeView::delta() const {
  auto uni0n = L2( _1.uni0n( _2.view->rect( _2.coord ) ) );
  auto rect  = accumulate( begin(), end(), Rect{}, uni0n );
  return {rect.w, rect.h};
}

bool CompositeView::dispatch_mouse_event(
    input::event_t const& event ) {
  auto maybe_pos = input::mouse_position( event );
  CHECK( maybe_pos.has_value() );
  for( auto p_view : *this ) {
    if( maybe_pos.value().get().is_inside( p_view.rect() ) ) {
      auto new_event =
          move_mouse_origin_by( event, p_view.coord - Coord{} );
      if( p_view.view->input( new_event ) ) //
        return true;
    }
  }
  return false;
}

bool CompositeView::on_key( input::key_event_t const& event ) {
  for( auto p_view : *this )
    if( p_view.view->input( event ) ) return true;
  return false;
}

bool CompositeView::on_wheel(
    input::mouse_wheel_event_t const& event ) {
  return dispatch_mouse_event( event );
}

bool CompositeView::on_mouse_move(
    input::mouse_move_event_t const& event ) {
  return dispatch_mouse_event( event );
}

bool CompositeView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  return dispatch_mouse_event( event );
}

void CompositeView::children_under_coord( Coord      where,
                                          ObjectSet& objects ) {
  for( auto p_view : *this ) {
    if( where.is_inside( p_view.rect() ) ) {
      objects.insert( p_view.view.get() );
      p_view.view->children_under_coord(
          where.with_new_origin( p_view.rect().upper_left() ),
          objects );
    }
  }
}

PositionedView CompositeView::at( int idx ) {
  auto p_view =
      static_cast<CompositeView const*>( this )->at_const( idx );
  ObserverPtr<View> view{const_cast<View*>( p_view.view.get() )};
  return {view, p_view.coord};
}

PositionedViewConst ViewVector::at_const( int idx ) const {
  CHECK( idx >= 0 && idx < int( views_.size() ) );
  auto& view = views_[idx];
  return {view.view(), view.coord()};
}

/****************************************************************
** Primitive Views
*****************************************************************/
void SolidRectView::draw( Texture const& tx,
                          Coord          coord ) const {
  render_fill_rect( tx, color_, rect( coord ) );
}

OneLineStringView::OneLineStringView( string msg, Color color,
                                      bool shadow )
  : msg_( move( msg ) ) {
  if( shadow )
    tx_ = render_text_line_solid( fonts::standard, color, msg_ );
  else
    tx_ = render_text_line_solid( fonts::standard, color, msg_ );
}

void OneLineStringView::draw( Texture const& tx,
                              Coord          coord ) const {
  copy_texture( this->tx_, tx, coord );
}

ButtonBaseView::ButtonBaseView( string label ) {
  auto info = TextMarkupInfo{}; // Should be irrelevant
  auto size = render_text_markup( fonts::standard, info, label )
                  .size()
                  .round_up( Scale{8} );
  auto size_in_blocks = size / Scale{8};
  size_in_blocks.w += 2_w;
  render( label, size_in_blocks );
}

ButtonBaseView::ButtonBaseView( string label,
                                Delta  size_in_blocks ) {
  render( label, size_in_blocks );
}

void ButtonBaseView::draw( Texture const& tx,
                           Coord          coord ) const {
  auto do_copy = [&]( auto const& src ) {
    copy_texture( src, tx, coord );
  };
  switch( state_ ) {
    case button_state::disabled: do_copy( disabled_ ); return;
    case button_state::down: do_copy( pressed_ ); return;
    case button_state::up: do_copy( unpressed_ ); return;
    case button_state::hover: do_copy( hover_ ); return;
  }
  SHOULD_NOT_BE_HERE;
}

void ButtonBaseView::render( string const& label,
                             Delta         size_in_blocks ) {
  auto pixel_size = size_in_blocks * Scale{8};
  pressed_        = create_texture_transparent( pixel_size );
  hover_          = create_texture_transparent( pixel_size );
  unpressed_      = create_texture_transparent( pixel_size );
  disabled_       = create_texture_transparent( pixel_size );

  render_rect_of_sprites_with_border(
      unpressed_, Coord{}, size_in_blocks, //
      g_tile::button_up_mm, g_tile::button_up_um,
      g_tile::button_up_lm, g_tile::button_up_ml,
      g_tile::button_up_mr, g_tile::button_up_ul,
      g_tile::button_up_ur, g_tile::button_up_ll,
      g_tile::button_up_lr );

  render_rect_of_sprites_with_border(
      hover_, Coord{}, size_in_blocks, //
      g_tile::button_up_mm, g_tile::button_up_um,
      g_tile::button_up_lm, g_tile::button_up_ml,
      g_tile::button_up_mr, g_tile::button_up_ul,
      g_tile::button_up_ur, g_tile::button_up_ll,
      g_tile::button_up_lr );

  render_rect_of_sprites_with_border(
      disabled_, Coord{}, size_in_blocks, //
      g_tile::button_up_mm, g_tile::button_up_um,
      g_tile::button_up_lm, g_tile::button_up_ml,
      g_tile::button_up_mr, g_tile::button_up_ul,
      g_tile::button_up_ur, g_tile::button_up_ll,
      g_tile::button_up_lr );

  render_rect_of_sprites_with_border(
      pressed_, Coord{}, size_in_blocks, //
      g_tile::button_down_mm, g_tile::button_down_um,
      g_tile::button_down_lm, g_tile::button_down_ml,
      g_tile::button_down_mr, g_tile::button_down_ul,
      g_tile::button_down_ur, g_tile::button_down_ll,
      g_tile::button_down_lr );

  auto info_normal = TextMarkupInfo{Color::wood().shaded( 3 ),
                                    /*highlight=*/{}};
  auto info_hover =
      TextMarkupInfo{Color::banana(), /*highlight=*/{}};
  auto info_pressed =
      TextMarkupInfo{Color::banana(), /*highlight=*/{}};
  auto info_disabled = TextMarkupInfo{config_palette.grey.n50,
                                      /*highlight=*/{}};

  auto tx_normal =
      render_text_markup( fonts::standard, info_normal, label );
  auto tx_pressed =
      render_text_markup( fonts::standard, info_pressed, label );
  auto tx_hover =
      render_text_markup( fonts::standard, info_hover, label );
  auto tx_disabled = render_text_markup( fonts::standard,
                                         info_disabled, label );

  auto unpressed_coord =
      centered( tx_normal.size(), unpressed_.rect() );
  auto pressed_coord = unpressed_coord + Delta{-1_w, 1_h};

  copy_texture( tx_normal, unpressed_, unpressed_coord );
  copy_texture( tx_hover, hover_, unpressed_coord );
  copy_texture( tx_pressed, pressed_, pressed_coord );
  copy_texture( tx_disabled, disabled_, unpressed_coord );
}

ButtonView::ButtonView( string label )
  : ButtonBaseView( std::move( label ) ) {
  set_state( button_state::up );
}

ButtonView::ButtonView( string label, Delta size_in_blocks )
  : ButtonBaseView( std::move( label ), size_in_blocks ) {
  set_state( button_state::up );
}

bool ButtonView::on_mouse_move(
    input::mouse_move_event_t const& /*unused*/ ) {
  switch( state() ) {
    case button_state::down: break;
    case button_state::up:
      set_state( button_state::hover );
      break;
    case button_state::disabled: break;
    case button_state::hover: break;
  }
  return true;
}

bool ButtonView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  switch( event.buttons ) {
    case input::e_mouse_button_event::left_down:
      set_state( button_state::down );
      break;
    case input::e_mouse_button_event::left_up:
      set_state( button_state::hover );
      break;
    default: break;
  }
  return false;
}

void ButtonView::on_mouse_leave() {
  set_state( button_state::up );
}

/****************************************************************
** Derived Views
*****************************************************************/
OptionSelectItemView::OptionSelectItemView( string msg )
  : active_{e_option_active::inactive},
    background_active_{config_palette.yellow.sat1.lum11},
    background_inactive_{config_palette.orange.sat0.lum3},
    foreground_active_( msg, config_palette.orange.sat0.lum2,
                        /*shadow=*/true ),
    foreground_inactive_( msg, config_palette.orange.sat1.lum11,
                          /*shadow=*/true ) {
  auto delta_active   = foreground_active_.delta();
  auto delta_inactive = foreground_inactive_.delta();
  background_active_.set_delta( delta_active );
  background_inactive_.set_delta( delta_inactive );
}

PositionedViewConst OptionSelectItemView::at_const(
    int idx ) const {
  CHECK( idx == 0 || idx == 1 );
  Coord       coord{};
  View const* view{};
  switch( idx ) {
    case 0:
      switch( active_ ) {
        case e_option_active::active:
          view = &background_active_;
          break;
        case e_option_active::inactive:
          view = &background_inactive_;
          break;
      }
      break;
    case 1:
      switch( active_ ) {
        case e_option_active::active:
          view = &foreground_active_;
          break;
        case e_option_active::inactive:
          view = &foreground_inactive_;
          break;
      }
  }
  return {ObserverCPtr<View>{view}, coord};
}

void OptionSelectItemView::grow_to( W w ) {
  auto new_delta = foreground_active_.delta();
  if( new_delta.w > w )
    // we only grow here, not shrink.
    return;
  new_delta.w = w;
  background_active_.set_delta( new_delta );
  background_inactive_.set_delta( new_delta );
}

OptionSelectView::OptionSelectView( Vec<Str> const& options,
                                    int initial_selection )
  : selected_{initial_selection}, has_confirmed{false} {
  CHECK( options.size() > 0 );
  CHECK( selected_ >= 0 && selected_ < int( options.size() ) );

  Coord so_far{};
  W     min_width{0};
  for( auto const& option : options ) {
    auto view   = make_unique<OptionSelectItemView>( option );
    auto width  = view->delta().w;
    auto height = view->delta().h;
    this->push_back(
        OwningPositionedView( move( view ), so_far ) );
    // `view` is no longer available here (moved from).
    so_far.y += height;
    min_width = std::max( min_width, width );
  }

  grow_to( min_width );

  // Now that we have the individual options populated we can
  // officially set a selected one.
  set_selected( selected_ );
}

ObserverPtr<OptionSelectItemView> OptionSelectView::get_view(
    int item ) {
  CHECK( item >= 0 && item < count(),
         "item '{}' is out of bounds", item );
  auto* view    = at( item ).view.get();
  auto* o_s_i_v = view->cast<OptionSelectItemView>();
  return ObserverPtr<OptionSelectItemView>{o_s_i_v};
}

// TODO: duplication
ObserverCPtr<OptionSelectItemView> OptionSelectView::get_view(
    int item ) const {
  CHECK( item >= 0 && item < count(),
         "item '{}' is out of bounds", item );
  auto* view    = at_const( item ).view.get();
  auto* o_s_i_v = view->cast<OptionSelectItemView>();
  return ObserverCPtr<OptionSelectItemView>{o_s_i_v};
}

void OptionSelectView::set_selected( int item ) {
  get_view( selected_ )->set_active( e_option_active::inactive );
  get_view( item )->set_active( e_option_active::active );
  selected_ = item;
}

void OptionSelectView::grow_to( W w ) {
  for( auto p_view : *this ) {
    auto* view    = p_view.view.get();
    auto* o_s_i_v = view->cast<OptionSelectItemView>();
    o_s_i_v->grow_to( w );
  }
}

bool OptionSelectView::on_key(
    input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down ) return false;
  // It's a key down.
  switch( event.keycode ) {
    case ::SDLK_UP:
    case ::SDLK_KP_8:
      if( selected_ > 0 ) set_selected( selected_ - 1 );
      return true;
      break;
    case ::SDLK_DOWN:
    case ::SDLK_KP_2:
      if( selected_ < count() - 1 )
        set_selected( selected_ + 1 );
      return true;
      break;
    case ::SDLK_RETURN:
    case ::SDLK_KP_ENTER:
      has_confirmed = true;
      return true;
      break;
    default: break;
  }
  return false;
}

string const& OptionSelectView::get_selected() const {
  return get_view( selected_ )->line();
}

} // namespace rn::ui
