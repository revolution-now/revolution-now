/****************************************************************
**fsm.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-25.
*
* Description: Finite State Machine.
*
*****************************************************************/
#include "fsm.hpp"

// Revolution Now
#include "fmt-helper.hpp"
#include "logging.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

namespace internal {

#if 0
using tag_t = FmtTags<    //
    FmtRemoveRnNamespace, //
    FmtRemoveTemplateArgs //
    >;

void log_state( std::string const& child_name,
                std::string const& logged_state ) {
  auto tag = tag_t{};
  lg.debug( "{} state: {}", tag( child_name ),
            tag( logged_state ) );
}

void log_event( std::string const& child_name,
                std::string const& logged_event ) {
  auto tag = tag_t{};
  lg.debug( "{} processing event: {}", tag( child_name ),
            tag( logged_event ) );
}
#else
void log_state( std::string const& /*unused*/,
                std::string const& /*unused*/ ) {}

void log_event( std::string const& /*unused*/,
                std::string const& /*unused*/ ) {}
#endif

} // namespace internal

void test_fsm() {}

} // namespace rn
