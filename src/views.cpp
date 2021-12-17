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
#include "coord.hpp"
#include "gfx.hpp"
#include "logger.hpp"
#include "render.hpp"
#include "text.hpp"
#include "util.hpp"

// Revolution Now (config)
#include "../config/rcl/palette.inl"
#include "../config/rcl/ui.inl"

// base
#include "base/lambda.hpp"

// C++ standard library
#include <chrono>
#include <numeric>

using namespace std;

namespace rn::ui {

namespace {} // namespace

/****************************************************************
** Fundamental Views
*****************************************************************/
void CompositeView::draw( Texture& tx, Coord coord ) const {
  // Draw each of the sub views, by augmenting its origin (which
  // is relative to the origin of the parent by the origin that
  // we have been given.
  for( auto [view, view_coord] : *this )
    view->draw( tx, coord + ( view_coord - Coord() ) );
}

Delta CompositeView::delta() const {
  auto uni0n = L2( _1.uni0n( _2.view->rect( _2.coord ) ) );
  auto rect  = accumulate( begin(), end(), Rect{}, uni0n );
  return { rect.w, rect.h };
}

bool CompositeView::dispatch_mouse_event(
    input::event_t const& event ) {
  UNWRAP_CHECK( pos, input::mouse_position( event ) );
  for( auto p_view : *this ) {
    if( pos.is_inside( p_view.rect() ) ) {
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
  auto to   = event.pos;
  auto from = event.prev;
  // First check for and send out any on_mouse_leave or
  // on_mouse_enter events that happen among the sub-views.
  for( auto p_view : *this ) {
    if( from.is_inside( p_view.rect() ) &&
        !to.is_inside( p_view.rect() ) )
      p_view.view->on_mouse_leave(
          from.with_new_origin( p_view.rect().upper_left() ) );
    if( !from.is_inside( p_view.rect() ) &&
        to.is_inside( p_view.rect() ) )
      p_view.view->on_mouse_enter(
          from.with_new_origin( p_view.rect().upper_left() ) );
  }

  return dispatch_mouse_event( event );
}

bool CompositeView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  return dispatch_mouse_event( event );
}

void CompositeView::on_mouse_leave( Coord from ) {
  for( auto p_view : *this )
    if( from.is_inside( p_view.rect() ) )
      p_view.view->on_mouse_leave(
          from.with_new_origin( p_view.rect().upper_left() ) );
}

void CompositeView::on_mouse_enter( Coord to ) {
  for( auto p_view : *this )
    if( to.is_inside( p_view.rect() ) )
      p_view.view->on_mouse_enter(
          to.with_new_origin( p_view.rect().upper_left() ) );
}

PositionedView CompositeView::at( int idx ) {
  View* view{ const_cast<View*>( mutable_at( idx ).get() ) };
  return { view, pos_of( idx ) };
}

PositionedViewConst CompositeView::at( int idx ) const {
  auto*       this_ = const_cast<CompositeView*>( this );
  View const* view{ this_->mutable_at( idx ).get() };
  return { view, pos_of( idx ) };
}

CompositeSingleView::CompositeSingleView( unique_ptr<View> view,
                                          Coord coord )
  : view_( std::move( view ) ), coord_( coord ) {}

Coord CompositeSingleView::pos_of( int idx ) const {
  CHECK( idx == 0 );
  return coord_;
}

unique_ptr<View>& CompositeSingleView::mutable_at( int idx ) {
  CHECK( idx == 0 );
  return view_;
}

Coord VectorView::pos_of( int idx ) const {
  CHECK( idx >= 0 && idx < int( views_.size() ) );
  return views_[idx].coord();
}

unique_ptr<View>& VectorView::mutable_at( int idx ) {
  CHECK( idx >= 0 && idx < int( views_.size() ) );
  return views_[idx].mutable_view();
}

/****************************************************************
** Simple Views
*****************************************************************/
void SolidRectView::draw( Texture& tx, Coord coord ) const {
  render_fill_rect( tx, color_, rect( coord ) );
}

OneLineStringView::OneLineStringView( string     msg,
                                      gfx::pixel color,
                                      bool       shadow )
  : msg_( move( msg ) ) {
  if( shadow )
    tx_ = clone_texture(
        render_text( font::standard(), color, msg_ ) );
  else
    tx_ = clone_texture(
        render_text( font::standard(), color, msg_ ) );
}

void OneLineStringView::draw( Texture& tx, Coord coord ) const {
  copy_texture( this->tx_, tx, coord );
}

TextView::TextView( string_view           msg,
                    TextMarkupInfo const& m_info,
                    TextReflowInfo const& r_info ) {
  tx_ = clone_texture( render_text_markup_reflow(
      font::standard(), m_info, r_info, msg ) );
}

void TextView::draw( Texture& tx, Coord coord ) const {
  copy_texture( this->tx_, tx, coord );
}

ButtonBaseView::ButtonBaseView( string label, e_type type )
  : type_( type ) {
  auto info = TextMarkupInfo{}; // Should be irrelevant
  auto size = render_text_markup( font::standard(), info, label )
                  .size()
                  .round_up( Scale{ 8 } );
  auto size_in_blocks = size / Scale{ 8 };
  size_in_blocks.w += 2_w;
  render( label, size_in_blocks );
}

ButtonBaseView::ButtonBaseView( string label )
  : ButtonBaseView( label, e_type::standard ) {}

ButtonBaseView::ButtonBaseView( string label,
                                Delta  size_in_blocks,
                                e_type type )
  : type_( type ) {
  render( label, size_in_blocks );
}

ButtonBaseView::ButtonBaseView( string label,
                                Delta  size_in_blocks )
  : ButtonBaseView( label, size_in_blocks, e_type::standard ) {}

void ButtonBaseView::draw( Texture& tx, Coord coord ) const {
  auto do_copy = [&]( auto const& src ) {
    copy_texture( src, tx, coord );
  };

  using namespace std::chrono;
  using namespace std::literals::chrono_literals;
  auto time        = system_clock::now().time_since_epoch();
  auto one_second  = 1000ms;
  auto half_second = 500ms;
  bool on          = time % one_second > half_second;

  switch( state_ ) {
    case button_state::disabled: do_copy( disabled_ ); return;
    case button_state::down: do_copy( pressed_ ); return;
    case button_state::up:
      if( type_ == e_type::blink && on )
        do_copy( hover_ );
      else
        do_copy( unpressed_ );
      return;
    case button_state::hover:
      if( type_ == e_type::blink && !on )
        do_copy( unpressed_ );
      else
        do_copy( hover_ );
      return;
  }

  SHOULD_NOT_BE_HERE;
}

void ButtonBaseView::render( string const& label,
                             Delta         size_in_blocks ) {
  auto pixel_size = size_in_blocks * Scale{ 8 };
  pressed_        = create_texture_transparent( pixel_size );
  hover_          = create_texture_transparent( pixel_size );
  unpressed_      = create_texture_transparent( pixel_size );
  disabled_       = create_texture_transparent( pixel_size );

  render_rect_of_sprites_with_border(
      unpressed_, Coord{}, size_in_blocks, //
      e_tile::button_up_mm, e_tile::button_up_um,
      e_tile::button_up_lm, e_tile::button_up_ml,
      e_tile::button_up_mr, e_tile::button_up_ul,
      e_tile::button_up_ur, e_tile::button_up_ll,
      e_tile::button_up_lr );

  render_rect_of_sprites_with_border(
      hover_, Coord{}, size_in_blocks, //
      e_tile::button_up_mm, e_tile::button_up_um,
      e_tile::button_up_lm, e_tile::button_up_ml,
      e_tile::button_up_mr, e_tile::button_up_ul,
      e_tile::button_up_ur, e_tile::button_up_ll,
      e_tile::button_up_lr );

  render_rect_of_sprites_with_border(
      disabled_, Coord{}, size_in_blocks, //
      e_tile::button_up_mm, e_tile::button_up_um,
      e_tile::button_up_lm, e_tile::button_up_ml,
      e_tile::button_up_mr, e_tile::button_up_ul,
      e_tile::button_up_ur, e_tile::button_up_ll,
      e_tile::button_up_lr );

  render_rect_of_sprites_with_border(
      pressed_, Coord{}, size_in_blocks, //
      e_tile::button_down_mm, e_tile::button_down_um,
      e_tile::button_down_lm, e_tile::button_down_ml,
      e_tile::button_down_mr, e_tile::button_down_ul,
      e_tile::button_down_ur, e_tile::button_down_ll,
      e_tile::button_down_lr );

  auto info_normal =
      TextMarkupInfo{ gfx::pixel::wood().shaded( 3 ),
                      /*highlight=*/{} };
  auto info_hover =
      TextMarkupInfo{ gfx::pixel::banana(), /*highlight=*/{} };
  auto info_pressed =
      TextMarkupInfo{ gfx::pixel::banana().shaded( 2 ),
                      /*highlight=*/{} };
  auto info_disabled = TextMarkupInfo{ config_palette.grey.n50,
                                       /*highlight=*/{} };

  auto const& tx_normal =
      render_text_markup( font::standard(), info_normal, label );
  auto const& tx_pressed = render_text_markup(
      font::standard(), info_pressed, label );
  auto const& tx_hover =
      render_text_markup( font::standard(), info_hover, label );
  auto const& tx_disabled = render_text_markup(
      font::standard(), info_disabled, label );

  auto unpressed_coord =
      centered( tx_normal.size(), unpressed_.rect() ) + 1_w -
      1_h;
  auto pressed_coord = unpressed_coord + Delta{ -1_w, 1_h };

  copy_texture( tx_normal, unpressed_, unpressed_coord );
  copy_texture( tx_hover, hover_, unpressed_coord );
  copy_texture( tx_pressed, pressed_, pressed_coord );
  copy_texture( tx_disabled, disabled_, unpressed_coord );
}

void SpriteView::draw( Texture& tx, Coord coord ) const {
  render_sprite( tx, tile_, coord, 0, 0 );
}

LineEditorView::LineEditorView( e_font font, W pixels_wide,
                                OnChangeFunc on_change,
                                gfx::pixel fg, gfx::pixel bg,
                                string_view prompt,
                                string_view initial_text )
  : prompt_{ prompt },
    fg_{ fg },
    bg_{ bg },
    font_{ font },
    on_change_{ std::move( on_change ) },
    line_editor_( string( initial_text ), initial_text.size() ),
    input_view_{ 1 },
    background_{},
    current_rendering_{},
    cursor_width_{} {
  string      text( 100, 'X' );
  auto const& X_tx =
      render_text( font, gfx::pixel::wood(), text );
  cursor_width_ = X_tx.size().w / SX{ int( text.size() ) };
  set_pixel_size( Delta{ pixels_wide, X_tx.size().h } );
}

LineEditorView::LineEditorView( int          chars_wide,
                                string_view  initial_text,
                                OnChangeFunc on_change )
  : LineEditorView(
        font::standard(),
        render_text( font::standard(), gfx::pixel::wood(),
                     string( chars_wide, 'X' ) )
            .size()
            .w,
        std::move( on_change ), gfx::pixel::wood(),
        gfx::pixel::banana(), /*prompt=*/"", initial_text ) {}

LineEditorView::LineEditorView( int         chars_wide,
                                string_view initial_text )
  : LineEditorView( chars_wide, initial_text,
                    []( auto const& ) {} ) {}

void LineEditorView::set_pixel_size( Delta const& size ) {
  render_background( size );
  // This doesn't work precisely because 1) the font may not be
  // fixed width, and 2) cursor_width_ is just an average.
  input_view_ = LineEditorInputView{ size.w / cursor_width_ };
  update_visible_string();
}

void LineEditorView::render_background( Delta const& size ) {
  background_ = Texture::create( size + Delta{ 2_w, 2_h } );
  background_.fill( bg_ );
}

// Implement Object
void LineEditorView::draw( Texture& tx, Coord coord ) const {
  copy_texture( background_, tx, coord );
  auto        all_chars = prompt_ + current_rendering_;
  auto const& text_tx   = render_text( font_, fg_, all_chars );
  auto        bounds    = background_.size();
  auto        copy_size = min( bounds, text_tx.size() );
  auto        from_rect = Rect::from( Coord{}, copy_size );
  auto        to_rect =
      Rect::from( coord + Delta{ 1_w, 1_h }, copy_size );
  copy_texture( text_tx, tx, from_rect, to_rect );

  auto rel_pos = input_view_.rel_pos( line_editor_.pos() ) +
                 int( prompt_.size() );
  CHECK( rel_pos <= int( all_chars.size() ) );
  string string_up_to_cursor( all_chars.begin(),
                              all_chars.begin() + rel_pos );
  auto   rel_cursor_pixels =
      rel_pos == 0
            ? W{ 0 } // render_text might return 1_w in this case.
            : render_text( font_, fg_, string_up_to_cursor )
                .size()
                .w;
  Rect cursor{ coord.x + 1_w + rel_cursor_pixels, coord.y + 1_h,
               cursor_width_, background_.size().h - 2_h };
  render_rect( tx, fg_, cursor );
}

bool LineEditorView::on_key( input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down ) return false;
  if( !line_editor_.input( event ) ) return false;
  update_visible_string();
  return true;
}

void LineEditorView::update_visible_string() {
  current_rendering_ = input_view_.render(
      line_editor_.pos(), line_editor_.buffer() );
  // This technically doesn't necessarily need to be called each
  // time the visible string gets updated, because not all of
  // those updates consist of modifying the string, i.e., some
  // just change the visible window, but it probably doesn't hurt
  // to call it.
  on_change_( current_rendering_ );
}

void LineEditorView::clear() {
  line_editor_.clear();
  update_visible_string();
}

void LineEditorView::set( std::string_view new_string,
                          maybe<int>       cursor_pos ) {
  line_editor_.set( new_string, cursor_pos );
  update_visible_string();
}

/****************************************************************
** Derived Views
*****************************************************************/
unique_ptr<PlainMessageBoxView> PlainMessageBoxView::create(
    string_view msg, wait_promise<> on_close ) {
  TextMarkupInfo m_info{
      /*normal=*/config_ui.dialog_text.normal,
      /*highlight=*/config_ui.dialog_text.highlighted };
  TextReflowInfo r_info{
      /*max_cols=*/config_ui.dialog_text.columns };
  unique_ptr<TextView> tview =
      make_unique<TextView>( msg, m_info, r_info );
  return make_unique<PlainMessageBoxView>(
      std::move( tview ), std::move( on_close ) );
}

PlainMessageBoxView::PlainMessageBoxView(
    unique_ptr<TextView> tview, wait_promise<> on_close )
  : CompositeSingleView( std::move( tview ), Coord{} ),
    on_close_( std::move( on_close ) ) {}

bool PlainMessageBoxView::on_key(
    input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down ) return false;
  // It's a key down.
  switch( event.keycode ) {
    case ::SDLK_RETURN:
    case ::SDLK_KP_ENTER:
    case ::SDLK_ESCAPE:
    case ::SDLK_KP_5:
    case ::SDLK_SPACE:
      on_close_.set_value_emplace();
      return true;
    default: //
      return false;
  }
}

