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
#include "logger.hpp"
#include "math.hpp"
#include "render.hpp"
#include "text.hpp"
#include "util.hpp"

// config
#include "config/ui.rds.hpp"
#include "config/unit-type.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/lambda.hpp"

// C++ standard library
#include <chrono>
#include <numeric>

using namespace std;

namespace rn::ui {

namespace {} // namespace

/****************************************************************
** CompositeView
*****************************************************************/
void CompositeView::children_updated() {
  for( auto p_view : *this )
    if( auto cv = p_view.view->cast_safe<CompositeView>();
        cv.has_value() )
      ( *cv )->children_updated();
  // Finally, this node. Note that this is the only place that
  // this notify_children_updated function should be called.
  notify_children_updated();
}

void CompositeView::advance_state() {
  for( auto p_view : *this ) p_view.view->advance_state();
}

void CompositeView::draw( rr::Renderer& renderer,
                          Coord         coord ) const {
  // Draw each of the sub views, by augmenting its origin (which
  // is relative to the origin of the parent by the origin that
  // we have been given.
  for( auto [view, view_coord] : *this )
    view->draw( renderer, coord + ( view_coord - Coord() ) );
}

Delta CompositeView::delta() const {
  auto uni0n = L2( _1.uni0n( _2.view->rect( _2.coord ) ) );
  auto rect  = accumulate( begin(), end(), Rect{}, uni0n );
  return { .w = rect.w, .h = rect.h };
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

bool CompositeView::on_win_event(
    input::win_event_t const& event ) {
  // Window events are special; we want all child views to get
  // them.
  for( auto p_view : *this )
    (void)p_view.view->on_win_event( event );
  return false;
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

/****************************************************************
** CompositeSingleView
*****************************************************************/
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

/****************************************************************
** VectorView
*****************************************************************/
Coord VectorView::pos_of( int idx ) const {
  CHECK( idx >= 0 && idx < int( views_.size() ) );
  return views_[idx].coord;
}

unique_ptr<View>& VectorView::mutable_at( int idx ) {
  CHECK( idx >= 0 && idx < int( views_.size() ) );
  return views_[idx].view;
}

/****************************************************************
** SolidRectView
*****************************************************************/
void SolidRectView::draw( rr::Renderer& renderer,
                          Coord         coord ) const {
  renderer.painter().draw_solid_rect( rect( coord ), color_ );
}

/****************************************************************
** OneLineStringView
*****************************************************************/
// NOTE: If you add reflow info to this constructor, don't forget
// to add it into the calculation of the text size as well.
OneLineStringView::OneLineStringView( string     msg,
                                      gfx::pixel color,
                                      Delta      size_override )
  : msg_( std::move( msg ) ),
    view_size_( size_override ),
    text_size_( rendered_text_size_no_reflow( msg ) ),
    color_( color ) {}

OneLineStringView::OneLineStringView( string     msg,
                                      gfx::pixel color )
  : OneLineStringView( msg, color,
                       rendered_text_size_no_reflow( msg ) ) {}

Delta OneLineStringView::delta() const { return view_size_; }

void OneLineStringView::draw( rr::Renderer& renderer,
                              Coord         coord ) const {
  int const start_offset = ( view_size_.h - text_size_.h ) / 2;
  TextMarkupInfo const markup_info{
      .normal = color_,
      // FIXME
      .highlight = gfx::pixel::white() };
  render_text_markup( renderer,
                      coord + Delta{ .h = start_offset },
                      e_font{}, markup_info, msg_ );
}

/****************************************************************
** TextView
*****************************************************************/
TextView::TextView( std::string           msg,
                    TextMarkupInfo const& m_info,
                    TextReflowInfo const& r_info )
  : msg_( std::move( msg ) ),
    text_size_{ rendered_text_size( r_info, msg_ ) },
    markup_info_( m_info ),
    reflow_info_( r_info ) {}

TextView::TextView( std::string msg )
  : TextView( std::move( msg ), default_text_markup_info(),
              default_text_reflow_info() ) {}

Delta TextView::delta() const { return text_size_; }

void TextView::draw( rr::Renderer& renderer,
                     Coord         coord ) const {
  render_text_markup_reflow( renderer, coord, font::standard(),
                             markup_info_, reflow_info_, msg_ );
}

/****************************************************************
** ButtonBaseView
*****************************************************************/
ButtonBaseView::ButtonBaseView( string label )
  : ButtonBaseView( std::move( label ), e_type::standard ) {}

ButtonBaseView::ButtonBaseView( string label, e_type type )
  : ButtonBaseView(
        label,
        rendered_text_size( /*reflow_info=*/{}, label )
                    .round_up( Delta{ .w = 8, .h = 8 } ) /
                Delta{ .w = 8, .h = 8 } +
            Delta{ .w = 2 } + Delta{ .h = 1 },
        type ) {}

ButtonBaseView::ButtonBaseView( string label,
                                Delta  size_in_blocks )
  : ButtonBaseView( std::move( label ), size_in_blocks,
                    e_type::standard ) {}

ButtonBaseView::ButtonBaseView( string label,
                                Delta  size_in_blocks,
                                e_type type )
  : label_( std::move( label ) ),
    type_( type ),
    size_in_pixels_( size_in_blocks * Delta{ .w = 8, .h = 8 } ),
    text_size_in_pixels_(
        rendered_text_size( /*reflow_info=*/{}, label_ ) ) {}

Delta ButtonBaseView::delta() const { return size_in_pixels_; }

void ButtonBaseView::draw( rr::Renderer& renderer,
                           Coord         coord ) const {
  using namespace std::chrono;
  using namespace std::literals::chrono_literals;
  auto time        = system_clock::now().time_since_epoch();
  auto one_second  = 1000ms;
  auto half_second = 500ms;
  bool on          = time % one_second > half_second;

  switch( state_ ) {
    case button_state::disabled:
      render_disabled( renderer, coord );
      return;
    case button_state::down:
      render_pressed( renderer, coord );
      return;
    case button_state::up:
      if( type_ == e_type::blink && on )
        render_hover( renderer, coord );
      else
        render_unpressed( renderer, coord );
      return;
    case button_state::hover:
      if( type_ == e_type::blink && !on )
        render_unpressed( renderer, coord );
      else
        render_hover( renderer, coord );
      return;
  }

  SHOULD_NOT_BE_HERE;
}

void ButtonBaseView::render_disabled( rr::Renderer& renderer,
                                      gfx::point where ) const {
  rr::Painter painter = renderer.painter();
  render_rect_of_sprites_with_border(
      painter, Coord::from_gfx( where ),
      size_in_pixels_ / Delta{ .w = 8, .h = 8 }, //
      e_tile::button_up_mm, e_tile::button_up_um,
      e_tile::button_up_lm, e_tile::button_up_ml,
      e_tile::button_up_mr, e_tile::button_up_ul,
      e_tile::button_up_ur, e_tile::button_up_ll,
      e_tile::button_up_lr );

  auto markup_info = TextMarkupInfo{
      gfx::pixel{ .r = 0x50, .g = 0x50, .b = 0x50, .a = 255 },
      /*highlight=*/{} };

  Coord text_position =
      centered( text_size_in_pixels_,
                Rect::from( Coord::from_gfx( where ),
                            size_in_pixels_ ) ) +
      Delta{ .w = 1 } - Delta{ .h = 1 };
  render_text_markup( renderer, text_position, font::standard(),
                      markup_info, label_ );
}

void ButtonBaseView::render_pressed( rr::Renderer& renderer,
                                     gfx::point where ) const {
  rr::Painter painter = renderer.painter();
  render_rect_of_sprites_with_border(
      painter, Coord::from_gfx( where ),
      size_in_pixels_ / Delta{ .w = 8, .h = 8 }, //
      e_tile::button_down_mm, e_tile::button_down_um,
      e_tile::button_down_lm, e_tile::button_down_ml,
      e_tile::button_down_mr, e_tile::button_down_ul,
      e_tile::button_down_ur, e_tile::button_down_ll,
      e_tile::button_down_lr );

  auto markup_info =
      TextMarkupInfo{ gfx::pixel::banana().shaded( 2 ),
                      /*highlight=*/{} };
  Coord text_position =
      centered( text_size_in_pixels_,
                Rect::from( Coord::from_gfx( where ),
                            size_in_pixels_ ) ) +
      -Delta{ .w = 1 } + Delta{ .h = 1 };
  render_text_markup( renderer, text_position, font::standard(),
                      markup_info, label_ );
}

void ButtonBaseView::render_unpressed( rr::Renderer& renderer,
                                       gfx::point where ) const {
  rr::Painter painter = renderer.painter();
  render_rect_of_sprites_with_border(
      painter, Coord::from_gfx( where ),
      size_in_pixels_ / Delta{ .w = 8, .h = 8 }, //
      e_tile::button_up_mm, e_tile::button_up_um,
      e_tile::button_up_lm, e_tile::button_up_ml,
      e_tile::button_up_mr, e_tile::button_up_ul,
      e_tile::button_up_ur, e_tile::button_up_ll,
      e_tile::button_up_lr );

  auto markup_info =
      TextMarkupInfo{ gfx::pixel::wood().shaded( 3 ),
                      /*highlight=*/{} };

  Coord text_position =
      centered( text_size_in_pixels_,
                Rect::from( Coord::from_gfx( where ),
                            size_in_pixels_ ) ) +
      Delta{ .w = 1 } - Delta{ .h = 1 };
  render_text_markup( renderer, text_position, font::standard(),
                      markup_info, label_ );
}

void ButtonBaseView::render_hover( rr::Renderer& renderer,
                                   gfx::point    where ) const {
  rr::Painter painter = renderer.painter();
  render_rect_of_sprites_with_border(
      painter, Coord::from_gfx( where ),
      size_in_pixels_ / Delta{ .w = 8, .h = 8 }, //
      e_tile::button_up_mm, e_tile::button_up_um,
      e_tile::button_up_lm, e_tile::button_up_ml,
      e_tile::button_up_mr, e_tile::button_up_ul,
      e_tile::button_up_ur, e_tile::button_up_ll,
      e_tile::button_up_lr );

  auto markup_info =
      TextMarkupInfo{ gfx::pixel::banana(), /*highlight=*/{} };

  Coord text_position =
      centered( text_size_in_pixels_,
                Rect::from( Coord::from_gfx( where ),
                            size_in_pixels_ ) ) +
      Delta{ .w = 1 } - Delta{ .h = 1 };
  render_text_markup( renderer, text_position, font::standard(),
                      markup_info, label_ );
}

/****************************************************************
** SpriteView
*****************************************************************/
void SpriteView::draw( rr::Renderer& renderer,
                       Coord         coord ) const {
  rr::Painter painter = renderer.painter();
  render_sprite( painter, tile_, coord );
}

/****************************************************************
** LineEditorView
*****************************************************************/
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
    input_view_( 1 ),
    current_rendering_{},
    cursor_width_{} {
  string text( 100, 'X' );
  Delta  char_delta = Delta::from_gfx(
      rr::rendered_text_line_size_pixels( text ) );

  cursor_width_ = char_delta.w / SX{ int( text.size() ) };

  Delta pixel_size = Delta{ pixels_wide, char_delta.h };
  // This doesn't work precisely because 1) the font may not be
  // fixed width, and 2) cursor_width_ is just an average.
  input_view_ =
      LineEditorInputView{ pixel_size.w / cursor_width_ };
  update_visible_string();
}

LineEditorView::LineEditorView( int          chars_wide,
                                string_view  initial_text,
                                OnChangeFunc on_change )
  : LineEditorView(
        font::standard(),
        Delta::from_gfx( rr::rendered_text_line_size_pixels(
                             string( chars_wide, 'X' ) ) )
            .w,
        std::move( on_change ), gfx::pixel::wood(),
        gfx::pixel::banana(), /*prompt=*/"", initial_text ) {}

LineEditorView::LineEditorView( int         chars_wide,
                                string_view initial_text )
  : LineEditorView( chars_wide, initial_text,
                    []( auto const& ) {} ) {}

Delta LineEditorView::delta() const {
  return Delta::from_gfx( rr::rendered_text_line_size_pixels(
             string( input_view_.width(), 'X' ) ) ) +
         Delta{ .w = 4, .h = 4 };
}

void LineEditorView::render_background( rr::Renderer& renderer,
                                        Rect const&   r ) const {
  renderer.painter().draw_solid_rect(
      Rect::from( r.upper_left(), r.delta() ), bg_ );
}

// Implement Object
void LineEditorView::draw( rr::Renderer& renderer,
                           Coord         coord ) const {
  render_background( renderer, rect( coord ) );
  auto      all_chars = prompt_ + current_rendering_;
  gfx::size text_size =
      rr::rendered_text_line_size_pixels( current_rendering_ );
  Y text_pos_y =
      centered( Delta::from_gfx( text_size ), rect( coord ) ).y;
  rr::Typer typer = renderer.typer(
      Coord{ .x = coord.x + 1, .y = text_pos_y }, fg_ );
  typer.write( all_chars );

  auto rel_pos = input_view_.rel_pos( line_editor_.pos() ) +
                 int( prompt_.size() );
  CHECK( rel_pos <= int( all_chars.size() ) );
  string string_up_to_cursor( all_chars.begin(),
                              all_chars.begin() + rel_pos );
  W      rel_cursor_pixels =
      rel_pos == 0
               ? W{ 0 }
          // The rendered text might have width 1 in this case.
               : Delta::from_gfx( rr::rendered_text_line_size_pixels(
                                 string_up_to_cursor ) )
                .w;
  Rect cursor{ .x = coord.x + 1 + rel_cursor_pixels,
               .y = coord.y + 1,
               .w = cursor_width_,
               .h = delta().h - 2 };
  renderer.painter().draw_solid_rect( cursor, fg_ );
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
** PlainMessageBoxView
*****************************************************************/
TextMarkupInfo const& default_text_markup_info() {
  static TextMarkupInfo info{
      /*normal=*/config_ui.dialog_text.normal,
      /*highlight=*/config_ui.dialog_text.highlighted };
  return info;
}

TextReflowInfo const& default_text_reflow_info() {
  static TextReflowInfo info{
      /*max_cols=*/config_ui.dialog_text.columns };
  return info;
}

unique_ptr<PlainMessageBoxView> PlainMessageBoxView::create(
    string_view msg, wait_promise<> on_close ) {
  TextMarkupInfo const& m_info = default_text_markup_info();
  TextReflowInfo const& r_info = default_text_reflow_info();
  unique_ptr<TextView>  tview =
      make_unique<TextView>( string( msg ), m_info, r_info );
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

/****************************************************************
** PaddingView
*****************************************************************/
PaddingView::PaddingView( std::unique_ptr<View> view, int pixels,
                          bool l, bool r, bool u, bool d )
  : CompositeSingleView(
        std::move( view ),
        Coord{} +                                   //
            Delta{ .w = ( l ? W{ pixels } : 0 ) } + //
            Delta{ .h = ( u ? H{ pixels } : 0 ) } ),
    pixels_( pixels ),
    l_( l ),
    r_( r ),
    u_( u ),
    d_( d ),
    delta_( single()->delta() + //
            Delta{ .w = ( l ? W{ pixels_ } : 0 ) } +
            Delta{ .h = ( u ? H{ pixels_ } : 0 ) } + //
            Delta{ .w = ( r ? W{ pixels_ } : 0 ) } + //
            Delta{ .h = ( d ? H{ pixels_ } : 0 ) } ) {}

void PaddingView::notify_children_updated() {
  delta_ = single()->delta() + //
           Delta{ .w = ( l_ ? W{ pixels_ } : 0 ) } +
           Delta{ .h = ( u_ ? H{ pixels_ } : 0 ) } + //
           Delta{ .w = ( r_ ? W{ pixels_ } : 0 ) } + //
           Delta{ .h = ( d_ ? H{ pixels_ } : 0 ) };
}

// This prevents more padding from being added (this is already
// ad padding view).
bool PaddingView::can_pad_immediate_children() const {
  // If we're asking whether we can add padding into a
  // PaddingView then something has gone wrong somewhere.
  SHOULD_NOT_BE_HERE;
}

/****************************************************************
** ButtonView
*****************************************************************/
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

void ButtonView::click() const { on_click_(); }

/****************************************************************
** OkCancelView2
*****************************************************************/
constexpr Delta ok_cancel_button_size_blocks{ .w = 8, .h = 2 };

OkCancelView2::OkCancelView2() {
  auto ok_button = make_unique<ButtonView>(
      "OK", ok_cancel_button_size_blocks,
      [this] { clicks_.send( e_ok_cancel::ok ); } );
  auto cancel_button = make_unique<ButtonView>(
      "Cancel", ok_cancel_button_size_blocks,
      [this] { clicks_.send( e_ok_cancel::cancel ); } );

  ok_ref_     = ok_button.get();
  cancel_ref_ = cancel_button.get();

  ok_     = std::move( ok_button );
  cancel_ = std::move( cancel_button );
}

Coord OkCancelView2::pos_of( int idx ) const {
  if( idx == 0 ) return Coord{};
  if( idx == 1 ) return Coord{} + Delta{ .w = ok_->delta().w };
  SHOULD_NOT_BE_HERE;
}

unique_ptr<View>& OkCancelView2::mutable_at( int idx ) {
  CHECK( idx == 0 || idx == 1 );
  return ( idx == 0 ) ? ok_ : cancel_;
}

wait<e_ok_cancel> OkCancelView2::next() {
  return clicks_.next();
}

bool OkCancelView2::on_key( input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down ) return false;
  // It's a key down.
  switch( event.keycode ) {
    case ::SDLK_ESCAPE: cancel_ref_->click(); return true;
    default: break;
  }
  return false;
}

/****************************************************************
** OkCancelView (deprecated)
*****************************************************************/
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
  if( idx == 1 ) return Coord{} + Delta{ .w = ok_->delta().w };
  SHOULD_NOT_BE_HERE;
}

unique_ptr<View>& OkCancelView::mutable_at( int idx ) {
  CHECK( idx == 0 || idx == 1 );
  return ( idx == 0 ) ? ok_ : cancel_;
}

/****************************************************************
** OkButtonView
*****************************************************************/
OkButtonView::OkButtonView( ButtonView::OnClickFunc on_ok )
  : CompositeSingleView( make_unique<ButtonView>(
                             "OK", ok_cancel_button_size_blocks,
                             std::move( on_ok ) ),
                         Coord{} ) {
  ok_ref_ = single()->cast<ButtonView>();
}

/****************************************************************
** VerticalArrayView
*****************************************************************/
VerticalArrayView::VerticalArrayView(
    vector<unique_ptr<View>> views, align how )
  : alignment_( how ) {
  for( auto& view : views ) add_view( std::move( view ) );
  recompute_child_positions();
}

VerticalArrayView::VerticalArrayView( align how )
  : VerticalArrayView( {}, how ) {}

void VerticalArrayView::add_view( unique_ptr<View> view ) {
  OwningPositionedView pos_view{ .view  = std::move( view ),
                                 .coord = Coord{} };
  push_back( std::move( pos_view ) );
}

// When a child view is updated then we must recompute the posi-
// tions of all the views.
void VerticalArrayView::notify_children_updated() {
  recompute_child_positions();
}

// When a child view is updated then we must recompute the posi-
// tions of all the views.
void VerticalArrayView::recompute_child_positions() {
  W max_width = 0;
  for( int i = 0; i < count(); ++i )
    max_width = std::max( max_width, at( i ).view->delta().w );
  Y y = 0;
  for( int i = 0; i < count(); ++i ) {
    auto& view = mutable_at( i );
    auto  size = view->delta();
    X     x{ 0 };
    switch( alignment_ ) {
      case align::left: x = 0; break;
      case align::right: x = 0 + ( max_width - size.w ); break;
      case align::center:
        x = 0 + ( max_width / 2 - size.w / 2 );
        break;
    }
    CHECK( x >= 0 );
    CHECK( x <= 0 + max_width );
    OwningPositionedView pos_view{
        .view  = std::move( view ),
        .coord = Coord{ .x = x, .y = y } };
    ( *this )[i] = std::move( pos_view );
    y += size.h;
  }
}

/****************************************************************
** HorizontalArrayView
*****************************************************************/
HorizontalArrayView::HorizontalArrayView(
    vector<unique_ptr<View>> views, align how )
  : alignment_( how ) {
  for( auto& view : views ) add_view( std::move( view ) );
  recompute_child_positions();
}

HorizontalArrayView::HorizontalArrayView( align how )
  : HorizontalArrayView( {}, how ) {}

void HorizontalArrayView::add_view( unique_ptr<View> view ) {
  OwningPositionedView pos_view{ .view  = std::move( view ),
                                 .coord = Coord{} };
  push_back( std::move( pos_view ) );
}

// When a child view is updated then we must recompute the posi-
// tions of all the views.
void HorizontalArrayView::notify_children_updated() {
  recompute_child_positions();
}

// When a child view is updated then we must recompute the posi-
// tions of all the views.
void HorizontalArrayView::recompute_child_positions() {
  H max_height = 0;
  for( int i = 0; i < count(); ++i )
    max_height = std::max( max_height, at( i ).view->delta().h );
  X x = 0;
  for( int i = 0; i < count(); ++i ) {
    auto& view = mutable_at( i );
    auto  size = view->delta();
    Y     y{ 0 };
    switch( alignment_ ) {
      case align::up: y = 0; break;
      case align::down: y = 0 + ( max_height - size.h ); break;
      case align::middle:
        y = 0 + ( max_height / 2 - size.h / 2 );
        break;
    }
    CHECK( y >= 0 );
    CHECK( y <= 0 + max_height );
    OwningPositionedView pos_view{
        .view  = std::move( view ),
        .coord = Coord{ .x = x, .y = y } };
    ( *this )[i] = std::move( pos_view );
    x += size.w;
  }
}

/****************************************************************
** CheckBoxView
*****************************************************************/
CheckBoxView::CheckBoxView( bool on ) : on_( on ) {}

Delta CheckBoxView::delta() const {
  return { .w = 11, .h = 11 };
}

void CheckBoxView::draw( rr::Renderer& renderer,
                         Coord         coord ) const {
  rr::Painter             painter = renderer.painter();
  static gfx::pixel const color =
      gfx::pixel{ .r = 0x3b, .g = 0x76, .b = 0x35, .a = 0xff };
  static gfx::pixel const border = color.shaded( 2 );
  static gfx::pixel const x_color =
      gfx::pixel{ .r = 0x22, .g = 0x22, .b = 0x22, .a = 0xff };
  painter.draw_solid_rect( rect( coord ), color );
  painter.draw_empty_rect( rect( coord ),
                           rr::Painter::e_border_mode::inside,
                           border );
  if( on_ ) {
    // This creates an anti-aliased x.
    {
      rr::Typer typer =
          renderer.typer( coord + Delta{ .w = 1, .h = 1 },
                          x_color.with_alpha( 64 ) );
      typer.write( 'x' );
    }
    {
      rr::Typer typer =
          renderer.typer( coord + Delta{ .w = 3, .h = 1 },
                          x_color.with_alpha( 64 ) );
      typer.write( 'x' );
    }
    {
      rr::Typer typer =
          renderer.typer( coord + Delta{ .w = 2, .h = 0 },
                          x_color.with_alpha( 64 ) );
      typer.write( 'x' );
    }
    {
      rr::Typer typer =
          renderer.typer( coord + Delta{ .w = 2, .h = 2 },
                          x_color.with_alpha( 64 ) );
      typer.write( 'x' );
    }
    // This is the main foreground x.
    {
      rr::Typer typer = renderer.typer(
          coord + Delta{ .w = 2, .h = 1 }, x_color );
      typer.write( 'x' );
    }
  }
}

bool CheckBoxView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    return false;
  on_ = !on_;
  return true;
}

/****************************************************************
** LabeledCheckBoxView
*****************************************************************/
LabeledCheckBoxView::LabeledCheckBoxView( string label, bool on )
  : HorizontalArrayView( HorizontalArrayView::align::middle ) {
  auto check_box = make_unique<CheckBoxView>( on );
  check_box_     = check_box.get();
  add_view( std::move( check_box ) );
  auto label_view = make_unique<TextView>( std::move( label ) );
  add_view( std::move( label_view ) );
  recompute_child_positions();
}

bool LabeledCheckBoxView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  // Clicking on the label is equivalent to clicking in the box.
  return check_box_->on_mouse_button( event );
}

/****************************************************************
** OkCancelAdapterView
*****************************************************************/
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

/****************************************************************
** OptionSelectItemView
*****************************************************************/
OptionSelectItemView::OptionSelectItemView( Option option )
  : active_{ e_option_active::inactive },
    enabled_( option.enabled ) {
  Delta const text_size =
      rendered_text_size_no_reflow( option.name ) +
      Delta{ .h = 2 };
  background_active_ = make_unique<SolidRectView>(
      gfx::pixel{ .r = 0xDB, .g = 0xC9, .b = 0x5A, .a = 255 } );
  background_inactive_ = make_unique<SolidRectView>(
      gfx::pixel{ .r = 0x58, .g = 0x3C, .b = 0x30, .a = 255 } );
  foreground_active_ = make_unique<OneLineStringView>(
      option.name,
      gfx::pixel{ .r = 0x42, .g = 0x2D, .b = 0x22, .a = 255 },
      text_size );
  if( enabled_ ) {
    foreground_inactive_ = make_unique<OneLineStringView>(
        option.name,
        gfx::pixel{ .r = 0xE4, .g = 0xC8, .b = 0x90, .a = 255 },
        text_size );
  } else {
    foreground_inactive_ = make_unique<OneLineStringView>(
        option.name,
        gfx::pixel{ .r = 0x80, .g = 0x80, .b = 0x80, .a = 255 },
        text_size );
  }

  Delta delta_active   = foreground_active_->delta();
  Delta delta_inactive = foreground_inactive_->delta();
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
      break;
  }
  SHOULD_NOT_BE_HERE;
}

