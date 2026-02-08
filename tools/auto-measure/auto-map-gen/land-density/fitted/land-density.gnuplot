#!/usr/bin/env -S gnuplot -p
Gaussian( x, b, c ) = exp(-.5*((x - b)/c)**2)
# NormalizedGaussian( x, b, c ) = (1/(c*(2*pi)**.5)) * exp(-.5*((x - b)/c)**2)

# Set the x and y ranges for better visualization
set xrange [0:0.5]
set yrange [0:2.0]

# Plot the function
plot Gaussian( x, .23-.07, .03) title "t",   \
     Gaussian( x, .23,     .03) title "m",   \
     Gaussian( x, .23+.07, .03) title "b",   \
     Gaussian( x, .28,     .07) title "new", \
     0.5 title "half"