#!/bin/bash
# Run this when the smcol_sav_struct.json file is updated, which
# is done by pulling in changes from the smcol_saves_utility ex-
# tern repo.
set -eo pipefail

this="$(realpath "$(dirname "$0")")"
sav="$(realpath "$this/../")"
tools="$(realpath "$sav/../")"
root="$(realpath "$tools/../")"
schema="$(realpath "$sav/schema")"

cd "$root"
[[ -d src ]]

input="extern/smcol_saves_utility/smcol_sav_struct.json"
output="$schema/sav-structure.json"

[[ -f "$input" ]]

python3 "$schema/preprocess-structure-file.py" "$input" "$output"