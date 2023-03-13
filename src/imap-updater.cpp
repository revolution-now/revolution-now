/****************************************************************
**imap-updater.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Interface for modifying the map.
*
*****************************************************************/
#include "imap-updater.hpp"

using namespace std;

namespace rn {

namespace {}

/****************************************************************
** MapUpdaterOptions
*****************************************************************/
namespace detail {

MapUpdaterOptionsPopper::~MapUpdaterOptionsPopper() noexcept {
  CHECK( !map_updater_.options_.empty() );
  MapUpdaterOptions const popped = map_updater_.options_.top();
  map_updater_.options_.pop();
  CHECK( !map_updater_.options_.empty() );
  if( popped != map_updater_.options_.top() )
    map_updater_.redraw();
}

} // namespace detail

/****************************************************************
** IMapUpdater
*****************************************************************/
IMapUpdater::IMapUpdater(
    MapUpdaterOptions const& initial_options ) {
  options_.push( initial_options );
}

IMapUpdater::IMapUpdater()
  : IMapUpdater( MapUpdaterOptions{} ) {}

IMapUpdater::Popper IMapUpdater::push_options_and_redraw(
    OptionsUpdateFunc mutator ) {
  CHECK( !options_.empty() );
  MapUpdaterOptions new_options = options_.top();
  mutator( new_options );
  options_.push( std::move( new_options ) );
  redraw();
  return Popper{ *this };
}

MapUpdaterOptions const& IMapUpdater::options() const {
  CHECK( !options_.empty() );
  return options_.top();
}

void IMapUpdater::mutate_options_and_redraw(
    OptionsUpdateFunc mutator ) {
  CHECK( !options_.empty() );
  MapUpdaterOptions const old_options = options();
  mutator( options_.top() );
  if( options_.top() != old_options ) redraw();
}

void to_str( IMapUpdater const&, string& out, base::ADL_t ) {
  out += "IMapUpdater";
}

} // namespace rn
