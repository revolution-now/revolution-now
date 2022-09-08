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
#include "dragdrop.hpp"
#include "input.hpp"
#include "wait.hpp"

// base
#include "base/any-util.hpp"

// C++ standard library
#include <memory>
#include <unordered_map>

/****************************************************************
** Macros
*****************************************************************/
// This will take a std::any and extract either a
// HarborDraggableObject_t from it or any of its alternative
// types, and check-fail if it doesn't contain any of them.
#define UNWRAP_DRAGGABLE( v, std_any )                         \
  HarborDraggableObject_t const v =                            \
      base::extract_variant_from_any<HarborDraggableObject_t>( \
          std_any );

#define CONVERT_ENTITY( to, from_entity )                       \
  UNWRAP_CHECK( to,                                             \
                refl::enum_from_integral<e_harbor_view_entity>( \
                    from_entity ) );

namespace rn {

struct SS;
struct SSConst;
struct TS;
struct Player;

namespace ui {

struct View;

// TODO: Keep this generic and dedupe it with the one in colony
// view.
struct AwaitView {
  virtual ~AwaitView() = default;

  virtual wait<> perform_click(
      input::mouse_button_event_t const& ) {
    return make_wait<>();
  }
};

} // namespace ui

/****************************************************************
** HarborSubView
*****************************************************************/
class HarborSubView : public IDraggableObjectsView,
                      public ui::AwaitView {
 public:
  HarborSubView( SS& ss, TS& ts, Player& player )
    : ss_( ss ), ts_( ts ), player_( player ) {}

  // All HarborSubView's will also be unspecified subclassess of
  // ui::View.
  virtual ui::View&       view() noexcept       = 0;
  virtual ui::View const& view() const noexcept = 0;

  // Implement IDraggableObjectsView.
  virtual maybe<PositionedDraggableSubView> view_here(
      Coord ) override {
    return PositionedDraggableSubView{ this, Coord{} };
  }

  // Implement IDraggableObjectsView.
  virtual maybe<DraggableObjectWithBounds> object_here(
      Coord const& /*where*/ ) const override {
    return nothing;
  }

  // This will update any internal state held inside the view
  // that needs to be recomputed if any state external to the
  // view changes. Note: this assumes that the screen size has
  // not been changed. If the screen size has not been changed,
  // then we need to do a full recompositing instead. This is not
  // supposed to be called directly, but should only be called
  // via the HarborViewComposited::update method.
  virtual void update_this_and_children();

 protected:
  SS&     ss_;
  TS&     ts_;
  Player& player_;
};

void update_harbor_view( SSConst const& ss );

struct HarborViewComposited {
  std::unique_ptr<HarborSubView> top_level;
  std::unordered_map<e_harbor_view_entity, HarborSubView*>
      entities;

  void update();
};

HarborViewComposited recomposite_harbor_view(
    SS& ss, TS& ts, Player& player, Delta const& canvas_size );

} // namespace rn
