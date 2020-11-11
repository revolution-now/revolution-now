/****************************************************************
**code-gen.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Code generator for the RNL language.
*
*****************************************************************/
#include "code-gen.hpp"

// rnlc
#include "rnl-util.hpp"

// c++ standard library
#include <sstream>

using namespace std;

namespace rnl {

optional<string> generate_code( expr::Rnl const& rnl ) {
  ostringstream oss;
  oss << "\n/*\n\n" << rnl.to_string() << "\n*/";
  return oss.str();
}

} // namespace rnl
