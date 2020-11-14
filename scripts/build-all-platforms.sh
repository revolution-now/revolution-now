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
  if (( print_only )); then
    echo "cmc $flags"
    return
  fi
  log "configuration: $flags"
  if ! cmc $flags; then
    error "configure failed for flags: $flags"
    return 1
  fi

  if ! make all; then
    error "build failed for flags: $flags"
    return 1
  fi

  if ! make test; then
    error "tests failed for flags: $flags"
    return 1
  fi
  return 0
}

logfile="/tmp/build-all.log"

rm -f $logfile
echo -n >$logfile

prev_platform=

for cc in --clang --gcc=system --gcc=current; do
  for lib in '' --libstdcxx --libcxx; do
    for opt in '' --release; do
      for asan in '' --asan; do
        [[ "$cc" =~ clang ]] && lld=$lld || lld=
        platform="$(cmc st | awk '{print $2}')"
        run_for_flags $cc $lib $opt $asan $lld     \
            && status="${c_green}SUCCESS${c_norm}" \
            || status="${c_red}FAILURE${c_norm}"
        if [[ "$platform" == "$prev_platform" ]]; then
          # failure
          platform="$(echo unknown:$cc,$lib,$opt,$asan,$lld \
            | sed -r 's/,+/,/g; s/(.*),$/\1/; s/--//g')"
        else
          prev_platform="$platform"
        fi
        echo "$platform $status" >> $logfile
      done
    done
  done
done

# Do --lto just once since it can take a really long time.
run_for_flags --clang --lld --release --lto

# Restore to default devel flags.
(( print_only )) || cmc --cached --clang --lld --asan

print_table() {
  {
    echo "Configuration Result"
    cat $logfile | sort
  } | column -t
}

clear
echo '--------------------------------------------------------------------------------------'
echo -e "$(print_table)"
echo '--------------------------------------------------------------------------------------'
