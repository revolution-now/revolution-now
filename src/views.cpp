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
#include "render.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "unit-flag.hpp"
#include "visibility.hpp"

// config
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/natives.hpp"

// render
#include "render/extra.hpp"
#include "render/itextometer.hpp"
#include "render/painter.hpp"
#include "render/renderer.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/lambda.hpp"
#include "base/math.hpp"

// C++ standard library
#include <chrono>
#include <numeric>

using namespace std;

namespace rn::ui {

namespace {

using ::gfx::point;
using ::gfx::size;

template<typename Res, typename... T>
std::vector<Res> params_to_vector( T&&... ts ) {
  std::vector<Res> res;
  res.reserve( sizeof...( T ) );
  ( res.push_back( std::forward<T>( ts ) ), ... );
  return res;
}

} // namespace

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
                          Coord coord ) const {
  // Draw each of the sub views, by augmenting its origin (which
  // is relative to the origin of the parent by the origin that
  // we have been given.
  for( auto [view, view_coord] : *this ) {
    if( disabled() || view->disabled() ) {
      SCOPED_RENDERER_MOD_OR( painter_mods.desaturate, true );
      view->draw( renderer, coord + ( view_coord - Coord() ) );
    } else {
      view->draw( renderer, coord + ( view_coord - Coord() ) );
    }
  }
}

Delta CompositeView::delta() const {
  auto uni0n = L2( _1.uni0n( _2.view->bounds( _2.coord ) ) );
  auto rect  = accumulate( begin(), end(), Rect{}, uni0n );
  return { .w = rect.w, .h = rect.h };
}

maybe<PositionedView> CompositeView::first_view_under_cursor(
    Coord pos ) {
  for( PositionedView pview : *this )
    if( pos.is_inside( pview.rect() ) ) return pview;
  return nothing;
}

maybe<PositionedViewConst>
CompositeView::first_view_under_cursor( Coord pos ) const {
  for( PositionedViewConst pview : *this )
    if( pos.is_inside( pview.rect() ) ) return pview;
  return nothing;
}

bool CompositeView::dispatch_mouse_event(
    input::event_t const& event ) {
  if( disabled() ) return false;
  UNWRAP_CHECK( pos, input::mouse_position( event ) );
  for( auto p_view : *this ) {
    if( p_view.view->disabled() ) continue;
    if( pos.is_inside( p_view.rect() ) ) {
      auto new_event =
          mouse_origin_moved_by( event, p_view.coord - Coord{} );
      if( p_view.view->input( new_event ) ) //
        return true;
    }
  }
  return false;
}

bool CompositeView::on_key( input::key_event_t const& event ) {
  if( disabled() ) return false;
  for( auto p_view : *this ) {
    if( p_view.view->disabled() ) continue;
    if( p_view.view->input( event ) ) return true;
  }
  return false;
}

bool CompositeView::on_wheel(
    input::mouse_wheel_event_t const& event ) {
  if( disabled() ) return false;
  return dispatch_mouse_event( event );
}

void CompositeView::send_mouse_enter_leave_events(
    input::mouse_move_event_t const& event ) {
  if( disabled() ) return;
  auto to   = event.pos;
  auto from = event.prev;
  // First check for and send out any on_mouse_leave or
  // on_mouse_enter events that happen among the sub-views.
  for( auto p_view : *this ) {
    if( p_view.view->disabled() ) continue;
    if( from.is_inside( p_view.rect() ) &&
        !to.is_inside( p_view.rect() ) )
      p_view.view->on_mouse_leave(
          from.with_new_origin( p_view.rect().upper_left() ) );
    if( !from.is_inside( p_view.rect() ) &&
        to.is_inside( p_view.rect() ) )
      p_view.view->on_mouse_enter(
          from.with_new_origin( p_view.rect().upper_left() ) );
  }
}

bool CompositeView::on_mouse_move(
    input::mouse_move_event_t const& event ) {
  send_mouse_enter_leave_events( event );
  return dispatch_mouse_event( event );
}

bool CompositeView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  return dispatch_mouse_event( event );
}

