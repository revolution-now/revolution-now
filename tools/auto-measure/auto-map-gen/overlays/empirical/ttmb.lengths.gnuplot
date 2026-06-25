#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","mountains","hills","clearing"
"1","0.0","0.0","0.0"
"2","-2.4122768745036","-2.5580663411037","-2.0887069951256"
"3","-4.3974077367122","-4.4788179305229","-3.1662658745959"
"4","-5.8245240923523","-6.2280177853321","-3.4539479470477"
"5","-inf","-6.9211649658921","-4.3294166844016"
"6","-inf","-inf","-4.6860916283403"
"7","-7.6162835615804","-inf","-5.5333894887275"
"8","-inf","-inf","-5.9388545968357"
"9","-7.6162835615804","-inf","-5.0225638649615"
"10","-inf","-inf","-5.5333894887275"
"11","-inf","-inf","-5.9388545968357"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "Range Length Histogram (empirical) (ttmb) [100]"
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