PaddingView::PaddingView( std::unique_ptr<View> view, int pixels,
                          bool l, bool r, bool u, bool d )
  : CompositeSingleView( std::move( view ),
                         Coord{} +                       //
                             ( l ? W{ pixels } : 0_w ) + //
                             ( u ? H{ pixels } : 0_h ) ),
    pixels_( pixels ),
    l_( l ),
    r_( r ),
    u_( u ),
    d_( d ),
    delta_( single()->delta() + //
            ( l ? W{ pixels_ } : 0_w ) +
            ( u ? H{ pixels_ } : 0_h ) + //
            ( r ? W{ pixels_ } : 0_w ) + //
            ( d ? H{ pixels_ } : 0_h ) ) {}

void PaddingView::notify_children_updated() {
  delta_ = single()->delta() + //
           ( l_ ? W{ pixels_ } : 0_w ) +
           ( u_ ? H{ pixels_ } : 0_h ) + //
           ( r_ ? W{ pixels_ } : 0_w ) + //
           ( d_ ? H{ pixels_ } : 0_h );
}

// This prevents more padding from being added (this is already
// ad padding view).
bool PaddingView::can_pad_immediate_children() const {
  // If we're asking whether we can add padding into a
  // PaddingView then something has gone wrong somewhere.
  SHOULD_NOT_BE_HERE;
}