bool CompositeView::on_mouse_drag(
    input::mouse_drag_event_t const& event ) {
  if( disabled() ) return false;
  send_mouse_enter_leave_events( event );

  maybe<PositionedView> const origin_pview =
      first_view_under_cursor( event.state.origin );
  if( !origin_pview.has_value() ) return false;

  // We don't use dispatch_mouse_event here because that will se-
  // lect the view that is under the mouse position, which we
  // don't want to do in the case of drag events. For drag events
  // all that matters is the view under the origin, namely the
  // one where the drag started. Moreover, that view will con-
  // tinue to receive drag events even if the cursor leaves the
  // view during the drag.
  input::event_t const new_event = mouse_origin_moved_by(
      event, origin_pview->coord - Coord{} );
  if( origin_pview->view->disabled() ) return false;
  return origin_pview->view->input( new_event );
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
  if( disabled() ) return;
  for( auto p_view : *this ) {
    if( p_view.view->disabled() ) continue;
    if( from.is_inside( p_view.rect() ) )
      p_view.view->on_mouse_leave(
          from.with_new_origin( p_view.rect().upper_left() ) );
  }
}

void CompositeView::on_mouse_enter( Coord to ) {
  if( disabled() ) return;
  for( auto p_view : *this ) {
    if( p_view.view->disabled() ) continue;
    if( to.is_inside( p_view.rect() ) )
      p_view.view->on_mouse_enter(
          to.with_new_origin( p_view.rect().upper_left() ) );
  }
}

PositionedView CompositeView::at( int idx ) {
  View* view{ const_cast<View*>( mutable_at( idx ).get() ) };
  return { view, pos_of( idx ) };
}

PositionedViewConst CompositeView::at( int idx ) const {
  auto* this_ = const_cast<CompositeView*>( this );
  View const* view{ this_->mutable_at( idx ).get() };
  return { view, pos_of( idx ) };
}

/****************************************************************
** RefView
*****************************************************************/
void RefView::advance_state() { return view_.advance_state(); }

void RefView::draw( rr::Renderer& renderer,
                    Coord const coord ) const {
  return view_.draw( renderer, coord );
}

Delta RefView::delta() const { return view_.delta(); }

bool RefView::on_key( input::key_event_t const& event ) {
  return view_.on_key( event );
}

bool RefView::on_wheel(
    input::mouse_wheel_event_t const& event ) {
  return view_.on_wheel( event );
}

bool RefView::on_mouse_move(
    input::mouse_move_event_t const& event ) {
  return view_.on_mouse_move( event );
}

bool RefView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  return view_.on_mouse_button( event );
}

bool RefView::on_mouse_drag(
    input::mouse_drag_event_t const& event ) {
  return view_.on_mouse_drag( event );
}

bool RefView::on_win_event( input::win_event_t const& event ) {
  return view_.on_win_event( event );
}

void RefView::on_mouse_leave( Coord const from ) {
  return view_.on_mouse_leave( from );
}

void RefView::on_mouse_enter( Coord to ) {
  return view_.on_mouse_enter( to );
}

Rect RefView::bounds( Coord position ) const {
  return view_.bounds( position );
}

bool RefView::input( input::event_t const& e ) {
  return view_.input( e );
}

bool RefView::needs_padding() const {
  return view_.needs_padding();
}

bool RefView::on_resolution_event(
    input::resolution_event_t const& event ) {
  return view_.on_resolution_event( event );
}

bool RefView::on_cheat_event(
    input::cheat_event_t const& event ) {
  return view_.on_cheat_event( event );
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
                          Coord coord ) const {
  renderer.painter().draw_solid_rect( bounds( coord ), color_ );
}

/****************************************************************
** OneLineStringView
*****************************************************************/
// NOTE: If you add reflow info to this constructor, don't forget
// to add it into the calculation of the text size as well.
OneLineStringView::OneLineStringView(
    rr::ITextometer const& textometer, string msg,
    gfx::pixel color, Delta size_override )
  : msg_( std::move( msg ) ),
    view_size_( size_override ),
    text_size_( rendered_text_size_no_reflow(
        textometer, rr::TextLayout{}, msg ) ),
    color_( color ) {}

