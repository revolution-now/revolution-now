#!/bin/bash
[[ "$(uname)" == Darwin ]] && osx=1 || osx=0

(( osx )) && lld= || lld='--lld'

[[ "$1" == '--print-only' ]] && print_only=1

build_threads() {
  local threads=4
  if [[ "$(uname)" == Linux ]]; then
    threads=$(nproc --all)
  elif [[ "$(uname)" == Darwin ]]; then
    threads=$(sysctl -n hw.ncpu)
  fi
  [[ -z "$threads" ]] && return 1
  echo "$threads"
  return 0
}

remove_spaces() {
  echo $*
}

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

for cc in --clang --gcc=current; do
  for opt in '' --release; do
    for asan in '' --asan; do

      maybe_lld=
      maybe_lto=
      if [[ "$cc" =~ clang ]]; then
        maybe_lld=$lld
        [[ "$opt" =~ release ]] && maybe_lto='--lto'
      fi

      config_flags="$cc $opt $asan $maybe_lld $maybe_lto"
      config_flags="$(remove_spaces $config_flags)"

      (( print_only )) && {
        echo "cmc $config_flags"
        continue
      }

      log "configuration: $config_flags"

      if ! cmc $config_flags; then
        die "configure failed for flags: $config_flags"
      fi

      if ! make all -j$(build_threads); then
        die "build failed for flags: $config_flags"
      fi

      if ! make test; then
        die "tests failed for flags: $config_flags"
      fi

    done
  done
done

cmc --clang $lld

log "success."
