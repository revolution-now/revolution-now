/****************************************************************
* window.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-30.
*
* Description: Handles windowing system for user interaction.
*
*****************************************************************/
#include "window.hpp"

using namespace std;

namespace rn {
namespace gui {

namespace {

class View : public Object {
public:
  virtual ~View() {}
};

} // namespace

void message_box( string_view msg, RenderFunc render_bg ) {
  (void)msg;
  (void)render_bg;
}
  
} // namespace gui
} // namespace rn
