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
: experiment_name = native-muskets/conquistador-agrarian-old-brave
: key_delay       = 50
: target_trials   = 1000

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
  # TODO
}

# ---------------------------------------------------------------
# Action.
# ---------------------------------------------------------------
action() {
  # This action is to be run when there is a single dwelling with
  # a regular brave right on top of it and where the tribe has
  # one unit of muskets to potentially give to the brave on the
  # very next turn.

  # Advance the turn to make the game consider giving muskets to
  # the brave.
  keys Return
  sleep 1.5
}

# ---------------------------------------------------------------
# Main loop.
# ---------------------------------------------------------------
for (( i=0; i<$num_trials; i++ )); do
  load_game
  action
  save_game
  record_outcome >> "$log_file"
  sleep 1
done

exit_game