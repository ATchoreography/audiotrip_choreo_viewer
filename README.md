# Audio Trip Choreography Viewer

## Work in progress!

Reads an ATS files and renders it.

ATS parser adapted from [hn3000/ats-types](https://github.com/hn3000/ats-types).

### Building

This project depends on [raylib](https://www.raylib.com) and [jsoncpp](https://github.com/open-source-parsers/jsoncpp/).

Build with CMake:

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Run it in the same directory as `barrier.obj` to load the barrier model.
