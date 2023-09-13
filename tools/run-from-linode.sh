#!/bin/bash
set -eo pipefail

source ~/dev/utilities/bashlib/util.sh

[[ -d .builds ]] || die 'must run from rn root folder.'

get_file() {
  local from="$1"
  local to="$2"
  log "creating local destination: $(dirname "$to")"
  mkdir -p "$(dirname "$to")"
  log "pulling remote file: $from"
  rsync --checksum "dsicilia@linode:$from" "$to"
}

get_dir() {
  local from="$1"
  local to="$2"
  log "creating local destination: $to"
  mkdir -p "$to"
  log "pulling remote folder: $from"
  rsync --checksum --recursive "dsicilia@linode:$from" "$to"
}

local_exe='.builds/from-linode/exe/exe'
linode_exe='/home/dsicilia/dev/revolution-now/.builds/current/exe/exe'
get_file "$linode_exe"  "$local_exe"

local_conf='config/'
linode_conf='/home/dsicilia/dev/revolution-now/config/'
get_dir  "$linode_conf" "$local_conf"

log "checking result..."
[[ -x "$local_exe" ]] || die "result ($local_exe) is not executable."

log "loading env-vars..."
env_vars=.builds/current/env-vars.sh
[[ -f "$env_vars" ]] || die "$env_vars file does not exist."
source "$env_vars"
export LSAN_OPTIONS='print_suppressions=false,suppressions=tools/lsan.suppressions'

log "done. running..."
exec $local_exe