#!/bin/bash
set -eo pipefail

source ~/dev/utilities/bashlib/util.sh

[[ -d .builds ]] || die 'must run from rn root folder.'

remote="$1"

[[ -n "$remote" ]] || die 'must specify remote'

# FIXME: need to fail if we are asked to rsync a file that con-
# tains local changes as recognized by git (timestamps is not a
# good indicator). Otherwise this will just overwrite any local
# changes.

get_file() {
  local from="$1"
  local to="$2"
  debug "creating local destination: $(dirname "$to")"
  mkdir -p "$(dirname "$to")"
  info "pulling remote file: $from"
  rsync --checksum "dsicilia@$remote:$from" "$to"
}

get_dir() {
  local from="$1"
  local to="$2"
  debug "creating local destination: $to"
  mkdir -p "$to"
  info "pulling remote folder: $from"
  rsync --checksum --recursive "dsicilia@$remote:$from" "$to"
}

ssh "$remote" 'fish -c "cd /home/dsicilia/dev/revolution-now && make all REMOTE=1"'

local_exe='.builds/from-remote/exe/exe'
remote_exe='/home/dsicilia/dev/revolution-now/.builds/current/exe/exe'
get_file "$remote_exe" "$local_exe" &

local_conf='config/'
remote_conf='/home/dsicilia/dev/revolution-now/config/'
get_dir  "$remote_conf" "$local_conf" &

local_lua='src/lua/'
remote_lua='/home/dsicilia/dev/revolution-now/src/lua/'
get_dir  "$remote_lua" "$local_lua" &

local_key='keys/game-exchange-key.rcl'
remote_key='/home/dsicilia/dev/revolution-now/keys/game-exchange-key.rcl'
get_file "$remote_key" "$local_key" &

# Fragment shader.
local_frag='src/render/generic.frag'
remote_frag='/home/dsicilia/dev/revolution-now/src/render/generic.frag'
get_file "$remote_frag" "$local_frag" &

# Vertex shader.
local_vert='src/render/generic.vert'
remote_vert='/home/dsicilia/dev/revolution-now/src/render/generic.vert'
get_file "$remote_vert" "$local_vert" &

wait

debug "checking result..."
[[ -x "$local_exe" ]] || die "result ($local_exe) is not executable."

debug "loading env-vars..."
env_vars=.builds/current/env-vars.sh
[[ -f "$env_vars" ]] || die "$env_vars file does not exist."
source "$env_vars"
export LSAN_OPTIONS='print_suppressions=false,suppressions=tools/lsan.suppressions'

log "done. running..."
exec $local_exe