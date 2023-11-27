#!/bin/bash
# This script is not meant to be run standalone; it is meant to
# be run by the auto-differ.
set -eo pipefail
# set -x

interactive="$(dirname "$0")"
conv="$(realpath "$interactive/../conversion")"

sav_path=$1
echo "sav_path: $sav_path"
[[ -e "$sav_path" ]]

sav_file="$(basename "$sav_path")"
echo "sav_file: $sav_file"
[[ "$sav_file" =~ ^COLONY[0-9][0-9]\.SAV ]]

sav_dir="$(dirname "$sav_path")"
echo "sav_dir: $sav_dir"
[[ -d "$sav_dir" ]]

out_dir="$sav_dir/$sav_file.dir"
echo "out_dir: $out_dir"
mkdir -p "$out_dir"

tmp_conv_dir=/tmp/diff-analyzer
echo "tmp_conv_dir: $tmp_conv_dir"
mkdir -p "$tmp_conv_dir"
[[ -d "$tmp_conv_dir" ]]

tmp_conv_file="$tmp_conv_dir/$sav_file.json"
echo "tmp_conv_file: $tmp_conv_file"
command rm -f "$tmp_conv_file"

converter="$conv/binary-to-json.sh"

echo "running converter: $converter $sav_path $tmp_conv_file"
$converter "$sav_path" "$tmp_conv_file"

find_year() {
  cat "$tmp_conv_file" \
    | sed -rn 's/ +"year": ([0-9]+),$/\1/p'
}

find_season() {
  cat "$tmp_conv_file" \
    | sed -rn 's/ +"season": "([a-z]+)",$/\1/p'
}

year="$(find_year)"
echo "found year:   $year"
[[ -n "$year" ]]

season="$(find_season)"
echo "found season: $season"
[[ -n "$season" ]]

epoch_secs="$(date +"%s")"
echo "epoch seconds: $epoch_secs"

renamed_basename="$sav_file.$year-$season.$epoch_secs.SAV.json"
renamed_file="$out_dir/$renamed_basename"
echo "renaming file to: $renamed_file"
[[ ! -e "$renamed_file" ]]

echo "$tmp_conv_file --> $renamed_file"
mv "$tmp_conv_file" "$renamed_file"

# When this is 1 we will not delete previous versions; they will
# just accumulate (which they can do since they all will have
# unique file names). This uses a lot of space. When it is 0 then
# we only keep the last two for comparison.
keep_all=0

if (( keep_all == 0 )); then
  if [[ -e "$out_dir/previous" ]]; then
    echo "moving previous to deleteme..."
    (cd "$out_dir" && mv previous deleteme)
  fi

  if [[ -L "$out_dir/deleteme" ]]; then
    # If there is a file pointed to by delete me then delete it.
    if [[ -e "$out_dir/deleteme" ]]; then
      echo "deleting file pointed to by deleteme..."
      (cd "$out_dir" && rm "$(readlink -f deleteme)")
    fi
    [[   -L "$out_dir/deleteme" ]]
    [[ ! -e "$out_dir/deleteme" ]]
    echo "deleting deleteme..."
    (cd "$out_dir" && rm deleteme)
  fi
fi

if [[ -e "$out_dir/latest" ]]; then
  echo "moving latest to previous..."
  (cd "$out_dir" && mv latest previous)
fi

latest="$renamed_basename"
echo "creating latest symlink: latest -> $latest"
(cd "$out_dir" && ln -sf "$latest" latest)

echo "finished."