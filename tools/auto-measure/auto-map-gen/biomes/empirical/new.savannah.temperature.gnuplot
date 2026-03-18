#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"temperature","frequency"
"-500.0","0"
"-450.0","0"
"-400.0","0"
"-350.0","0"
"-300.0","0.002"
"-250.0","0.0025"
"-200.0","0.014"
"-150.0","0.0605"
"-100.0","0.1935"
"-50.0","0.0705"
"0.0","0.177"
"50.0","0.0635"
"100.0","0.176"
"150.0","0.064"
"200.0","0.1765"
EOF

set title "Temperature Histograph from Savannah (empirical) (new) [2000]"
set key outside right
set grid
set xlabel "Temperature"
set ylabel "Frequency"
set key autotitle columnhead
set xrange [-300:300]
set yrange [0:1]
plot for [col=2:*] $CSVData using 1:col with lines lw 2
