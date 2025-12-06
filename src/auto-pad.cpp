/****************************************************************
**auto-pad.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-04-05.
*
* Description: Auto merged padding around UI elements.
*
*****************************************************************/
#include "auto-pad.hpp"

// config
#include "config/ui.rds.hpp"

#define DISPATCH( type )                        \
  if( auto const o = v.cast_safe<type>(); o ) { \
    pad_children( *o );                         \
    return;                                     \
  }

using namespace std;
using namespace rn::ui;

namespace rn {

namespace {

/****************************************************************
** AutoPadder.
*****************************************************************/
struct AutoPadder {
  AutoPadder( int const pixels ) : pixels_( pixels ) {}

  void pad( unique_ptr<View>& view ) const {
    CHECK( view != nullptr );
    pad_children( *view );
    pad_self( view );
  }

 private:
  void pad_self( unique_ptr<View>& view ) const {
    if( !view->needs_padding() ) return;
    // In this one we use half the padding because the assumption
    // is that this sub view will be touching another view that
    // also has half padding.
    view = make_unique<PaddingView>( std::move( view ),
                                     pixels_ / 2 );
  }

  void pad_children( View& v ) const {
    // Any (base) view that has children needs to be given an
    // overload in this section.
    DISPATCH( RefView );
    DISPATCH( CompositeView );
  }

  void pad_children( RefView& ref ) const {
    pad_children( ref.referenced() );
  }

  void pad_children( CompositeView& parent ) const {
    for( int i = 0; i < parent.count(); ++i ) {
      unique_ptr<View>& child = parent.mutable_at( i );
      CHECK( child != nullptr );
      // Always try padding the sub views themselves, even if
      // this view has can_pad_immediate_children() == false, be-
      // cause that only applies to this level in the hierarchy,
      // not recursively.
      pad_children( *child );
      if( parent.can_pad_immediate_children() )
        pad_self( child );
    }
    parent.children_updated();
  }

  int const pixels_ = {};
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void autopad( unique_ptr<View>& view ) {
  AutoPadder( config_ui.window.ui_padding ).pad( view );
}

} // namespace rn
