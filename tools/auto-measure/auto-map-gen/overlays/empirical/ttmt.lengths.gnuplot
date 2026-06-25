#!/usr/bin/env -S gnuplot -p
set datafile separator comma

$CSVData << EOF
"length","mountains","hills","clearing"
"1","0.0","0.0","0.0"
"2","-2.200081722137","-2.3196197918731","-2.0794415416798"
"3","-4.0957721142133","-4.0556903552406","-2.8798342525934"
"4","-6.0416822632686","-5.4419847163604","-3.482830336159"
"5","-6.5525078870346","-7.7445698093545","-4.9042160170902"
"6","-6.5525078870346","-inf","-4.9042160170902"
"7","-inf","-inf","-5.2406882537114"
"8","-inf","-inf","-5.7515138774774"
"9","-inf","-inf","-6.1569789855856"
"10","-inf","-inf","-6.8501261661455"
"11","-inf","-inf","-6.1569789855856"
"12","-inf","-inf","-6.1569789855856"
"13","-inf","-inf","-inf"
"14","-inf","-inf","-inf"
"15","-inf","-inf","-inf"
"16","-inf","-inf","-6.8501261661455"
EOF

outfile = system( "mktemp /tmp/gnuplot-XXXXXX.png" )

set term png size 1920,1200 font "Fira Sans,14"
set output outfile

set title "Range Length Histogram (empirical) (ttmt) [100]"
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
