local conf = {
  target_count=100, --

  -- Values:
  --   new:       "Start a Game in NEW WORLD".
  --   customize: "CUSTOMIZE New World".
  mode='customize',

  -- Values:
  --   -1 ==> up
  --    0 ==> middle
  --    1 ==> down
  settings={},
}

if conf.mode == 'customize' then
  for land_mass = -1, 1 do
    for land_form = -1, 1 do
      for temperature = -1, 1 do
        for climate = -1, 1 do
          table.insert( conf.settings, {
            land_mass=land_mass,
            land_form=land_form,
            temperature=temperature,
            climate=climate,
          } )
        end
      end
    end
  end
else
  table.insert( conf.settings, 'n/a' )
end

return conf