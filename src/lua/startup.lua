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
  populate.create_units_in_europort()
end

package_exports = {
  run = run
}
