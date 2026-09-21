# Shakalizer v0.2

A VST3 effect for FL Studio focused on controlled digital destruction rather than generic distortion.

## Processing

- 4x oversampled nonlinear stage for cleaner distortion.
- Multiple resampling styles: Hold, Linear, Stair, Smear and Random.
- Bit-depth reduction with TPDF dither.
- Hard/soft clipping blend with level compensation.
- Frequency split to protect the low end while destroying the upper content.
- Envelope-aware processing with separate Transient and Body controls.
- Stereo widening plus independent channel movement.
- Movement modulation with Sine, Triangle, Sample+Hold and Stepped shapes.
- Unstable slow random modulation.
- Alien ring-modulation mode.
- Three post filter types: Low Pass, Band Pass and High Pass.
- Sparse glitch/freeze events.
- Auto Match loudness compensation.
- A/B snapshot swapping.
- Random parameter generator.

## Modes

Clean, Crunch, Shakal, Destroy, Fried, Pixel, Alien and Melt.

The sound target is an original digital-destruction tool for aggressive SoundCloud-style production workflows; it is not a recreation of a particular artist or proprietary plugin.

## Build

Windows:

    cmake -S . -B build-shakalizer
    cmake --build build-shakalizer --config Release --parallel

GitHub Actions builds and uploads the Windows VST3 artifact automatically.

The current CI workflow only runs for source/CMake/workflow changes, not README-only commits.
