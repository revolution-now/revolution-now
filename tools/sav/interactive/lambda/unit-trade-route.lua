local function lambda( sav )
  assert( sav )
  local UNIT = assert( sav.UNIT )

  for _, unit in ipairs( UNIT ) do
    if unit.nation_info.nation_id ~= 'England' then
      goto continue
    end
    -- if unit.type ~= 'Wagon train' then goto continue end
    if unit.type ~= 'Privateer' then goto continue end
    local prof = unit.auxiliary_data
    printf( 'auxiliary_data: %d', prof )
    ::continue::
  end
end

return lambda