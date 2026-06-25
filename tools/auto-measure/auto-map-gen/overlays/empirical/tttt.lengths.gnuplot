#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","mountains","hills","clearing"
"1","0.0","0.0","0.0"
"2","-2.2597449343176","-2.4177621333899","-1.9661128563728"
"3","-4.0020561281316","-3.9492385043543","-3.0063143828845"
"4","-5.84788281863","-5.623214937926","-3.5983654465731"
"5","-6.5410299991899","-inf","-4.0838732623548"
"6","-7.639642287858","-inf","-5.7578496959265"
"7","-inf","-inf","-5.0647025153665"
"8","-inf","-inf","-6.1633148040346"
"9","-7.639642287858","-inf","-6.1633148040346"
"10","-inf","-inf","-inf"
"11","-inf","-inf","-5.7578496959265"
"12","-inf","-inf","-inf"
"13","-inf","-inf","-6.8564619845946"
"14","-inf","-inf","-inf"
"15","-inf","-inf","-6.8564619845946"
"16","-inf","-inf","-inf"
"17","-inf","-inf","-inf"
"18","-inf","-inf","-inf"
"19","-inf","-inf","-inf"
"20","-inf","-inf","-inf"
"21","-inf","-inf","-6.8564619845946"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "Range Length Histogram (empirical) (tttt) [100]"
set key outside right
set grid
set xlabel "Length (cardinal adjacent)"
set ylabel "Frequency"
set key autotitle columnhead
set xrange [1:30]
set yrange [-20:0]
plot for [col=2:*] $CSVData using 1:col with lines lw 3

set output
system sprintf( "eog --fullscreen '%s' >/dev/null 2>&1 &", outfile )
