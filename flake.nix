{
  description = "A tool for efficiently extracting rectangular photographs.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nix-appimage.url = "github:ralismark/nix-appimage";
  };

  outputs = {
    self,
    nixpkgs,
    nix-appimage,
  }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    packages.${system} = let
      name = "scannedImageExtractor";
    in {
      default = pkgs.gccStdenv.mkDerivation {
        inherit name;
        src = ./.;

        nativeBuildInputs = with pkgs; [
          cmake
          opencv2
          liblbfgs
          qt5.full
          libsForQt5.qt5.wrapQtAppsHook
        ];

        dontUseCmakeConfigure = true;
        buildPhase = ''
          cmake scannerExtract -DCMAKE_BUILD_TYPE=release -DOPENCV2=1
          make
        '';

        installPhase = ''
          install -m 555 ${name} -Dt $out/bin
        '';
      };

      appImage = let
        src = nix-appimage.bundlers.${system}.default self.packages.${system}.default;
      in
        pkgs.stdenvNoCC.mkDerivation {
          inherit name src;

          dontUnpack = true;
          dontBuild = true;

          installPhase = ''
            install -m 555 $src -D $out/${name}.AppImage
          '';
        };
    };

    devShells = {
      default = pkgs.mkShellNoCC {
        packages = with pkgs; [
          cmake
          opencv2
          liblbfgs
          gt5.full

          appimage-run
        ];
      };
    };
  };
}
