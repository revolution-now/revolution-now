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
#include <vector>

namespace rds::expr {

/****************************************************************
** General
*****************************************************************/
struct TemplateParam {
  std::string param;
};

enum class e_feature { formattable, serializable, equality };

/****************************************************************
** sumtype
*****************************************************************/
struct AlternativeMember {
  std::string type;
  std::string var;
};

struct Alternative {
  std::string                    name;
  std::vector<AlternativeMember> members;
};

std::string            to_str( e_feature feature );
base::maybe<e_feature> feature_from_str(
    std::string_view feature );

struct Sumtype {
  std::string                name;
  std::vector<TemplateParam> tmpl_params;
  // A specified-but-empty feature list means something different
  // from one that was not specified at all.
  base::maybe<std::vector<e_feature>> features;
  std::vector<Alternative>            alternatives;
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
struct StructMember {
  std::string type;
  std::string var;
};

struct Struct {
  std::string                name;
  std::vector<TemplateParam> tmpl_params;
  // A specified-but-empty feature list means something different
  // from one that was not specified at all.
  base::maybe<std::vector<e_feature>> features;
  std::vector<StructMember>           members;
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

struct Metadata {
  // Module name should be stem of the input rds file.
  std::string module_name;
};

struct Rds {
  Metadata                 meta;
  std::vector<std::string> imports;
  std::vector<std::string> includes;
  std::vector<Item>        items;
};

} // namespace rds::expr
