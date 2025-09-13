local function lambda( json )
  json.HEADER.game_flags_1.cheats_enabled = true
  json.HEADER.game_flags_1.show_indian_moves = false
  json.HEADER.show_entire_map = 1
  json.STUFF.zoom_level = 3
end

return lambda
