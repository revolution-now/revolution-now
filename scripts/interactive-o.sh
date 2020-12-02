#!/bin/bash
# This script is meant to be run under `entr` with the dependent
# list of files piped in.
c_norm="\033[00m"
c_green="\033[32m"
c_red="\033[31m"

clear

if $(dirname $0)/o.sh "$@"; then
  clear
  echo -e "${c_green}success${c_norm}."
else
  echo
  echo -e "${c_red}failed${c_norm}."
fi