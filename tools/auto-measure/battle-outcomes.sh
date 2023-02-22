#!/bin/bash
set -e

source ~/dev/utilities/bashlib/util.sh

:() { eval "$1=\"$3\""; }

# ---------------------------------------------------------------
# Parameters.
# ---------------------------------------------------------------
: training        = 1
: experiment_name = frigate-attacks-merchantman
: key_delay       = 50

: evade_md5            = 3b95c1c8e2d88ffdf4eb7725bf372121
: attacker_damaged_md5 = 58554aa46de7fd4f2241ada82f87fedf
: defender_damaged_md5 = db98f3a5fb82e386d23815625559659a
: attacker_sunk_md5    = 05a088b7ad8fe82965a7c7eea9c660ab
: defender_sunk_md5    = 4deb093647141161aced9b0fd437dc55

# ---------------------------------------------------------------
# Initialization.
# ---------------------------------------------------------------
log_file="$experiment_name.txt"

win=$(xdotool search --name DOSBox)
echo 'Click on the DOSBox window.'
win=$(xdotool selectwindow)
[[ -z "$win" ]] && die "failed to find DOSBox window."

if (( training )); then
  echo 'Click on the window running this script.'
  script_win=$(xdotool selectwindow)
  [[ -z "$script_win" ]] && die "failed to find script window."
fi

if (( !training )); then
  touch "$log_file"
  echo "Run './watch-log.sh $log_file' and press enter."
  read
fi

# ---------------------------------------------------------------
# General X commands.
# ---------------------------------------------------------------
keys() { xdotool key --delay=$key_delay --window $win "$@"; }

# ---------------------------------------------------------------
# Game specific composite commands.
# ---------------------------------------------------------------
# Loads the game from COLONY07.SAV.
load_game() {
  # Open Game menu.
  keys alt+g
  # Select "Load Game".
  keys Down Down Down Down Down Return
  # Select last real save slot.
  keys Up Up Up Return
  # Wait for game to load.
  sleep .1
  # Close the "Loaded COLONY07.SAV successfully" window.
  keys Return
}

# Saves the game to COLONY06.SAV.
save_game() {
  # Open Game menu.
  keys alt+g
  # Select "Load Game".
  keys Down Down Down Down Return
  # Select second-to-last save slot.
  keys Up Up Return
  # Wait for game to save.
  sleep 1
  # Close the "Saved COLONY06.SAV" window.
  keys Return
}

attack() {
  # Attack the ship to the right of the blinking unit.
  keys Right
  sleep 1.5
  keys Return
  sleep 1.5
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
while true; do
  load_game
  attack
  save_game
  if (( training )); then
    record_outcome
    xdotool windowfocus $script_win
    read
  else
    record_outcome >> "$log_file"
    sleep 1
  fi
done