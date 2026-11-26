#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","count"
"1","0.000000"
"2","0.000000"
"3","0.000000"
"4","5.425100"
"5","3.076300"
"6","1.781000"
"7","1.259400"
"8","0.903600"
"9","0.634900"
"10","0.466200"
"11","0.338400"
"12","0.240600"
"13","0.191200"
"14","0.136900"
"15","0.099400"
"16","0.078100"
"17","0.059500"
"18","0.043900"
"19","0.035800"
"20","0.025200"
"21","0.021200"
"22","0.013400"
"23","0.012500"
"24","0.007700"
"25","0.008300"
"26","0.004800"
"27","0.005700"
"28","0.004100"
"29","0.003100"
"30","0.002500"
"31","0.002200"
"32","0.001500"
"33","0.001700"
"34","0.000600"
"35","0.000800"
"36","0.000500"
"37","0.001200"
"38","0.000600"
"39","0.000200"
"40","0.000400"
"41","0.000200"
"42","0.000200"
"43","0.000100"
"44","0.000400"
"45","0.000100"
"46","0.000100"
"47","0.000200"
"48","0.000000"
"49","0.000100"
"50","0.000000"
"51","0.000000"
"52","0.000100"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "River Length Histogram (generated) (mbmt) [10000]"
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