void OptionSelectItemView::grow_to( W w ) {
  auto new_delta = background_active_->delta();
  if( new_delta.w > w )
    // we only grow here, not shrink.
    return;
  new_delta.w = w;
  background_active_->cast<SolidRectView>()->set_delta(
      new_delta );
  background_inactive_->cast<SolidRectView>()->set_delta(
      new_delta );
}

/****************************************************************
** OptionSelectView
*****************************************************************/
OptionSelectView::OptionSelectView(
    vector<OptionSelectItemView::Option> const& options,
    int initial_selection )
  : selected_{ initial_selection } {
  CHECK( options.size() > 0 );
  CHECK( selected_ >= 0 && selected_ < int( options.size() ) );

  Coord so_far{};
  W     min_width{ 0 };
  for( auto const& option : options ) {
    auto view   = make_unique<OptionSelectItemView>( option );
    auto width  = view->delta().w;
    auto height = view->delta().h;
    this->push_back( OwningPositionedView{
        .view = std::move( view ), .coord = so_far } );
    // `view` is no longer available here (moved from).
    so_far.y += height;
    min_width = std::max( min_width, width );
  }

  grow_to( min_width );

  // Now that we have the individual options populated we can
  // officially set a selected one.
  update_selected();
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

void OptionSelectView::update_selected() {
  CHECK( get_view( selected_ )->enabled() );
  for( int i = 0; i < count(); ++i )
    get_view( i )->set_active( e_option_active::inactive );
  get_view( selected_ )->set_active( e_option_active::active );
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
    case ::SDLK_k: // TODO: temporary?
      do {
        selected_ = cyclic_modulus( selected_ - 1, count() );
      } while( !get_view( selected_ )->enabled() );
      update_selected();
      return true;
    case ::SDLK_DOWN:
    case ::SDLK_KP_2:
    case ::SDLK_j: // TODO: temporary?
      do {
        selected_ = cyclic_modulus( selected_ + 1, count() );
      } while( !get_view( selected_ )->enabled() );
      update_selected();
      return true;
    default: break;
  }
  return false;
}

