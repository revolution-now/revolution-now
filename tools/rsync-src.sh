#!/bin/bash
set -eo pipefail

source ~/dev/utilities/bashlib/util.sh

usage() {
  die "usage: $0 <source-host>"
  exit 1
}

[[ -d .builds ]] || die 'must run from rn root folder.'

host="$1"
[[ -n "$host" ]] || usage

get_file() {
  local from="$1"
  local to="$2"
  log "creating local destination: $(dirname "$to")"
  mkdir -p "$(dirname "$to")"
  log "pulling $host file: $from"
  rsync --checksum "dsicilia@$host:$from" "$to"
}

get_dir() {
  local from="$1"
  local to="$2"
  log "creating local destination: $to"
  mkdir -p "$to"
  log "pulling $host folder: $from"
  rsync --checksum --recursive "dsicilia@$host:$from" "$to"
}

sync_file() {
  local rel="$1"
  echo "Are you sure you want to rsync the file $rel? <Ctrl-c> to cancel."
  read
  [[ -n "$rel" ]]
  local local_file="$rel"
  local remote_file="/home/dsicilia/dev/revolution-now/$rel"
  [[ -f "$local_file" ]]
  [[ -f "$remote_file" ]]
  get_file  "$remote_file" "$local_file"
}

sync_dir() {
  local rel="$1"
  echo "Are you sure you want to the folder $rel? <Ctrl-c> to cancel."
  read
  [[ -n "$rel" ]]
  local local_dir="$rel/"
  local remote_dir="/home/dsicilia/dev/revolution-now/$rel/"
  [[ -d "$local_dir" ]]
  [[ -d "$remote_dir" ]]
  get_dir  "$remote_dir" "$local_dir"
}

# sync_dir  src
# sync_dir  test
# sync_dir  config
# sync_dir  assets
# sync_dir  saves
# sync_file tools/ide/contents/rn.lua