#!/bin/bash
set -eo pipefail

source ~/dev/utilities/bashlib/util.sh

[[ -d .builds ]] ||
  die 'must run from rn root folder.'

local_dst='.builds/from-linode/exe/exe'
linode_src='linode:/home/dsicilia/dev/revolution-now/.builds/current/exe/exe'

log "creating local destination: $local_dst"
mkdir -p "$(dirname "$local_dst")"

log "pulling remote file: $linode_src"
scp "$linode_src" "$local_dst"

log "checking result..."
[[ -x "$local_dst" ]] ||
  die "result ($local_dst) is not executable."

log "done. running..."
$local_dst