{
  description = "";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    packages.${system} = {
      default = pkgs.stdenvNoCC.mkDerivation {
        name = "scannerExtract";
        src = ./.;

        buildInputs = with pkgs; [
          opencv3
          liblbfgs
          qt6.full
        ];

        buildPhase = ''
          mkdir $out
          cd $out
          cmake $src/scannerExtract/ -DCMAKE_BUILD_TYPE=release
          make
          # (make install)
        '';
      };
    };
  };
}
# http://www.dominik-ruess.de/scannerExtract/

