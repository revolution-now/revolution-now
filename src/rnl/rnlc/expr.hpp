/****************************************************************
**expr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Expression objects for the RNL language.
*
*****************************************************************/
#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace rnl::expr {

struct AlternativeMember {
  std::string type;
  std::string var;

  std::string to_string( std::string_view spaces ) const;
};

struct Alternative {
  std::string                    name;
  std::vector<AlternativeMember> members;

  std::string to_string( std::string_view spaces ) const;
};

enum class e_sumtype_feature {
  formattable,
  serializable,
  equality
};

std::string to_str( e_sumtype_feature feature );
std::optional<e_sumtype_feature> from_str( std::string feature );

struct TemplateParam {
  std::string param;
};

struct Sumtype {
  std::string                name;
  std::vector<TemplateParam> tmpl_params;
  // A specified-but-empty feature list means something different
  // from one that was not specified at all.
  std::optional<std::vector<e_sumtype_feature>> features;
  std::vector<Alternative>                      alternatives;

  std::string to_string( std::string_view spaces ) const;
};

using Construct = std::variant< //
    Sumtype                     //
    >;

std::string to_str( Construct const& construct,
                    std::string_view spaces = "" );

struct Item {
  std::string            ns;
  std::vector<Construct> constructs;

  std::string to_string( std::string_view spaces ) const;
};

struct Metadata {
  // Module name should be stem of the input rnl file.
  std::string module_name;
};

struct Rnl {
  Metadata                 meta;
  std::vector<std::string> imports;
  std::vector<std::string> includes;
  std::vector<Item>        items;

  std::string to_string() const;
};

} // namespace rnl::expr
