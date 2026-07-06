#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"savannah_distance","frequency"
"0","0"
"1","0"
"2","0"
"3","0"
"4","0"
"5","0"
"6","0"
"7","0"
"8","0"
"9","0"
"10","0.0115"
"11","0.189"
"12","0.7995"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "Savannah Row Limit Histograph (empirical) (bbbm) [2000]"
set key outside right
set grid
set xlabel "Distance from Equator"
set ylabel "Frequency"
set key autotitle columnhead
set xrange [0:20]
set yrange [0:1]
plot for [col=2:*] $CSVData using 1:col with lines lw 3

set output
system sprintf( "eog --fullscreen '%s' >/dev/null 2>&1 &", outfile )
