#!/bin/bash
# This is a temporary script used to move a local copy from the
# dpacbach github account to the revolution-now org.
set -eo pipefail

git remote remove gitlab
git remote remove origin
git remote add origin ssh://git@github.com/revolution-now/revolution-now
git remote add gitlab ssh://git@gitlab.com/dpacbach/revolution-now

git checkout master
git pull origin master
git submodule update --init
git submodule sync
git submodule update --init