OneLineStringView::OneLineStringView(
    rr::ITextometer const& textometer, string msg,
    gfx::pixel color )
  : OneLineStringView(
        textometer, msg, color,
        rendered_text_size_no_reflow(
            textometer, rr::TextLayout{}, msg ) ) {}

Delta OneLineStringView::delta() const { return view_size_; }

void OneLineStringView::draw( rr::Renderer& renderer,
                              Coord coord ) const {
  int const start_offset = ( view_size_.h - text_size_.h ) / 2;
  TextMarkupInfo const markup_info{ .normal = color_ };
  render_text_markup(
      renderer, coord + Delta{ .h = start_offset }, e_font{},
      rr::TextLayout{}, markup_info, msg_ );
}

/****************************************************************
** TextView
*****************************************************************/
TextView::TextView( rr::ITextometer const& textometer,
                    std::string msg,
                    TextMarkupInfo const& m_info,
                    TextReflowInfo const& r_info )
  : msg_( std::move( msg ) ),
    text_size_{ rendered_text_size( textometer, rr::TextLayout{},
                                    r_info, msg_ ) },
    markup_info_( m_info ),
    reflow_info_( r_info ) {}

TextView::TextView( rr::ITextometer const& textometer,
                    std::string msg )
  : TextView( textometer, std::move( msg ),
              default_text_markup_info(),
              default_text_reflow_info() ) {}

Delta TextView::delta() const { return text_size_; }

void TextView::draw( rr::Renderer& renderer,
                     Coord coord ) const {
  render_text_markup_reflow( renderer, coord, font::standard(),
                             markup_info_, reflow_info_, msg_ );
}

/****************************************************************
** ButtonBaseView
*****************************************************************/
ButtonBaseView::ButtonBaseView(
    rr::ITextometer const& textometer, string label )
  : ButtonBaseView( textometer, std::move( label ),
                    e_type::standard ) {}

ButtonBaseView::ButtonBaseView(
    rr::ITextometer const& textometer, string label,
    e_type type )
  : ButtonBaseView(
        textometer, label,
        rendered_text_size( textometer, rr::TextLayout{},
                            /*reflow_info=*/{}, label )
                    .round_up( Delta{ .w = 8, .h = 8 } ) /
                Delta{ .w = 8, .h = 8 } +
            Delta{ .w = 2 } + Delta{ .h = 1 },
        type ) {}

ButtonBaseView::ButtonBaseView(
    rr::ITextometer const& textometer, string label,
    Delta size_in_blocks )
  : ButtonBaseView( textometer, std::move( label ),
                    size_in_blocks, e_type::standard ) {}

ButtonBaseView::ButtonBaseView(
    rr::ITextometer const& textometer, string label,
    Delta size_in_blocks, e_type type )
  : label_( std::move( label ) ),
    type_( type ),
    size_in_pixels_( size_in_blocks * Delta{ .w = 8, .h = 8 } ),
    text_size_in_pixels_(
        rendered_text_size( textometer, rr::TextLayout{},
                            /*reflow_info=*/{}, label_ ) ) {}

Delta ButtonBaseView::delta() const { return size_in_pixels_; }

void ButtonBaseView::draw( rr::Renderer& renderer,
                           Coord coord ) const {
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
  render_rect_of_sprites_with_border(
      renderer, Coord::from_gfx( where ),
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
                      rr::TextLayout{}, markup_info, label_ );
}

void ButtonBaseView::render_pressed( rr::Renderer& renderer,
                                     gfx::point where ) const {
  render_rect_of_sprites_with_border(
      renderer, Coord::from_gfx( where ),
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
                      rr::TextLayout{}, markup_info, label_ );
}

void ButtonBaseView::render_unpressed( rr::Renderer& renderer,
                                       gfx::point where ) const {
  render_rect_of_sprites_with_border(
      renderer, Coord::from_gfx( where ),
      size_in_pixels_ / Delta{ .w = 8, .h = 8 }, //
      e_tile::button_up_mm, e_tile::button_up_um,
      e_tile::button_up_lm, e_tile::button_up_ml,
      e_tile::button_up_mr, e_tile::button_up_ul,
      e_tile::button_up_ur, e_tile::button_up_ll,
      e_tile::button_up_lr );

  auto markup_info =
      TextMarkupInfo{ gfx::pixel::wood().shaded( 5 ),
                      /*highlight=*/{} };

  point const text_position = centered(
      text_size_in_pixels_,
      Rect::from( Coord::from_gfx( where ), size_in_pixels_ ) );
  render_text_markup( renderer, text_position, font::standard(),
                      rr::TextLayout{}, markup_info, label_ );
}

