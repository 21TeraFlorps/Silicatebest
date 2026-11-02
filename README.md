# Silicate

This is the main repository for the Silicate bot.

## End of life notice

Silicate is being rewritten (version 2). This version will be maintained enough to not have breaking issues until v2 is fully ready.

## Structure

```
src/
    assist/ - Assist features, such as autoclicker or hitboxes.
    bot/ - Core bot components.
    checkpoint/ - The practice fix.
    hooks/ - All of the hooks Silicate uses. Interacts with core game logic.
    label/ - The label system for displaying overlays.
    physics/ - Geometry Dash physics decomp for trajectory.
    render/ - The renderer and DSP recorder.
    replay/ - The replay system.
    settings/ - The bot's settings module.
    shared/ - Shared parts of the code, such as keybind logic.
    trajectory/ - Simulation/trajectory logic.
    ui/ - The interface.
    util/ - Generic utilities, such as midhooking.
lib/
    tabby/ - The UI library, based on ImGUI.
```

## Compiling

1. Run `build.bat` or `build-rel.bat` if you're compiling for public use. (Use `build.sh` if you're on Linux!)

## Contributing

Please use feature branches. Use clang-format for formatting your code (unless it makes it horribly unreadable).
Currently we do not have automated testing. Please test the features you're implementing/changing and related components before releasing builds.