maybe<int> OptionSelectView::item_under_point(
    Coord coord ) const {
  int i = 0;
  for( PositionedViewConst pvc : *this ) {
    if( coord.is_inside( pvc.rect() ) ) return i;
    ++i;
  }
  return nothing;
}

bool OptionSelectView::enabled_option_at_point(
    Coord coord ) const {
  maybe<int> item = item_under_point( coord );
  if( !item.has_value() ) return false;
  return get_view( *item )->enabled();
}

bool OptionSelectView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  maybe<int> item = item_under_point( event.pos );
  if( !item.has_value() ) return false;
  if( get_view( *item )->enabled() ) {
    selected_ = *item;
    update_selected();
  }
  return true;
}

int OptionSelectView::get_selected() const {
  CHECK( get_view( selected_ )->enabled() );
  return selected_;
}

/****************************************************************
** FakeUnitView
*****************************************************************/
FakeUnitView::FakeUnitView( e_unit_type type, e_nation nation,
                            e_unit_orders orders )
  : CompositeSingleView(
        make_unique<SpriteView>( unit_attr( type ).tile ),
        Coord{} ),
    type_( type ),
    nation_( nation ),
    orders_( orders ) {}

void FakeUnitView::draw( rr::Renderer& renderer,
                         Coord         coord ) const {
  if( !unit_attr( type_ ).nat_icon_front ) {
    render_nationality_icon( renderer, coord, type_, nation_,
                             orders_ );
    this->CompositeSingleView::draw( renderer, coord );
  } else {
    this->CompositeSingleView::draw( renderer, coord );
    render_nationality_icon( renderer, coord, type_, nation_,
                             orders_ );
  }
}

