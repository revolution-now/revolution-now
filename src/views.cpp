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
#include "text.hpp"

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

bool CompositeView::input( input::event_t const& event ) {
  for( auto p_view : *this )
    if( p_view.view->input( event ) ) return true;
  return false;
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

bool OptionSelectView::input( input::event_t const& event ) {
  bool handled = false;
  switch_v( event ) {
    case_v( input::key_event_t ) {
      auto const& key_event = val;
      if( key_event.change != input::e_key_change::down )
        break_v;
      // It's a key down.
      switch( key_event.keycode ) {
        case ::SDLK_UP:
        case ::SDLK_KP_8:
          if( selected_ > 0 ) set_selected( selected_ - 1 );
          handled = true;
          break;
        case ::SDLK_DOWN:
        case ::SDLK_KP_2:
          if( selected_ < count() - 1 )
            set_selected( selected_ + 1 );
          handled = true;
          break;
        case ::SDLK_RETURN:
        case ::SDLK_KP_ENTER:
          has_confirmed = true;
          handled       = true;
          break;
        default: break;
      }
    }
    default_v_no_check;
  };
  return handled;
}

string const& OptionSelectView::get_selected() const {
  return get_view( selected_ )->line();
}

} // namespace rn::ui
