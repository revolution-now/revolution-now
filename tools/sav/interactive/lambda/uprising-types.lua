-- local NATIONS = { ENGLISH=1, FRENCH=2, SPANISH=3, DUTCH=4 }
local UNIT_TYPES = {
  Soldier={
    [false]='soldier', --
    [true]='veteran_soldier', --
  },
  Dragoon={
    [false]='dragoon', --
    [true]='veteran_dragoon', --
  },
}

local function lambda( sav )
  assert( sav )
  -- local HEADER = assert( sav.HEADER )

  local COLONY = assert( sav.COLONY )
  local population = assert( COLONY[1].population )

  local UNIT = sav.UNIT

  local units = {}
  local total = 0

  for _, unit in ipairs( UNIT ) do
    if unit.nation_info.nation_id ~= 'Netherlands' then
      goto continue
    end
    total = total + 1
    if not UNIT_TYPES[unit.type] then goto continue end
    local prof = unit.profession_or_treasure_amount
    assert( prof == 21 or prof == 28 )
    local veteran = (unit.profession_or_treasure_amount == 21)
    local type = assert( UNIT_TYPES[unit.type][veteran] )
    units[type] = units[type] or 0
    units[type] = units[type] + 1
    ::continue::
  end

  printf( 'pop=%d total=%d types=%d/%d/%d/%d', population, total,
          units.soldier or 0, units.veteran_soldier or 0,
          units.dragoon or 0, units.veteran_dragoon or 0 )
end

return lambda