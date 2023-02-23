#!/bin/bash
cat $1 | sort | uniq -c | sort -nr

evade_count=$(cat $1 | grep evade | wc -l)
total_count=$(cat $1 | wc -l)

echo
echo "Total trials: $total_count"
evade_percent="$(lua -e "print( string.format( '%.2f%%', $evade_count*100/$total_count ) )")"
echo "Evade %:      $evade_percent"
error="$(lua -e "print( string.format( '%.2f', (1/($total_count)^.5)*$evade_count*100/$total_count ) )")"
echo "  error:   +/-$error (absolute)"