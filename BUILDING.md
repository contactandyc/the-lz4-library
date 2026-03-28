# BUILDING

This project: **The LZ4 Library**
Version: **0.0.2**

## Local build

```bash
# one-shot build + install
./build.sh install
```

Or run the steps manually:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j"$(nproc || sysctl -n hw.ncpu || echo 4)"
sudo cmake --install .
```



## Install dependencies (from `deps.libraries`)



### Development tooling (optional)

```bash
sudo apt-get update && sudo apt-get install -y valgrind gdb perl autoconf automake libtool
```



### a-memory-library

Clone & build:

```bash
git clone --depth 1 --single-branch "https://github.com/contactandyc/a-memory-library.git" "a-memory-library"
cd "a-memory-library"
./build.sh clean
./build.sh install
cd ..
rm -rf "a-memory-library"
```

