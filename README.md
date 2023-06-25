# Portal 2 Multiplayer Patch Plugin

This repository contains the source code for a Source engine plugin which is
aimed at allowing an arbitrary number of players to join and play in both the
singleplayer and cooperative campaign at the same time. It does so using only
generic patches, without the need for any map-specific support scripts, as well
as by only making the minimum number of changes required for the maps to be
playable.

Currently, the plugin only works on Linux, however Windows support is planned to
be added eventually.

Patches include:
- removing the player cap from all multiplayer game session
- removing the partner-disconnect check
- a rewritten transition ready-tracking system, which ensures that map
  transitions work with an arbitrary number of players both in singleplayer as
  well as in coop maps
- changes to the spawning logic for singleplayer maps when played in a
  multiplayer session: `point_teleport` triggers will teleport all players when
  they're targeting `!player`. They will also update the spawnpoint as well,
  anchoring it to any moving surfaces. This causes the start elevator to work
  properly with multiple players.
- changes to the stuck-handling logic: if multiple players are stuck within each
  other, they temporarily loose collision with each other until they become unstuck
- for singleplayer maps: doors will open and close multiple times, to ensure
  that all players can go through them (including after respawns)
- `env_fade` entities will never affect all players in single player maps

## Building
- ensure you have cloned the repository recursively - if not, execute `git
  submodule init` and `git submodule update`
- download and setup the Steam runtime using the `./setup_runtime.sh` script,
  ensuring that you select the 32-bit i386 architecture
- build the SDK using the `./build_sdk.sh` script
- build the plugin by executing `make` in the `plugin` directory
- copy the resulting `p2mppatch.so` file into your game's `portal2` directory

## Usage
Simply load the plugin using the `plugin_load p2mppatch` console command **while
on the main menu** - otherwise there is a high chance the game will crash. You
can unload the plugin using `plugin_unload p2mppatch` once you're done using it.
If the plugin is not loaded, your game will be unmodified, however the plugin
was also designed to not interfere with regular gameplay even when loaded.

Once the plugin is loaded, start a regular CoOp session with one of the people
you intend to play with. Afterwards, the other players can join using the
`connect <ip>` console command (ensure that you have ports 27015 and 27031-27036
forwarded).