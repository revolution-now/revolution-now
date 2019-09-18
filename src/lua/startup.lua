-----------------------------------------------------------------
-- startup.lua
--
-- Project: Revolution Now
--
-- Created by dsicilia on 2019-09-17.
--
-- Description: Code to be run at startup.
--
-----------------------------------------------------------------

local function run()
  europort.create_some_units( e.nation.dutch )
end

package_exports = {
  run = run
}
