{
  description = "raven — pixel art roguelike game";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };
      in
      {
        devShells.default = pkgs.mkShell {
          name = "raven-dev";

          nativeBuildInputs = with pkgs; [
            # Build toolchain
            gcc14
            cmake
            ninja
            pkg-config
            gnumake

            # C++ tooling
            clang-tools     # clangd, clang-format, clang-tidy
            cppcheck
            include-what-you-use

            # Debugging & profiling
            gdb
            valgrind
          ] ++ lib.optionals stdenv.isLinux [
            linuxPackages.perf
          ] ++ [
            renderdoc       # GPU frame debugging
            tracy           # Frame profiler (great for game dev)

            # Pixel art & asset tools
            libresprite     # Open-source Aseprite fork
            ldtk            # Level editor
            audacity        # Audio editing
            ffmpeg          # Audio/video conversion

            # Misc dev tools
            git
            direnv
            just            # Command runner (like make but nicer)
          ];

          buildInputs = with pkgs; [
            # Core game dependencies (system-level)
            SDL2
            SDL2_image
            SDL2_mixer
            SDL2_ttf

            # System libs SDL needs
            libGL
            libGLU
            xorg.libX11
            xorg.libXext
            xorg.libXrandr
            xorg.libXi
            xorg.libXcursor
            xorg.libXinerama

            # Audio backends
            libpulseaudio
            alsa-lib
          ];

          shellHook = ''
            echo "🐦 raven dev environment loaded"
            echo "   gcc:   $(gcc --version | head -1)"
            echo "   cmake: $(cmake --version | head -1)"
            echo "   SDL2:  $(pkg-config --modversion sdl2)"
            echo ""
            echo "Commands:"
            echo "   just build     — configure + build"
            echo "   just run       — build + run game"
            echo "   just test      — run tests"
            echo "   just clean     — nuke build dir"
            echo ""

            # Ensure SDL can find audio/video drivers
            export SDL_VIDEODRIVER=''${SDL_VIDEODRIVER:-x11}
          '';

          # For clangd to find system headers
          CMAKE_EXPORT_COMPILE_COMMANDS = "ON";
        };

        # Buildable package
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "raven";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = with pkgs; [ cmake ninja pkg-config ];
          buildInputs = with pkgs; [
            SDL2 SDL2_image SDL2_mixer SDL2_ttf
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DRAVEN_ENABLE_TESTS=OFF"
          ];
        };
      });
}
