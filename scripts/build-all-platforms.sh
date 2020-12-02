#!/bin/bash
[[ "$(uname)" == Darwin ]] && osx=1 || osx=0

(( osx )) && lld_default= || lld_default='--lld'

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
    return 2
  fi

  if ! make test; then
    error "tests failed for flags: $flags"
    return 3
  fi
  return 0
}

logfile="/tmp/build-all.log"

rm -f $logfile
echo -n >$logfile

# This function expects a bunch of variables to be set.
run_for_args() {
  [[ "$cc" =~ clang ]] && lld=$lld_default || lld=
  run_for_flags $cc $lib $opt $asan $lld $lto
  code=$?
  if (( code == 0 )); then
    # Success.
    status="${c_green}SUCCESS${c_norm}"
    platform="$(cmc st | awk '{print $2}')"
  elif (( code == 1 )); then
    # Configure failed.
    status="${c_red}FAILURE:configuration${c_norm}"
    # We need to craft our own platform string because
    # `cmc` has presumably not done so for us since it
    # failed.
    platform="$(echo $cc,$lib,$opt,$asan,$lld,$lto \
      | sed -r 's/,+/,/g; s/(.*),$/\1/; s/--//g')"
  elif (( code == 2 )); then
    # Build failed.
    status="${c_red}FAILURE:build${c_norm}"
    platform="$(cmc st | awk '{print $2}')"
  elif (( code == 3 )); then
    # Testing failed.
    status="${c_red}FAILURE:test${c_norm}"
    platform="$(cmc st | awk '{print $2}')"
  else
    status="${c_red}FAILURE:unknown${c_norm}"
    platform="$(echo $cc,$lib,$opt,$asan,$lld,$lto \
      | sed -r 's/,+/,/g; s/(.*),$/\1/; s/--//g')"
  fi
  echo "$platform $status" >> $logfile
}

# for cc in --clang '' --gcc=current; do
for cc in --clang --gcc=current; do
  for lib in '' --libstdcxx --libcxx; do
    for opt in ''; do # --release; do
      for asan in ''; do # --asan; do
        [[ "$cc" =~ gcc && "$lib" =~ libcxx ]] && continue
        [[ "$cc" == ""  && "$lib" =~ libcxx ]] && continue
        run_for_args
      done
    done
  done
done

# Do --lto just once since it can take a really long time.
cc=--clang; lib=; opt=--release; asan=; lto=--lto;
# run_for_args

# Restore to default devel flags.
(( print_only )) || cmc --cached --clang --lld --asan

print_table() {
  {
    echo "Configuration Result"
    cat $logfile
  } | column -t
}

clear
echo '--------------------------------------------------------------------------------------'
echo -e "$(print_table)"
echo '--------------------------------------------------------------------------------------'
