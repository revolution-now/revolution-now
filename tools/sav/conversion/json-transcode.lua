--[[ ------------------------------------------------------------
|
| json-transcode.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-29.
|
| Description: Reads and writes JSON.
|
--]] ------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local json = require( 'moon.json' )

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return {
  JNULL=assert( json.JNULL ),
  decode=assert( json.read ),
  read=assert( json.read ),
  print=assert( json.print ),
  write=assert( json.write ),
  write_pretty=assert( json.write_pretty ),
  write_oneline=assert( json.write_oneline ),
  pprint_ordered=assert( json.pprint_ordered ),
  tostring=assert( json.tostring ),
  tostring_pretty=assert( json.tostring_pretty ),
}
