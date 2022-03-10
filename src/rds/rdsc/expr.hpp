/****************************************************************
**expr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Expression objects for the RDS language.
*
*****************************************************************/
#pragma once

// base
#include "base/maybe.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <string>
#include <unordered_set>
#include <vector>

namespace rds::expr {

/****************************************************************
** General
*****************************************************************/
struct TemplateParam {
  std::string param;
};

struct StructMember {
  std::string type;
  std::string var;
};

enum class e_feature { equality, validation, offsets };

/****************************************************************
** sumtype
*****************************************************************/
struct Alternative {
  std::string               name;
  std::vector<StructMember> members;
};

std::string            to_str( e_feature feature );
base::maybe<e_feature> feature_from_str(
    std::string_view feature );

struct Sumtype {
  std::string                name;
  std::vector<TemplateParam> tmpl_params;
  // A specified-but-empty feature list means something different
  // from one that was not specified at all.
  base::maybe<std::unordered_set<e_feature>> features;
  std::vector<Alternative>                   alternatives;
};

/****************************************************************
** enum
*****************************************************************/
struct Enum {
  std::string              name;
  std::vector<std::string> values;
};

/****************************************************************
** struct
*****************************************************************/
struct Struct {
  std::string                name;
  std::vector<TemplateParam> tmpl_params;
  // A specified-but-empty feature list means something different
  // from one that was not specified at all.
  base::maybe<std::unordered_set<e_feature>> features;
  std::vector<StructMember>                  members;
};

/****************************************************************
** Document
*****************************************************************/
using Construct = base::variant< //
    Enum,                        //
    Sumtype,                     //
    Struct                       //
    >;

std::string to_str( Construct const& construct,
                    std::string_view spaces = "" );

struct Item {
  std::string            ns;
  std::vector<Construct> constructs;
};

struct Rds {
  std::vector<std::string> includes;
  std::vector<Item>        items;
};

} // namespace rds::expr
