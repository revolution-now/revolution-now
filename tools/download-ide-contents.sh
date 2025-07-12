#!/bin/bash
set -e

echo 'Select local host to download rn.lua from:'
ip="$(~/dev/utilities/network/choose-host.sh)"

[[ -n "$ip" ]]

rn_contents=~/dev/revolution-now/tools/ide/contents/rn.lua
[[ -n "$rn_contents" ]]
[[ -e "$rn_contents" ]]

scp "$ip:$rn_contents" "$rn_contents"