void ButtonBaseView::render_hover( rr::Renderer& renderer,
                                   gfx::point where ) const {
  render_rect_of_sprites_with_border(
      renderer, Coord::from_gfx( where ),
      size_in_pixels_ / Delta{ .w = 8, .h = 8 }, //
      e_tile::button_up_mm, e_tile::button_up_um,
      e_tile::button_up_lm, e_tile::button_up_ml,
      e_tile::button_up_mr, e_tile::button_up_ul,
      e_tile::button_up_ur, e_tile::button_up_ll,
      e_tile::button_up_lr );

  auto markup_info =
      TextMarkupInfo{ gfx::pixel::banana(), /*highlight=*/{} };

  point const text_position = centered(
      text_size_in_pixels_,
      Rect::from( Coord::from_gfx( where ), size_in_pixels_ ) );
  render_text_markup( renderer, text_position, font::standard(),
                      rr::TextLayout{}, markup_info, label_ );
}

/****************************************************************
** SpriteView
*****************************************************************/
Delta SpriteView::delta() const {
  return Delta{ .w = 1, .h = 1 } * sprite_size( tile_ );
}

void SpriteView::draw( rr::Renderer& renderer,
                       Coord coord ) const {
  render_sprite( renderer, coord, tile_ );
}

/****************************************************************
** LineEditorView
*****************************************************************/
LineEditorView::LineEditorView(
    rr::ITextometer const& textometer, e_font font,
    W pixels_wide, OnChangeFunc on_change, gfx::pixel fg,
    gfx::pixel bg, string_view prompt, string_view initial_text )
  : textometer_( textometer ),
    prompt_{ prompt },
    fg_{ fg },
    bg_{ bg },
    font_{ font },
    on_change_{ std::move( on_change ) },
    line_editor_( string( initial_text ), initial_text.size() ),
    input_view_( 1 ),
    current_rendering_{},
    cursor_width_{} {
  cursor_width_ = 1;
  size const pixel_size{ .w = pixels_wide,
                         .h = textometer_.font_height() };
  // This doesn't work precisely because 1) the font may not be
  // fixed width, and 2) the char width is just an average.
  input_view_ = LineEditorInputView{ pixel_size.w / 6 };
  update_visible_string();
}

LineEditorView::LineEditorView(
    rr::ITextometer const& textometer, int chars_wide,
    string_view initial_text, OnChangeFunc on_change )
  : LineEditorView( textometer, font::standard(), 6 * chars_wide,
                    std::move( on_change ), gfx::pixel::wood(),
                    gfx::pixel::banana(), /*prompt=*/"",
                    initial_text ) {}

LineEditorView::LineEditorView(
    rr::ITextometer const& textometer, int chars_wide,
    string_view initial_text )
  : LineEditorView( textometer, chars_wide, initial_text,
                    []( auto const& ) {} ) {}

Delta LineEditorView::delta() const {
  return gfx::size{ .w = input_view_.width() * 6 + 4,
                    .h = 8 + 4 };
}

void LineEditorView::render_background( rr::Renderer& renderer,
                                        Rect const& r ) const {
  renderer.painter().draw_solid_rect(
      Rect::from( r.upper_left(), r.delta() ), bg_ );
}

