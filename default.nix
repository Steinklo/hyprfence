{ lib, hyprland, cmake, pkg-config }:

hyprland.stdenv.mkDerivation {
  pname = "hyprfence";
  version = "0.1";
  src = ./.;

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [ hyprland ] ++ hyprland.buildInputs;

  meta = {
    description = "Hyprland plugin that confines the cursor to the active monitor";
    homepage = "https://github.com/steinklo/hyprfence";
    license = lib.licenses.bsd3;
    platforms = [ "x86_64-linux" ];
  };
}
