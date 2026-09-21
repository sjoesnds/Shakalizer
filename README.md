# Shakalizer

A VST3 effect for FL Studio focused on aggressive digital destruction, bit crushing, sample-rate reduction, clipping, glitch holds, jitter and lo-fi filtering.

## Sound

Shakalizer is built around deliberately mangled digital texture: hard digital clipping, quantization, sample-and-hold style decimation, short glitch events and tone shaping. It is an original effect rather than a recreation of a specific artist or proprietary plugin.

## Controls

- DESTROY — global destruction macro
- CRUSH — bit-depth reduction
- DECIMATE — sample-and-hold / sample-rate destruction
- DRIVE — pre-shaper gain
- CLIP — hard clipping
- GLITCH — short digital hold/stutter events
- JITTER — randomizes sample-and-hold lengths
- TONE — post-destruction low-pass
- MIX — dry/wet
- OUTPUT — final gain
- MODE — Clean / Crunch / Shakal / Destroy / Fried

RANDOMIZE generates fast experimental combinations.

## Build

Windows:

    cmake -S . -B build-shakalizer
    cmake --build build-shakalizer --config Release --parallel

GitHub Actions builds and uploads a Windows VST3 artifact automatically.
