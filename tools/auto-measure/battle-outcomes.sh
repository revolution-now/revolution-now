#!/bin/bash
set -e

# ---------------------------------------------------------------
# Imports.
# ---------------------------------------------------------------
source ~/dev/utilities/bashlib/util.sh

:() { eval "$1=\"$3\""; }

# ---------------------------------------------------------------
# Parameters.
# ---------------------------------------------------------------
: training        = 1
: experiment_name = frigate-attacks-merchantman
: key_delay       = 50
: target_trials   = 500

: evade_md5            = -
: attacker_damaged_md5 = -
: defender_damaged_md5 = -
: attacker_sunk_md5    = -
: defender_sunk_md5    = -

# ---------------------------------------------------------------
# Initialization.
# ---------------------------------------------------------------
log_file="$experiment_name.txt"

this="$(readlink -f $0)"

find_window_named() {
  local regex="$1"
  local var="$2"
  local id=$(xdotool search --name "$regex")
  [[ -z "$id" ]] && die "failed to find window matching '$regex'."
  eval "$var=\"$id\""
}

script_win="$(xdotool getactivewindow)"
find_window_named 'DOSBox.*OPENING' dosbox

num_trials=target_trials
if [[ -e "$log_file" ]]; then
  existing=$(cat "$log_file" | wc -l)
  (( existing >= target_trials )) && {
    echo 'Target trials already achieved. exiting.'
    exit 0
  }
  num_trials=$(( target_trials-existing ))
  echo "Partial results found. Running $num_trials more times."
fi

# ---------------------------------------------------------------
# General X commands.
# ---------------------------------------------------------------
keys() { xdotool key --delay=$key_delay --window $dosbox "$@"; }

# ---------------------------------------------------------------
# Game specific composite commands.
# ---------------------------------------------------------------
# Loads the game from COLONY07.SAV.
load_game() {
  keys alt+g
  keys Down Down Down Down Down Return
  keys Up Up Up Return # Select COLONY07.SAV.
  sleep .5 # Wait for game to load.
  keys Return # Close popup.
}

# Saves the game to COLONY06.SAV.
save_game() {
  keys alt+g
  keys Down Down Down Down Return
  keys Up Up Return # Select COLONY06.SAV.
  sleep 1 # Wait for game to save.
  keys Return # Close popup.
}

# Attack the ship to the right of the blinking unit.
attack_right() {
  keys Right
  sleep 1.5
  keys Return # Close popup.
  sleep 1.5
}

exit_game() {
  keys alt+g
  keys Up Return # Select "Exit to DOS".
  keys Down Return # Select Yes.
}

# ---------------------------------------------------------------
# Result detection.
# ---------------------------------------------------------------
compute_md5sum() {
  command md5sum -b \
    ~/games/colonization/data/MPS/COLONIZE/COLONY06.SAV \
    | cut -d' ' -f1
}

record_outcome() {
  local md5=$(compute_md5sum)
  case $md5 in
    $evade_md5)
      echo 'evade' ;;
    $attacker_damaged_md5)
      echo 'attacker_damaged' ;;
    $defender_damaged_md5)
      echo 'defender_damaged' ;;
    $attacker_sunk_md5)
      echo 'attacker_sunk' ;;
    $defender_sunk_md5)
      echo 'defender_sunk' ;;
    *)
      echo "unknown: $md5" ;;
  esac
}

# ---------------------------------------------------------------
# Main loop.
# ---------------------------------------------------------------
for (( i=0; i<$num_trials; i++ )); do
  load_game
  attack_right
  save_game
  if (( training )); then
    outcome="$(record_outcome)"
    echo "$outcome"
    if [[ "$outcome" =~ ^unknown ]]; then
      xdotool windowfocus $script_win
      read
      # The user is assumed to have edited this script to add the
      # hash for the appropriate outcome. Now restart this script
      # to pick it up.
      exec "$this"
    fi
  else
    record_outcome >> "$log_file"
    sleep 1
  fi
done

exit_game