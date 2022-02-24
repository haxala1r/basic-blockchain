# basic-blockchain
This is a simple test app I made to learn more about the blockchain algorithm and
the peer-to-peer network structure.

Although it isn't perfect, it correctly implements a simple blockchain. Each block in the
blockchain contains exactly 256 bytes of data, regardless of what it may be (since the app
only really works from the commandline, most data is going to be human-readable strings, but anyway).

The hash algorithm used is SHA-256, implemented by me.

# Building
If you're on linux or another UNIX-like system, you can simply run the `build.sh` build script.

If you're on windows, install cywgin and mingw, and *then* run the build script. It's not very complex anyways.
