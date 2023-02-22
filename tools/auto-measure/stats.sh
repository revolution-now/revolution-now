#!/bin/bash
cat $1 | sort | uniq -c | sort -nr

evade_count=$(cat $1 | grep evade | wc -l)
total_count=$(cat $1 | wc -l)

echo

evade_percent="$(lua -e "print( $evade_count*100/$total_count )")"
echo "Evade %: $evade_percent"