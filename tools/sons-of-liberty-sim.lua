-- This script is used to simulate the Sons of Liberty percent
-- evolution logic that the original game may have used. In par-
-- ticular, it runs iterations of the (correct) equation:
--
--        dR   B - 2R
-- (1)    -- = ------
--        dt    100
--
-- together with rounding behavior intended to simulate some
-- crude rounding that would happen with fixed point floating
-- point math which tended to be used before fast floating point
-- math was available in hardware.
--
-- The below will progressively increase the decimal precision by
-- powers of two and, for each precision level, it will run two
-- simulations of 1000 iterations each. These two simulations
-- start of the bottom and top of the range and will iterate to
-- find the convergence points. With no rounding, both simula-
-- tions (starting from the bottom and starting from top) should
-- converge onto the same theoretical equilibrium point of .5
-- (1-2*x=0). But instead, what is observed for the more crude
-- rounding levels, is that the convergence point differs between
-- the two and is generally incorrect.
--
-- Furthermore, between those two convergence points, there is a
-- "dead zone," where no evolution happens at all, no matter what
-- the starting point.
--
-- For example, with rounding precision of two binary decimal
-- places, a series of iterations starting from 1% will max out
-- at around 38%, while the one starting from the top will de-
-- scend to 50%. Then in between those two, there will be a dead
-- zone where no evolution happens (this is not demonstrated be-
-- low, but it can be by tweaking the numbers so that they begin
-- the range in the dead zone; e.g. change start and finish to 40
-- and 42, respectively, and observe that there is no movement
-- until a precision of 2^3).
--
-- Qualitatively this reproduces behavior that is observed in the
-- original game, namely that there can sometimes be two equilib-
-- rium points that are different and which are converged upon
-- depending on the direction things are moving, and that there
-- is a dead zone in the middle where there is no movement. The
-- fact that we can reproduce such strange behavior with a simple
-- (and plausible) algorithm, suggests that are true correct for-
-- mula (1) is the one that the game uses, but deviates from it
-- because of the primitive rounding that it uses.

-- Simulate the kind of rounding error that you'd have with a
-- fixed point number representation which keeps log2(precision)
-- bits.
local function fake_round( input, precision )
  return math.floor( input * precision ) / precision
end

-- Given the starting point 2, iterate the difference equation
-- (1) in the presence of simulated fixed-point rounding to find
-- where it converges.
local function find( s, precision )
  for i = 0, 1000 do
    local delta = (1.0 - 2 * s / 100.0)
    delta = fake_round( delta, precision )
    s = s + delta
    s = fake_round( s, precision )
  end
  return s
end

local round = 1

-- The interval boundaries; ideally, with no rounding, starting
-- from either of these points should converge onto the same
-- equilibrium point, since our difference equation only has one
-- such point. But with rounding that is not so.
local start = 0
local finish = 100

for i = 1, 20 do
  print( 'round=2^' .. string.format( '%-2d', i-1 ) .. ': [' ..
             string.format( '%15s', find( start, round ) ) .. ',' ..
             find( finish, round ) .. ']' )
  round = round * 2
end