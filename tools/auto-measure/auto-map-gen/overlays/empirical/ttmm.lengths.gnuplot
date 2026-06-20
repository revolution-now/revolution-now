#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","mountains","hills","clearing"
"1","0.0","0.0","0.0"
"2","-2.3987909922624","-2.4103470108781","-2.0370731623214"
"3","-4.1697564893157","-4.2475332410132","-3.0169253756853"
"4","-5.6791835358392","-6.0587107960983","-3.5899773808305"
"5","-7.0449993656156","-7.5926411560243","-4.2096766552673"
"6","-7.6113948405364","-8.8919241401545","-4.598120075806"
"7","-8.4586927009236","-10.683683609383","-5.0357422955487"
"8","-9.5573049895917","-inf","-5.3840489898169"
"9","-9.0464793658257","-inf","-5.8320737123439"
"10","-9.2696229171399","-inf","-6.4418392839648"
"11","-9.9627700976999","-inf","-6.7703433509368"
"12","-inf","-inf","-7.0957657513714"
"13","-9.9627700976999","-inf","-8.5621028201648"
"14","-9.9627700976999","-inf","-7.2628198360346"
"15","-inf","-inf","-7.4634905314967"
"16","-inf","-inf","-8.0512771963989"
"17","-inf","-inf","-inf"
"18","-10.65591727826","-inf","-8.5621028201648"
"19","-inf","-inf","-inf"
"20","-inf","-inf","-8.5621028201648"
"21","-inf","-inf","-8.967567928273"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "Range Length Histogram (empirical) (ttmm) [2000]"
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