// Implement Object
void LineEditorView::draw( rr::Renderer& renderer,
                           Coord coord ) const {
  render_background( renderer, bounds( coord ) );
  auto all_chars  = prompt_ + current_rendering_;
  rr::Typer typer = renderer.typer();
  typer.set_color( fg_ );
  gfx::size text_size =
      typer.dimensions_for_line( current_rendering_ );
  Y text_pos_y =
      centered( Delta::from_gfx( text_size ), bounds( coord ) )
          .y;
  typer.set_position( { .x = coord.x + 1, .y = text_pos_y } );
  typer.write( all_chars );

  auto rel_pos = input_view_.rel_pos( line_editor_.pos() ) +
                 int( prompt_.size() );
  CHECK( rel_pos <= int( all_chars.size() ) );
  string string_up_to_cursor( all_chars.begin(),
                              all_chars.begin() + rel_pos );
  W rel_cursor_pixels =
      rel_pos == 0
          ? W{ 0 }
          // The rendered text might have width 1 in this case.
          : Delta::from_gfx( typer.dimensions_for_line(
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
                          maybe<int> cursor_pos ) {
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
    rr::ITextometer const& textometer, string_view msg,
    wait_promise<>& on_close ) {
  TextMarkupInfo const& m_info = default_text_markup_info();
  TextReflowInfo const& r_info = default_text_reflow_info();
  unique_ptr<TextView> tview   = make_unique<TextView>(
      textometer, string( msg ), m_info, r_info );
  return make_unique<PlainMessageBoxView>( std::move( tview ),
                                           on_close );
}

PlainMessageBoxView::PlainMessageBoxView(
    unique_ptr<TextView> tview, wait_promise<>& on_close )
  : CompositeSingleView( std::move( tview ), Coord{} ),
    on_close_( on_close ) {}

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
PaddingView::PaddingView( std::unique_ptr<View> view,
                          int const pixels )
  : PaddingView( std::move( view ), pixels, true, true, true,
                 true ) {}

PaddingView::PaddingView( std::unique_ptr<View> view,
                          int const pixels, bool const l,
                          bool const r, bool const u,
                          bool const d )
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

bool PaddingView::can_pad_immediate_children() const {
  return false;
}

/****************************************************************
** ButtonView
*****************************************************************/
ButtonView::ButtonView( rr::ITextometer const& textometer,
                        string label, OnClickFunc on_click )
  : ButtonBaseView( textometer, std::move( label ) ),
    on_click_( std::move( on_click ) ) {
  set_state( button_state::up );
}

ButtonView::ButtonView( rr::ITextometer const& textometer,
                        string label, Delta size_in_blocks,
                        OnClickFunc on_click )
  : ButtonBaseView( textometer, std::move( label ),
                    size_in_blocks ),
    on_click_( std::move( on_click ) ) {
  set_state( button_state::up );
}

bool ButtonView::on_mouse_move(
    input::mouse_move_event_t const& event ) {
  if( state() != button_state::disabled ) {
    switch( state() ) {
      case button_state::down:
        break;
      case button_state::up:
        set_state( event.l_mouse_down ? button_state::down
                                      : button_state::hover );
        break;
      case button_state::disabled:
        break;
      case button_state::hover:
        break;
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
      default:
        break;
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

OkCancelView2::OkCancelView2(
    rr::ITextometer const& textometer ) {
  auto ok_button = make_unique<ButtonView>(
      textometer, "OK", ok_cancel_button_size_blocks,
      [this] { clicks_.send( e_ok_cancel::ok ); } );
  auto cancel_button = make_unique<ButtonView>(
      textometer, "Cancel", ok_cancel_button_size_blocks,
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
    case ::SDLK_ESCAPE:
      cancel_ref_->click();
      return true;
    default:
      break;
  }
  return false;
}

/****************************************************************
** OkCancelView (deprecated)
*****************************************************************/
OkCancelView::OkCancelView( rr::ITextometer const& textometer,
                            ButtonView::OnClickFunc on_ok,
                            ButtonView::OnClickFunc on_cancel ) {
  auto ok_button = make_unique<ButtonView>(
      textometer, "OK", ok_cancel_button_size_blocks,
      std::move( on_ok ) );
  auto cancel_button = make_unique<ButtonView>(
      textometer, "Cancel", ok_cancel_button_size_blocks,
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

bool OkCancelView::on_key( input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down ) return false;
  // It's a key down.
  switch( event.keycode ) {
    case ::SDLK_ESCAPE:
      cancel_ref_->click();
      return true;
    default:
      break;
  }
  return false;
}

/****************************************************************
** OkButtonView
*****************************************************************/
OkButtonView::OkButtonView( rr::ITextometer const& textometer,
                            ButtonView::OnClickFunc on_ok )
  : CompositeSingleView(
        make_unique<ButtonView>( textometer, "OK",
                                 ok_cancel_button_size_blocks,
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
    auto size  = view->delta();
    X x{ 0 };
    switch( alignment_ ) {
      case align::left:
        x = 0;
        break;
      case align::right:
        x = 0 + ( max_width - size.w );
        break;
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
    auto size  = view->delta();
    Y y{ 0 };
    switch( alignment_ ) {
      case align::up:
        y = 0;
        break;
      case align::down:
        y = 0 + ( max_height - size.h );
        break;
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
** RadioButtonGroup
*****************************************************************/
void RadioButtonGroup::add( IRadioButton& button ) {
  buttons_.push_back( &button );
}

void RadioButtonGroup::on_child_clicked(
    IRadioButton& clicked ) {
  for( IRadioButton* const button : buttons_ )
    button->turn_off();
  clicked.turn_on();
}

void RadioButtonGroup::set( int const idx ) {
  CHECK_GE( idx, 0 );
  CHECK_LT( idx, ssize( buttons_ ) );
  on_child_clicked( *buttons_[idx] );
}

maybe<int> RadioButtonGroup::get_selected() const {
  maybe<int> sel;
  for( int idx = 0; idx < ssize( buttons_ ); ++idx ) {
    IRadioButton const& button = *buttons_[idx];
    if( !button.on() ) continue;
    CHECK( !sel.has_value(),
           "cannot have multiple radio buttons selected." );
    sel = idx;
  }
  return sel;
}

/****************************************************************
** RadioButtonView
*****************************************************************/
RadioButtonView::RadioButtonView( RadioButtonGroup& group )
  : group_( group ), on_( false ) {}

Delta RadioButtonView::delta() const {
  return { .w = 11, .h = 11 };
}

void RadioButtonView::draw( rr::Renderer& renderer,
                            Coord const coord ) const {
  render_sprite( renderer, coord, e_tile::radio_back );
  if( on_ )
    render_sprite( renderer, coord, e_tile::radio_check );
}

bool RadioButtonView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_down )
    return false;
  // This callback should turn off all radio buttons in the group
  // and turn this one on, in accordance to how radio buttons are
  // supposed to work.
  group_.on_child_clicked( *this );
  CHECK( on_ );
  return true;
}

/****************************************************************
** LabeledRadioButtonView
*****************************************************************/
LabeledRadioButtonView::LabeledRadioButtonView(
    RadioButtonGroup& group, unique_ptr<View> label )
  : HorizontalArrayView( HorizontalArrayView::align::middle ) {
  auto check_box = make_unique<RadioButtonView>( group );
  radio_button_  = check_box.get();
  add_view( std::move( check_box ) );
  // Add some spacing between the box and text.
  add_view( make_unique<EmptyView>( Delta{ .w = 1, .h = 1 } ) );
  add_view( std::move( label ) );
  recompute_child_positions();
  CHECK( radio_button_ );
}

bool LabeledRadioButtonView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  // Clicking on the label is equivalent to clicking in the box.
  return radio_button_->on_mouse_button( event );
}

/****************************************************************
** TextLabeledRadioButtonView
*****************************************************************/
TextLabeledRadioButtonView::TextLabeledRadioButtonView(
    RadioButtonGroup& group, rr::ITextometer const& textometer,
    string label )
  : LabeledRadioButtonView(
        group, make_unique<TextView>( textometer,
                                      std::move( label ) ) ) {}

/****************************************************************
** CheckBoxView
*****************************************************************/
CheckBoxView::CheckBoxView( bool on ) : on_( on ) {}

Delta CheckBoxView::delta() const {
  return { .w = 11, .h = 11 };
}

void CheckBoxView::draw( rr::Renderer& renderer,
                         Coord const coord ) const {
  render_sprite( renderer, coord, e_tile::checkbox_back );
  if( on_ )
    render_sprite( renderer, coord, e_tile::checkbox_check );
}

bool CheckBoxView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_down )
    return false;
  on_ = !on_;
  return true;
}

/****************************************************************
** LabeledCheckBoxView
*****************************************************************/
LabeledCheckBoxView::LabeledCheckBoxView( unique_ptr<View> label,
                                          bool on )
  : HorizontalArrayView( HorizontalArrayView::align::middle ) {
  auto check_box = make_unique<CheckBoxView>( on );
  check_box_     = check_box.get();
  add_view( std::move( check_box ) );
  // Add some spacing between the box and text.
  add_view( make_unique<EmptyView>( Delta{ .w = 1, .h = 1 } ) );
  add_view( std::move( label ) );
  recompute_child_positions();
}

bool LabeledCheckBoxView::on_mouse_button(
    input::mouse_button_event_t const& event ) {
  // Clicking on the label is equivalent to clicking in the box.
  return check_box_->on_mouse_button( event );
}

/****************************************************************
** TextLabeledCheckBoxView
*****************************************************************/
TextLabeledCheckBoxView::TextLabeledCheckBoxView(
    rr::ITextometer const& textometer, string label, bool on )
  : LabeledCheckBoxView(
        make_unique<TextView>( textometer, std::move( label ) ),
        on ) {}

/****************************************************************
** OkCancelAdapterView
*****************************************************************/
OkCancelAdapterView::OkCancelAdapterView(
    rr::ITextometer const& textometer, unique_ptr<View> view,
    OnClickFunc on_click )
  : VerticalArrayView(
        params_to_vector<unique_ptr<View>>(
            std::move( view ),
            make_unique<OkCancelView>(
                textometer, /*on_ok=*/
                [on_click]() { on_click( e_ok_cancel::ok ); },
                /*on_cancel=*/
                [on_click]() {
                  on_click( e_ok_cancel::cancel );
                } ) ),
        VerticalArrayView::align::center ) {}

/****************************************************************
** OptionSelectItemView
*****************************************************************/
OptionSelectItemView::OptionSelectItemView(
    rr::ITextometer const& textometer, Option option )
  : active_{ e_option_active::inactive },
    enabled_( option.enabled ) {
  Delta const text_size =
      rendered_text_size_no_reflow( textometer, rr::TextLayout{},
                                    option.name ) +
      Delta{ .h = 4 };
  background_active_ = make_unique<SolidRectView>(
      config_ui.dialog_text.selected_background );
  background_inactive_ =
      make_unique<SolidRectView>( gfx::pixel{ .a = 0 } );
  foreground_active_ = make_unique<OneLineStringView>(
      textometer, option.name, config_ui.dialog_text.normal,
      text_size );
  if( enabled_ ) {
    foreground_inactive_ = make_unique<OneLineStringView>(
        textometer, option.name, config_ui.dialog_text.normal,
        text_size );
  } else {
    foreground_inactive_ = make_unique<OneLineStringView>(
        textometer, option.name, config_ui.dialog_text.disabled,
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
        case e_option_active::active:
          return background_active_;
        case e_option_active::inactive:
          return background_inactive_;
      }
      break;
    case 1:
      switch( active_ ) {
        case e_option_active::active:
          return foreground_active_;
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
    rr::ITextometer const& textometer,
    vector<OptionSelectItemView::Option> const& options,
    maybe<int> initial_selection )
  : selected_{ initial_selection } {
  CHECK( options.size() > 0 );
  if( selected_.has_value() ) {
    CHECK( *selected_ >= 0 &&
           *selected_ < int( options.size() ) );
  }

  if( !selected_.has_value() || !options[*selected_].enabled ) {
    // We either don't have a selected item or the item that was
    // requested to be selected is not enabled. So we will deal
    // with that by just selecting the first enabled item if
    // there is one.
    for( int i = 0; i < int( options.size() ); ++i ) {
      if( !options[i].enabled ) continue;
      selected_ = i;
      break;
    }
  }

  Coord so_far{};
  W min_width{ 0 };
  for( auto const& option : options ) {
    auto view =
        make_unique<OptionSelectItemView>( textometer, option );
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
  for( int i = 0; i < count(); ++i )
    get_view( i )->set_active( e_option_active::inactive );
  if( selected_.has_value() ) {
    CHECK( get_view( *selected_ )->enabled() );
    get_view( *selected_ )
        ->set_active( e_option_active::active );
  }
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
      if( selected_.has_value() ) {
        do {
          selected_ =
              base::cyclic_modulus( *selected_ - 1, count() );
        } while( !get_view( *selected_ )->enabled() );
        update_selected();
      }
      return true;
    case ::SDLK_DOWN:
    case ::SDLK_KP_2:
    case ::SDLK_j: // TODO: temporary?
      if( selected_.has_value() ) {
        do {
          selected_ =
              base::cyclic_modulus( *selected_ + 1, count() );
        } while( !get_view( *selected_ )->enabled() );
        update_selected();
      }
      return true;
    default:
      break;
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

maybe<int> OptionSelectView::get_selected() const {
  if( selected_.has_value() ) {
    CHECK( get_view( *selected_ )->enabled() );
  }
  return selected_;
}

/****************************************************************
** FakeUnitView
*****************************************************************/
FakeUnitView::FakeUnitView( e_unit_type type, e_player player,
                            unit_orders const& orders )
  : type_( type ), player_( player ), orders_( orders ) {}

Delta FakeUnitView::delta() const {
  return { .w = 32, .h = 32 };
}

void FakeUnitView::draw( rr::Renderer& renderer,
                         Coord coord ) const {
  UnitFlagRenderInfo const flag_info =
      euro_unit_type_flag_info( type_, orders_, player_ );
  render_unit_type( renderer, coord, type_,
                    UnitRenderOptions{ .flag = flag_info } );
}

/****************************************************************
** FakeNativeUnitView
*****************************************************************/
FakeNativeUnitView::FakeNativeUnitView(
    e_native_unit_type const type, e_tribe const tribe )
  : type_( type ), tribe_( tribe ) {}

Delta FakeNativeUnitView::delta() const {
  return { .w = 32, .h = 32 };
}

void FakeNativeUnitView::draw( rr::Renderer& renderer,
                               Coord coord ) const {
  UnitFlagRenderInfo const flag_info =
      native_unit_type_flag_info( type_, tribe_,
                                  UnitFlagOptions{} );
  render_unit_type( renderer, coord, type_,
                    UnitRenderOptions{ .flag = flag_info } );
}

/****************************************************************
** RenderedColonyView
*****************************************************************/
RenderedColonyView::RenderedColonyView( SSConst const& ss,
                                        Colony const& colony )
  : ss_( ss ),
    colony_( colony ),
    size_( sprite_size( houses_tile_for_colony( colony ) ) ),
    viz_( make_unique<VisibilityEntire>( ss ) ) {}

RenderedColonyView::~RenderedColonyView() = default;

Delta RenderedColonyView::delta() const { return size_; }

void RenderedColonyView::draw( rr::Renderer& renderer,
                               Coord const where ) const {
  render_colony( renderer, where, *viz_, colony_.location, ss_,
                 colony_,
                 ColonyRenderOptions{ .render_name       = false,
                                      .render_population = true,
                                      .render_flag = true } );
}

/****************************************************************
** RenderedDwellingView
*****************************************************************/
RenderedDwellingView::RenderedDwellingView(
    SSConst const& ss, Dwelling const& dwelling )
  : ss_( ss ),
    dwelling_( dwelling ),
    size_( sprite_size( tile_for_dwelling( ss, dwelling ) ) ),
    viz_( make_unique<VisibilityEntire>( ss ) ) {}

RenderedDwellingView::~RenderedDwellingView() = default;

Delta RenderedDwellingView::delta() const { return size_; }

void RenderedDwellingView::draw( rr::Renderer& renderer,
                                 Coord const where ) const {
  render_dwelling( renderer, where, *viz_, /*map_tile=*/point{},
                   ss_, dwelling_ );
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
  if( event.buttons == input::e_mouse_button_event::left_down )
    on_click_();
  return true;
}

/****************************************************************
** OnInputView
*****************************************************************/
OnInputView::OnInputView( unique_ptr<View> view,
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
                       Coord coord ) const {
  this->CompositeSingleView::draw(
      renderer, coord + Delta{ .w = 1 + W{ padding_ },
                               .h = 1 + H{ padding_ } } );
  if( on_ )
    renderer.painter().draw_empty_rect(
        Rect::from( coord, delta() ),
        rr::Painter::e_border_mode::outside, color_ );
}

} // namespace rn::ui
