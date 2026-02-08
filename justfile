# raven — development commands

mod book 'docs/book.just'
mod api 'docs/api.just'

default:
    @just --list

# Configure and build (Debug)
build:
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cmake --build build -j$(nproc)
    @# Symlink compile_commands.json for clangd
    @ln -sf build/compile_commands.json .

# Configure and build (Release)
release:
    cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DRAVEN_ENABLE_TESTS=OFF
    cmake --build build-release -j$(nproc)

# Build and run the game
run: build
    ./build/bin/raven

# Run tests
test: build
    cd build && ctest --output-on-failure

# Run with Address Sanitizer
asan:
    cmake -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DRAVEN_ENABLE_ASAN=ON
    cmake --build build-asan -j$(nproc)
    ./build-asan/bin/raven

# Format all source files
fmt:
    find src tests -name '*.hpp' -o -name '*.cpp' | xargs clang-format -i

# Run static analysis
lint:
    find src -name '*.hpp' -o -name '*.cpp' | xargs clang-tidy -p build

# Clean build artifacts
clean:
    rm -rf build build-release build-asan

# Watch for changes and rebuild (requires entr)
watch:
    find src -name '*.hpp' -o -name '*.cpp' | entr -c just run
