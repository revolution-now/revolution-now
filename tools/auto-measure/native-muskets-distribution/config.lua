return {
  -- Variable parameters.
  difficulty={
    'discoverer', --
    'explorer', --
    'conquistador', --
    'governor', --
    'viceroy', --
  },
  tribe_name={
    'tupi', --
    'iroquois', --
    'aztec', --
    'inca', --
  },

  -- Fixed parameters.
  --
  -- This can be "old" or "new". But we can't put both in a list
  -- because the setters for these scenarios don't know how to
  -- create/destroy a brave. So we need to manually prepare a sav
  -- file for each one.
  brave='new',

  -- General fields.
  target_trials=1000,
}