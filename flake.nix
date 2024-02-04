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

    name = "scannedImageExtractor";
  in {
    packages.${system} = {
      default = let
        desktopItem = pkgs.makeDesktopItem {
          inherit name;

          exec = name;
          desktopName = "Scanned Image Extractor";
          terminal = false;
          icon = name;
          mimeTypes = ["x-content/image-dcf"];
          categories = ["Graphics" "Photography" "GNOME" "KDE"];
        };
      in
        pkgs.gccStdenv.mkDerivation {
          inherit name;
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            opencv3
            liblbfgs
            qt5.full
            libsForQt5.qt5.wrapQtAppsHook
          ];

          configurePhase = ''
            cmake scannerExtract -DCMAKE_BUILD_TYPE=release
          '';

          buildPhase = ''
            make
          '';

          installPhase = ''
            install -m 555 ${name} -Dt $out/bin

            # Install .desktop file
            install -m 444 ${desktopItem}/share/applications/${name}.desktop -Dt $out/share/applications

            # Install icons
            for dimension in 128 256; do
              install -m 444 $src/scannerExtract/ico/${name}''${dimension}x''${dimension}.png -D $out/share/icons/hicolor/''${dimension}x''${dimension}/apps/${name}.png
            done
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

    apps.${system} = {
      appImage = {
        program = "${pkgs.writeShellScript "scannedImageExtractor" ''
          ${pkgs.appimage-run}/bin/appimage-run ${self.packages.${system}.appImage}/${name}.AppImage
        ''}";
        type = "app";
      };
    };

    devShells = {
      default = pkgs.mkShellNoCC {
        packages = with pkgs; [
          cmake
          opencv3
          liblbfgs
          gt5.full

          appimage-run
        ];
      };
    };
  };
}
