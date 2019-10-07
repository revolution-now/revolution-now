#!/bin/bash
[[ "$(uname)" == Darwin ]] && osx=1 || osx=0

(( osx )) && lld= || lld='--lld'

[[ "$1" == '--print-only' ]] && print_only=1

c_norm="\033[00m"
c_green="\033[32m"
c_red="\033[31m"

log() {
    echo -e "[$(date)] ${c_green}INFO${c_norm} $*"
}

error() {
    echo -e "[$(date)] ${c_red}ERROR${c_norm} $*" >&2
}

die() {
  error "$*"
  exit 1
}

run_for_flags() {
  flags="$@"
  flags="$flags --generator=ninja"
  if (( print_only )); then
    echo "cmc $flags"
    return
  fi
  log "configuration: $flags"
  if ! cmc $flags; then
    die "configure failed for flags: $flags"
  fi

  if ! make all; then
    die "build failed for flags: $flags"
  fi

  if ! make test; then
    die "tests failed for flags: $flags"
  fi
}

for cc in --clang --gcc=current; do
  for opt in '' --release; do
    for asan in '' --asan; do
      [[ "$cc" =~ clang ]] && lld=$lld || lld=
      run_for_flags $cc $opt $asan $lld
    done
  done
done

# Do --lto just once since it can take a really long time.
run_for_flags --clang --lld --release --lto

# Restore to default devel flags.
(( print_only )) || cmc --cached --clang --lld --generator=ninja

log "success."
