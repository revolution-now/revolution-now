local conf = {
  target_count=500, --

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
  local modes = {
    { temperature=0, climate=-1 }, --
    { temperature=0, climate=1 }, --
    { temperature=-1, climate=0 }, --
    { temperature=1, climate=0 }, --
    { temperature=0, climate=0 }, --
  }
  for _, p in ipairs( modes ) do
    table.insert( conf.settings, {
      land_mass=1,
      land_form=1,
      temperature=assert( p.temperature ),
      climate=assert( p.climate ),
    } )
  end
else
  table.insert( conf.settings, 'n/a' )
end

return conf