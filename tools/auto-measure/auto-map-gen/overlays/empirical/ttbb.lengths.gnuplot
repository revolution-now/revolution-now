#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","mountains","hills","clearing"
"1","0.0","0.0","0.0"
"2","-2.6088029212575","-2.5466840094842","-2.0750715420087"
"3","-4.4480289277244","-4.4860794775184","-2.9740707767728"
"4","-5.8343232888443","-6.1908275697568","-3.310543013394"
"5","-inf","-7.5771219308767","-3.9644694808007"
"6","-7.6260827580724","-inf","-4.5835086892069"
"7","-inf","-inf","-4.9199809258281"
"8","-inf","-inf","-6.5294188382622"
"9","-inf","-inf","-5.8362716577023"
"10","-inf","-inf","-inf"
"11","-inf","-inf","-inf"
"12","-inf","-inf","-5.4308065495941"
"13","-inf","-inf","-inf"
"14","-inf","-inf","-6.5294188382622"
"15","-inf","-inf","-6.5294188382622"
"16","-inf","-inf","-inf"
"17","-inf","-inf","-inf"
"18","-inf","-inf","-6.5294188382622"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "Range Length Histogram (empirical) (ttbb) [100]"
set key outside right
set grid
set xlabel "Length (cardinal adjacent)"
set ylabel "Frequency"
set key autotitle columnhead
set xrange [1:30]
set yrange [-20:0]
plot for [col=2:*] $CSVData using 1:col with lines lw 3

set output
system sprintf( "eog --fullscreen '%s' >/dev/null 2>&1", outfile )
