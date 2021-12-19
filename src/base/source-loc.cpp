/****************************************************************
**source-loc.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-19.
*
* Description: Use this to use the source_location facility.
*
*****************************************************************/
#include "source-loc.hpp"

using namespace std;

namespace base {

void to_str( base::SourceLoc const& o, std::string& out,
             ADL_t ) {
  out += fmt::format( "{}:{}:{}", o.file_name(), o.line(),
                      o.column() );
};

}
