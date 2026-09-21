# Shakalizer v0.3

Shakalizer is a VST3 sound-destruction effect for FL Studio. The design goal is controlled digital damage that can stay musical instead of collapsing into generic noise.

## Core

- SHAKAL macro
- DESTROY / CRUSH / DECIMATE
- Oversampled DRIVE / CLIP
- sparse GLITCH with selectable grid
- JITTER / UNSTABLE movement
- TRANSIENT / BODY envelope-aware shaping
- frequency SPLIT for low-end protection
- STEREO width and channel divergence
- post filter: LP / BP / HP
- Auto Match and A/B

## Shatter engine

- SPECTRAL SHATTER
- LOW / MID / HIGH / AIR independent shatter amounts
- WAVE FOLD
- SHIFT sideband/ring-modulation stage
- RESONATOR / comb feedback
- envelope following
- CHARACTER macro

## Movement and sync

- Sine / Triangle / Sample+Hold / Stepped movement
- Free / 1/4 / 1/8 / 1/16 / 1/32 host-tempo sync
- Free / 1/8 / 1/16 / 1/32 glitch grid

## Modes

Clean, Crunch, Shakal, Destroy, Fried, Pixel, Alien, Melt and Shatter.

## Quality

1x, 2x and 4x nonlinear oversampling modes are available. Higher-quality modes are intended to reduce aliasing from the nonlinear stage.

## RANDOMIZE ALL

The RANDOMIZE ALL button iterates over the plugin's complete parameter list. It randomizes:

- every continuous control
- every mode/algorithm selector
- oversampling quality
- tempo-sync mode
- glitch grid
- Auto Match

A/B snapshots also capture the complete current parameter state.

## Build

Windows:

    cmake -S . -B build-shakalizer
    cmake --build build-shakalizer --config Release --parallel

GitHub Actions builds and uploads a Windows VST3 artifact for source/CMake/workflow changes.
