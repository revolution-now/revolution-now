/****************************************************************
**to-str-ext-base.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-18.
*
* Description: to_str implementations to base types.
*
*****************************************************************/
#include "to-str-ext-base.hpp"

// base
#include "fmt.hpp"

using namespace std;

namespace base {

void to_str( base::SourceLoc const& o, std::string& out,
             ADL_t ) {
  out += fmt::format( "{}:{}:{}", o.file_name(), o.line(),
                      o.column() );
};

} // namespace base
