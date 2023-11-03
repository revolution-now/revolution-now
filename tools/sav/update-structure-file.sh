#!/bin/bash
# Run this when the smcol_sav_struct.json file is updated, which
# is done by pulling in changes from the smcol_saves_utility ex-
# tern repo.
set -eo pipefail

sav="$(realpath "$(dirname "$0")")"
tools="$(realpath "$sav/../")"
root="$(realpath "$tools/../")"

cd "$root"
[[ -d src ]]

input="extern/smcol_saves_utility/smcol_sav_struct.json"
output="$sav/sav-structure.json"

[[ -f "$input" ]]

python3 "$sav/preprocess-structure-file.py" "$input" "$output"