-- Test the probability that, when an armed brave is destroyed,
-- the tribe will retain the muskets.
--
-- In this test we have a european military unit waiting for
-- order sitting next to an armed brave. The european unit at-
-- tacks the branch. If the european unit loses then that is
-- recorded, but it is not relevant. If the european unit wins
-- (meaning that the armed brave has been destroyed) we measure
-- whether the tribe has retained the muskets. The tribe's mus-
-- kets start at 0 and if they end up at 1 then that means that
-- the tribe retained the muskets. Ideally the european unit
-- should be as strong as possible to minimize the number of out-
-- comes where it loses, although it is OK if it loses since that
-- result will be sifted out.
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
  target_trials=400,
}