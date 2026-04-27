{
  description = "Hyprland plugin that confines the cursor to the active monitor";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      packages.${system}.default = pkgs.callPackage ./default.nix {};

      overlays.default = final: prev: {
        hyprfence = final.callPackage ./default.nix {};
      };
    };
}
