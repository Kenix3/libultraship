# CoreAudio crackling repro

Standalone reproducer for the macOS CoreAudio backend bugs that caused
glitchy/crackling sound (see `src/ship/audio/CoreAudioAudioPlayer.cpp`).

The reproducer lifts the ring-buffer logic out of the audio backend into two
classes -- a `BuggyPlayer` that mirrors the original implementation byte for
byte, and a `FixedPlayer` that mirrors the patched lock-free SPSC ring
buffer -- and runs three deterministic scenarios against both. The diff
between the two columns is the proof that the patch removes the bugs.

## Build & run

```
clang++ -std=c++17 -O2 -pthread repro.cpp -o repro
./repro
```

## Expected output

```
exact-fill wrap        buggy: dropouts= 6000 ...   fixed: dropouts=    0 ...
overflow chunk drop    buggy: chunkDrops=   1 ...  fixed: partial=    1 ...
32 fill/drain cycles   buggy: dropouts=192000 ...  fixed: dropouts=    0 ...

[OK] buggy implementation reproduces crackling artifacts
[OK] fixed implementation: clean stream, no dropouts/discontinuities
```

## What each scenario shows

- **exact-fill wrap**: write a full ring's worth of frames in one shot. The
  buggy code lets `write % size` land exactly on `read`, so `Buffered()` reads
  0 and the render callback emits silence -- entire ring lost. The fix
  reserves one frame so `write == read` always means empty.
- **overflow chunk drop**: try to push 500 frames into 100 frames of free
  space. The buggy code discards the whole chunk; the fix writes what fits
  and only drops the tail.
- **32 fill/drain cycles**: hammer the wrap-equality state 32 times. Buggy
  code loses every cycle (32 * 6000 frames). Fix is lossless.
