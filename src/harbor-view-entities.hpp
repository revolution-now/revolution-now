/****************************************************************
**harbor-view-entities.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: The various UI sections/entities in the european
*              harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "harbor-view-entities.rds.hpp"

// Revolution Now
#include "drag-drop.hpp"
#include "input.hpp"
#include "view.hpp"
#include "wait.hpp"

// refl
#include "refl/enum-map.hpp"

// C++ standard library
#include <memory>
#include <unordered_map>

/****************************************************************
** Macros
*****************************************************************/
#define CONVERT_ENTITY( to, from_entity )                       \
  UNWRAP_CHECK( to,                                             \
                refl::enum_from_integral<e_harbor_view_entity>( \
                    from_entity ) );

namespace gfx {
enum class e_resolution;
}

namespace rn {

struct IEuroAgent;
struct SS;
struct SSConst;
struct TS;
struct Player;

namespace ui {

struct View;

// TODO: Keep this generic and dedupe it with the one in colony
// view. Also, need to figure out how its methods override (or
// not) the methods in ui::Object that accept the same events.
struct AwaitView {
  virtual ~AwaitView() = default;

  virtual wait<> perform_click(
      input::mouse_button_event_t const& ) {
    return make_wait<>();
  }

  // Returns whether or not it was handled.
  virtual wait<bool> perform_key( input::key_event_t const& ) {
    return make_wait<bool>( false );
  }
};

} // namespace ui

struct harbor_view_exit_interrupt : std::exception {};

/****************************************************************
** HarborSubView
*****************************************************************/
// FIXME: try to dedupe this with the one in the colony view.
class HarborSubView
  : public IDraggableObjectsView<HarborDraggableObject>,
    public ui::AwaitView {
 public:
  HarborSubView( SS& ss, TS& ts, Player& player );

  // All HarborSubView's will also be unspecified subclassess of
  // ui::View.
  virtual ui::View& view() noexcept             = 0;
  virtual ui::View const& view() const noexcept = 0;

  // Implement IDraggableObjectsView.
  virtual maybe<
      PositionedDraggableSubView<HarborDraggableObject>>
  view_here( Coord ) override {
    return PositionedDraggableSubView<HarborDraggableObject>{
      this, Coord{} };
  }

  // Implement IDraggableObjectsView.
  virtual maybe<DraggableObjectWithBounds<HarborDraggableObject>>
  object_here( Coord const& /*where*/ ) const override {
    return nothing;
  }

  // This will update any internal state held inside the view
  // that needs to be recomputed if any state external to the
  // view changes. Note: this assumes that the screen size has
  // not been changed. If the screen size has not been changed,
  // then we need to do a full recompositing instead. This is not
  // supposed to be called directly, but should only be called
  // via the HarborViewComposited::update method.
  virtual void update_this_and_children() {}

 protected:
  SS& ss_;
  TS& ts_;
  Player& player_;
  Player& colonial_player_;
  IEuroAgent& agent_;
};

template<typename T>
struct PositionedHarborSubView {
  ui::OwningPositionedView owned;
  HarborSubView* harbor = nullptr;
  T* actual             = nullptr;
};

void update_harbor_view( SSConst const& ss );

struct HarborViewComposited {
  Delta canvas_size;
  std::unique_ptr<HarborSubView> top_level;
  refl::enum_map<e_harbor_view_entity, HarborSubView*> entities;

  void update();
};

HarborViewComposited recomposite_harbor_view(
    SS& ss, TS& ts, Player& player,
    gfx::e_resolution resolution );

} // namespace rn
