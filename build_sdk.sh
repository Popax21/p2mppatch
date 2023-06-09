#!/bin/sh -e
export STEAM_RUNTIME_PATH="$PWD/runtime"
cd sdk/mp/src/
./createallprojects
pushd tier1 && make -f tier1_linux32.mak && popd