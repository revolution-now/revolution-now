/****************************************************************
**config-helper.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-28.
*
* Description: Helper for registering/loading config data.
*
*****************************************************************/
#pragma once

// refl
#include "refl/ext.hpp"

// cdr
#include "cdr/ext.hpp"

// base
#include "base/error.hpp"
#include "base/valid.hpp"

// C++ standard library
#include <functional>
#include <unordered_map>

namespace rds {

/****************************************************************
** Types.
*****************************************************************/
using PopulatorErrorType = base::valid_or<std::string>;

using PopulatorSig = PopulatorErrorType( cdr::value const& o );

using PopulatorFunc = std::function<PopulatorSig>;

using PopulatorsMap =
    std::unordered_map<std::string, PopulatorFunc>;

/****************************************************************
** Private stuff.
*****************************************************************/
namespace detail {

struct empty_registrar {};

void register_config_erased( std::string const& name,
                             PopulatorFunc      populator );

} // namespace detail

/****************************************************************
** Public API.
*****************************************************************/
PopulatorsMap const& config_populators();

} // namespace rds
