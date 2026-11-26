#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","9.753900"
"5","4.524800"
"6","2.161100"
"7","1.620900"
"8","1.134000"
"9","0.760900"
"10","0.516600"
"11","0.357900"
"12","0.264200"
"13","0.188000"
"14","0.133200"
"15","0.095700"
"16","0.073600"
"17","0.049700"
"18","0.035800"
"19","0.030400"
"20","0.019700"
"21","0.018000"
"22","0.011300"
"23","0.009500"
"24","0.006500"
"25","0.005000"
"26","0.004100"
"27","0.002900"
"28","0.002000"
"29","0.001800"
"30","0.001100"
"31","0.000900"
"32","0.000200"
"33","0.000700"
"34","0.001000"
"35","0.000100"
"36","0.000500"
"37","0.000400"
"38","0.000300"
"39","0.000000"
"40","0.000100"
"41","0.000200"
"42","0.000000"
"43","0.000000"
"44","0.000100"
"45","0.000000"
"46","0.000000"
"47","0.000200"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mmmt) [10000]"
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