ButtonView::ButtonView( string label, OnClickFunc on_click )
  : ButtonBaseView( std::move( label ) ),
    on_click_( std::move( on_click ) ) {
  set_state( button_state::up );
}

ButtonView::ButtonView( string label, Delta size_in_blocks,
                        OnClickFunc on_click )
  : ButtonBaseView( std::move( label ), size_in_blocks ),
    on_click_( std::move( on_click ) ) {
  set_state( button_state::up );
}

bool ButtonView::on_mouse_move(
    input::mouse_move_event_t const& event ) {
  if( state() != button_state::disabled ) {
    switch( state() ) {
      case button_state::down: break;
      case button_state::up:
        set_state( event.l_mouse_down ? button_state::down
                                      : button_state::hover );
        break;
      case button_state::disabled: break;
      case button_state::hover: break;
    }
  }
  return true;
}

bool ButtonView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  if( state() != button_state::disabled ) {
    switch( event.buttons ) {
      case input::e_mouse_button_event::left_down:
        set_state( button_state::down );
        break;
      case input::e_mouse_button_event::left_up:
        set_state( button_state::hover );
        // Must call on_click_ after setting the state to `hover`
        // just in case the on_click_ function wants to change
        // the state of the button (e.g., to disabled). In other-
        // words, we should allow on_click_() to have the last
        // word in this function.
        on_click_();
        break;
      default: break;
    }
  }
  return false;
}

