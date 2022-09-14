#!/bin/sh -e
wget -O - https://media.steampowered.com/client/runtime/steam-runtime-sdk_2013-09-05.tar.xz | tar -xJv
mv steam-runtime-sdk_2013-09-05 runtime
cd runtime
./setup.sh