#!/bin/bash
# This script is run from vim to make a target. It will create a
# split window and run the command inside of it. If it fails with
# an error then it will scroll to the top of the window and posi-
# tion the cursor on the first error.
target=$1
[[ -z "$target" ]] && exit 1

[[ -d ~/dev/revolution-now ]] && d=~/dev/revolution-now \
                              || d=~/dev/rn
cd $d

cmd="make $target NINJA_STATUS_PRINT_MODE=singleline"
[[ "$target" == o ]] && cmd="scripts/o.sh $2"

pane_size=1

tmux resize-pane -y $pane_size

$cmd; status=$?

# We need to move the cursor up to the pane that is running this
# command because otherwise if the cursor is in vim (i.e., in the
# other pane) when this script exits then for some strange reason
# vim decides to show the console behind the vim files, which is
# not relevant for anything. Note that even without this, the
# tmux commands below would still work.
tmux select-pane -U

(( status == 0 )) && exit 0

# Build has failed.

expanded_pane_size=30
tmux resize-pane -y $expanded_pane_size # so that we can see the error.

# Go to the top of the buffer and search for the first
# instance of "error:" to show the first error.
tmux copy-mode
tmux send-keys -X top-line
tmux send-keys -X search-forward "error:"

# Move the "error:" line to the top line in the pane.
for (( i=1; i < $expanded_pane_size; i++ )); do tmux send-keys -X scroll-down; done
for (( i=1; i < $expanded_pane_size; i++ )); do tmux send-keys -X cursor-up; done

# Move cursor so that it is not over the first letter of "er-
# ror:".
tmux send-keys -X cursor-left

read