-- Test the probability that, when an mounted brave is destroyed,
-- the tribe will retain the horses.
--
-- In this test we have a european military unit waiting for
-- order sitting next to a mounted brave. The european unit at-
-- tacks the brave. If the european unit loses then that is
-- recorded, but it is not relevant. If the european unit wins
-- (meaning that the mounted brave has been destroyed) we measure
-- whether the tribe has retained the horses. The tribe's
-- horse_breeding starts at 0 and if they end up at 25 then that
-- means that the tribe has retained the horses. Ideally the eu-
-- ropean unit should be as strong as possible to minimize the
-- number of outcomes where it loses, although it is OK if it
-- loses since that result will be sifted out.
return {
  -- Variable parameter.
  difficulty={
    'discoverer', --
    -- 'explorer', --
    -- 'conquistador', --
    -- 'governor', --
    'viceroy', --
  },

  -- Variable parameter.
  tribe_name={
    'tupi', --
    -- 'iroquois', --
    -- 'aztec', --
    'inca', --
  },

  -- General fields.
  target_trials=200,
}