void ButtonView::on_mouse_leave( Coord /*unused*/ ) {
  if( state() != button_state::disabled )
    set_state( button_state::up );
}

void ButtonView::enable( bool enabled ) {
  if( enabled ) {
    set_state( button_state::up );
  } else {
    set_state( button_state::disabled );
  }
}

bool ButtonView::enabled() const {
  return state() != button_state::disabled;
}

void ButtonView::blink( bool enabled ) {
  if( enabled ) {
    set_type( e_type::blink );
  } else {
    set_type( e_type::standard );
  }
}

constexpr Delta ok_cancel_button_size_blocks{ 2_h, 8_w };

OkCancelView::OkCancelView( ButtonView::OnClickFunc on_ok,
                            ButtonView::OnClickFunc on_cancel ) {
  auto ok_button = make_unique<ButtonView>(
      "OK", ok_cancel_button_size_blocks, std::move( on_ok ) );
  auto cancel_button = make_unique<ButtonView>(
      "Cancel", ok_cancel_button_size_blocks,
      std::move( on_cancel ) );

  ok_ref_     = ok_button.get();
  cancel_ref_ = cancel_button.get();

  ok_     = std::move( ok_button );
  cancel_ = std::move( cancel_button );
}

Coord OkCancelView::pos_of( int idx ) const {
  if( idx == 0 ) return Coord{};
  if( idx == 1 ) return Coord{} + ok_->delta().w;
  SHOULD_NOT_BE_HERE;
  return {};
}

