# hyprfence

A [Hyprland](https://hyprland.org) plugin that confines the cursor to the active monitor. No more accidental mouse drift — switch monitors only through workspace keybinds.

## How it works

**Cursor stays put.** The mouse cannot cross monitor boundaries. Switch monitors by switching to a workspace on another monitor via your keybinds.

## Install

### NixOS (flake)

```nix
# flake.nix
inputs.hyprfence = {
  url = "github:steinklo/hyprfence";
  inputs.nixpkgs.follows = "nixpkgs";
};
```

```nix
# home.nix
wayland.windowManager.hyprland.plugins = [
  inputs.hyprfence.packages.${pkgs.system}.default
];
```

### hyprpm

```sh
hyprpm add https://github.com/steinklo/hyprfence
hyprpm enable hyprfence
```

### Building from source

```sh
cmake -B build
cmake --build build
```

The resulting `libhyprfence.so` can be loaded with `hyprctl plugin load <path>`.

## Config

```ini
plugin {
  hyprfence {
    enabled = true  # set to false to disable on startup
  }
}
```

### Toggle at runtime

Bind a key to toggle the fence on/off:

```ini
bind = SUPER, G, hyprfence:toggle,
```

Or toggle from the command line:

```sh
hyprctl dispatch hyprfence:toggle
```

## Known limitations

- x86_64 only (Hyprland function hooks require x86_64)
- Drag-and-drop across monitors is blocked while enabled
- Tablet/touch input is not confined (only mouse pointer)

## Requirements

- Hyprland 0.54+

## License

BSD-3-Clause
