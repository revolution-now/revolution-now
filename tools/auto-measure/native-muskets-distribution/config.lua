-- Measure the probability that, when a brave is given muskets,
-- the tribe's stockpile reduces.
--
-- This test starts with a single native dwelling of a tribe that
-- has some muskets, and there are no european units involved.
-- The end of turn is blinking. Broadly speaking there are two
-- types of this test, "old" and "new", parametrized below. For
-- the "old" test, we have an existing brave sitting right on top
-- of the dwelling, then we advance the turn and then measure
-- whether the brave has been given muskets (i.e., it becomes an
-- armed brave) and, if so, whether the tribe has reduced its
-- stockpile. In the "new" case we have no braves to start, but
-- the dwelling's growth_counter is set to 20 so that when we ad-
-- vance the turn it will always create a brave, and then we
-- again measure whether it was given muskets and, if so, whether
-- the tribe's stockpile was decreased as a result.
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