unique_ptr<View>& OkCancelView::mutable_at( int idx ) {
  CHECK( idx == 0 || idx == 1 );
  return ( idx == 0 ) ? ok_ : cancel_;
}

OkButtonView::OkButtonView( ButtonView::OnClickFunc on_ok )
  : CompositeSingleView( make_unique<ButtonView>(
                             "OK", ok_cancel_button_size_blocks,
                             std::move( on_ok ) ),
                         Coord{} ) {
  ok_ref_ = single()->cast<ButtonView>();
}

VerticalArrayView::VerticalArrayView(
    vector<unique_ptr<View>> views, align how )
  : alignment_( how ) {
  for( auto& view : views ) {
    OwningPositionedView pos_view( std::move( view ), Coord{} );
    push_back( std::move( pos_view ) );
  }
  notify_children_updated();
}

// When a child view is updated then we must recompute the posi-
// tions of all the views.
void VerticalArrayView::notify_children_updated() {
  recompute_child_positions();
}

// When a child view is updated then we must recompute the posi-
// tions of all the views.
void VerticalArrayView::recompute_child_positions() {
  W max_width = 0_w;
  for( int i = 0; i < count(); ++i )
    max_width = std::max( max_width, at( i ).view->delta().w );
  Y y = 0_y;
  for( int i = 0; i < count(); ++i ) {
    auto& view = mutable_at( i );
    auto  size = view->delta();
    X     x{ 0 };
    switch( alignment_ ) {
      case align::left: x = 0_x; break;
      case align::right: x = 0_x + ( max_width - size.w ); break;
      case align::center:
        x = 0_x + ( max_width / 2 - size.w / 2 );
        break;
    }
    CHECK( x >= 0_x );
    CHECK( x <= 0_x + max_width );
    OwningPositionedView pos_view( std::move( view ),
                                   Coord{ x, y } );
    ( *this )[i] = std::move( pos_view );
    y += size.h;
  }
}