/****************************************************************
** ClickableView
*****************************************************************/
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

/****************************************************************
** OnInputView
*****************************************************************/
OnInputView::OnInputView( unique_ptr<View>     view,
                          OnInputView::OnInput on_input )
  : CompositeSingleView( std::move( view ), Coord{} ),
    on_input_( std::move( on_input ) ) {}

bool OnInputView::input( input::event_t const& event ) {
  if( on_input_( event ) ) return true;
  return this->CompositeSingleView::input( event );
}

/****************************************************************
** BorderView
*****************************************************************/
BorderView::BorderView( unique_ptr<View> view, gfx::pixel color,
                        int padding, bool on_initially )
  : CompositeSingleView(
        std::move( view ),
        Coord{ .x = 1 + W{ padding }, .y = 1 + H{ padding } } ),
    color_( color ),
    on_( on_initially ),
    padding_( padding ) {}

Delta BorderView::delta() const {
  return this->CompositeSingleView::delta() +
         Delta{ .w = ( 1 + W{ padding_ } ) * 2,
                .h = ( 1 + H{ padding_ } ) * 2 };
}

void BorderView::draw( rr::Renderer& renderer,
                       Coord         coord ) const {
  this->CompositeSingleView::draw(
      renderer, coord + Delta{ .w = 1 + W{ padding_ },
                               .h = 1 + H{ padding_ } } );
  if( on_ )
    renderer.painter().draw_empty_rect(
        Rect::from( coord, delta() ),
        rr::Painter::e_border_mode::outside, color_ );
}

} // namespace rn::ui
