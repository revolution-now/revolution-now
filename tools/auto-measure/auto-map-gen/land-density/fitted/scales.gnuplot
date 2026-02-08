#!/usr/bin/env -S gnuplot -p
Gaussian( x, b, c ) = exp(-.5*((x - b)/c)**2)
# NormalizedGaussian( x, b, c ) = (1/(c*(2*pi)**.5)) * exp(-.5*((x - b)/c)**2)

# Set the x and y ranges for better visualization
set xrange [0:20]
set yrange [0:2.0]

# Plot the function
plot Gaussian( x,  9, 1.25 ) title "t",   \
     Gaussian( x, 12, 1.25 ) title "m",   \
     Gaussian( x, 15, 1.25 ) title "b",   \
     Gaussian( x, 12, 2.50 ) title "new", \
     0.5 title "half"