HorizontalArrayView::HorizontalArrayView(
    vector<unique_ptr<View>> views, align how )
  : alignment_( how ) {
  for( auto& view : views ) {
    OwningPositionedView pos_view( std::move( view ), Coord{} );
    push_back( std::move( pos_view ) );
  }
  notify_children_updated();
}

// When a child view is updated then we must recompute the posi-
// tions of all the views.
void HorizontalArrayView::notify_children_updated() {
  recompute_child_positions();
}

// When a child view is updated then we must recompute the posi-
// tions of all the views.
void HorizontalArrayView::recompute_child_positions() {
  H max_height = 0_h;
  for( int i = 0; i < count(); ++i )
    max_height = std::max( max_height, at( i ).view->delta().h );
  X x = 0_x;
  for( int i = 0; i < count(); ++i ) {
    auto& view = mutable_at( i );
    auto  size = view->delta();
    Y     y{ 0 };
    switch( alignment_ ) {
      case align::up: y = 0_y; break;
      case align::down: y = 0_y + ( max_height - size.h ); break;
      case align::middle:
        y = 0_y + ( max_height / 2 - size.h / 2 );
        break;
    }
    CHECK( y >= 0_y );
    CHECK( y <= 0_y + max_height );
    OwningPositionedView pos_view( std::move( view ),
                                   Coord{ x, y } );
    ( *this )[i] = std::move( pos_view );
    x += size.w;
  }
}

OkCancelAdapterView::OkCancelAdapterView( unique_ptr<View> view,
                                          OnClickFunc on_click )
  : VerticalArrayView(
        params_to_vector<unique_ptr<View>>(
            std::move( view ),
            make_unique<OkCancelView>(
                /*on_ok=*/
                [on_click]() { on_click( e_ok_cancel::ok ); },
                /*on_cancel=*/
                [on_click]() {
                  on_click( e_ok_cancel::cancel );
                } ) ),
        VerticalArrayView::align::center ) {}

OptionSelectItemView::OptionSelectItemView( string msg )
  : active_{ e_option_active::inactive },
    background_active_( make_unique<SolidRectView>(
        config_palette.yellow.sat1.lum11 ) ),
    background_inactive_( make_unique<SolidRectView>(
        config_palette.orange.sat0.lum3 ) ),
    foreground_active_( make_unique<OneLineStringView>(
        msg, config_palette.orange.sat0.lum2,
        /*shadow=*/true ) ),
    foreground_inactive_( make_unique<OneLineStringView>(
        msg, config_palette.orange.sat1.lum11,
        /*shadow=*/true ) ) {
  auto delta_active   = foreground_active_->delta();
  auto delta_inactive = foreground_inactive_->delta();
  background_active_->cast<SolidRectView>()->set_delta(
      delta_active );
  background_inactive_->cast<SolidRectView>()->set_delta(
      delta_inactive );
}

Coord OptionSelectItemView::pos_of( int idx ) const {
  CHECK( idx == 0 || idx == 1 );
  return Coord{};
}

