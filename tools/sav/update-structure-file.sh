#!/bin/bash
set -eo pipefail

sav="$(realpath "$(dirname "$0")")"
tools="$(realpath "$sav/../")"
root="$(realpath "$tools/../")"

cd "$root"
[[ -d data ]]

input="extern/smcol_saves_utility/smcol_sav_struct.json"
output="data/sav-structure.json"

[[ -f "$input" ]]

python3 "$sav/preprocess-structure-file.py" "$input" "$output"