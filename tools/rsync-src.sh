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

local_src='src/'
remote_src='/home/dsicilia/dev/revolution-now/src/'
get_dir  "$remote_src" "$local_src" &

local_conf='config/'
remote_conf='/home/dsicilia/dev/revolution-now/config/'
get_dir  "$remote_conf" "$local_conf" &

local_contents='tools/ide/contents/rn.lua'
remote_contents='/home/dsicilia/dev/revolution-now/tools/ide/contents/rn.lua'
get_file "$remote_contents" "$local_contents" &

wait