unique_ptr<View>& OptionSelectItemView::mutable_at( int idx ) {
  CHECK( idx == 0 || idx == 1 );
  switch( idx ) {
    case 0:
      switch( active_ ) {
        case e_option_active::active: return background_active_;
        case e_option_active::inactive:
          return background_inactive_;
      }
      break;
    case 1:
      switch( active_ ) {
        case e_option_active::active: return foreground_active_;
        case e_option_active::inactive:
          return foreground_inactive_;
      }
  }
  SHOULD_NOT_BE_HERE;
}

void OptionSelectItemView::grow_to( W w ) {
  auto new_delta = foreground_active_->delta();
  if( new_delta.w > w )
    // we only grow here, not shrink.
    return;
  new_delta.w = w;
  background_active_->cast<SolidRectView>()->set_delta(
      new_delta );
  background_inactive_->cast<SolidRectView>()->set_delta(
      new_delta );
}

OptionSelectView::OptionSelectView(
    vector<string> const& options, int initial_selection )
  : selected_{ initial_selection } {
  CHECK( options.size() > 0 );
  CHECK( selected_ >= 0 && selected_ < int( options.size() ) );

  Coord so_far{};
  W     min_width{ 0 };
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

OptionSelectItemView* OptionSelectView::get_view( int item ) {
  CHECK( item >= 0 && item < count(),
         "item '{}' is out of bounds", item );
  auto* view    = at( item ).view;
  auto* o_s_i_v = view->cast<OptionSelectItemView>();
  return o_s_i_v;
}

// TODO: duplication
OptionSelectItemView const* OptionSelectView::get_view(
    int item ) const {
  CHECK( item >= 0 && item < count(),
         "item '{}' is out of bounds", item );
  auto* view    = at( item ).view;
  auto* o_s_i_v = view->cast<OptionSelectItemView>();
  return o_s_i_v;
}

void OptionSelectView::set_selected( int item ) {
  get_view( selected_ )->set_active( e_option_active::inactive );
  get_view( item )->set_active( e_option_active::active );
  selected_ = item;
}

void OptionSelectView::grow_to( W w ) {
  for( auto p_view : *this ) {
    auto* view    = p_view.view;
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
    default: break;
  }
  return false;
}

string const& OptionSelectView::get_selected() const {
  return get_view( selected_ )->line();
}

FakeUnitView::FakeUnitView( e_unit_type type, e_nation nation,
                            e_unit_orders orders )
  : CompositeSingleView(
        make_unique<SpriteView>( unit_attr( type ).tile ),
        Coord{} ),
    type_( type ),
    nation_( nation ),
    orders_( orders ) {}

void FakeUnitView::draw( Texture& tx, Coord coord ) const {
  this->CompositeSingleView::draw( tx, coord );
  render_nationality_icon( tx, type_, nation_, orders_, coord );
}

ClickableView::ClickableView(
    unique_ptr<View> view, std::function<void( void )> on_click )
  : CompositeSingleView( std::move( view ), Coord{} ),
    on_click_( std::move( on_click ) ) {}

bool ClickableView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  if( event.buttons == input::e_mouse_button_event::left_up )
    on_click_();
  return true;
}

BorderView::BorderView( unique_ptr<View> view, gfx::pixel color,
                        int padding, bool on_initially )
  : CompositeSingleView(
        std::move( view ),
        Coord{ 1_x + W{ padding }, 1_y + H{ padding } } ),
    color_( color ),
    on_( on_initially ),
    padding_( padding ) {}

Delta BorderView::delta() const {
  return this->CompositeSingleView::delta() +
         Delta{ ( 1_w + W{ padding_ } ) * 2_sx,
                ( 1_h + H{ padding_ } ) * 2_sy };
}

void BorderView::draw( Texture& tx, Coord coord ) const {
  this->CompositeSingleView::draw(
      tx, coord + Delta{ 1_w + W{ padding_ },
                         1_h + H{ padding_ } } );
  if( on_ )
    render_rect( tx, color_, Rect::from( coord, delta() ) );
}

} // namespace rn::ui
