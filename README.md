# Power Struggle
N64Brew Game Jam #2 Project

A game where you play as a robot that can take control of other robots.

# Building

Make sure you've clone the repo recursively. If you haven't, run `git submodule update --init --recursive` to clone any required submodules.

Beyond the usual suspects, the packages n64graphics and vadpcm_enc must be installed from Crash's Modern SDK. You will also need the `convert` utility which is provided by the `imagemagick` package.

Finally you will need a custom build of `gcc-mips-n64` with full C++ support, as the current package is incomplete in that regard. A download for a pre-built version will be added to this repo.

Once that's all set up, simply run `make` with an optional job count to build the rom.
