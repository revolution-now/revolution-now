#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","11.822400"
"5","6.730400"
"6","3.878600"
"7","2.274100"
"8","1.447100"
"9","0.921300"
"10","0.592400"
"11","0.401100"
"12","0.254600"
"13","0.173800"
"14","0.110100"
"15","0.076800"
"16","0.051400"
"17","0.034900"
"18","0.027000"
"19","0.017500"
"20","0.012800"
"21","0.007300"
"22","0.004100"
"23","0.003500"
"24","0.003100"
"25","0.001200"
"26","0.001100"
"27","0.001000"
"28","0.000700"
"29","0.000300"
"30","0.000200"
"31","0.000100"
"32","0.000200"
"33","0.000000"
"34","0.000100"
"35","0.000100"
"36","0.000000"
"37","0.000000"
"38","0.000100"
"39","0.000000"
"40","0.000000"
"41","0.000000"
"42","0.000000"
"43","0.000000"
"44","0.000000"
"45","0.000000"
"46","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mmmb) [10000]"
set key outside right
set grid
set xlabel "Length"
set ylabel "Count Per Map"
set key autotitle columnhead
set xrange [1:20]
set yrange [0:20]
plot for [col=2:*] $CSVData using 1:col with lines lw 3

set output
system sprintf( "eog --fullscreen '%s' >/dev/null 2>&1 &", outfile )
