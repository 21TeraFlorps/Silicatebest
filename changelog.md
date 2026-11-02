# Silicate Changelog

## 1.1.0
- UI colors and sizes refreshed
- Assist and render UI reworked
  - Now related settings are grouped
- Some features now have tooltips explaining what they do
- Improved the hitbox trail algorithm to now properly deduplicate rendering based on pixels not on cocos2d points
- Added new `%difficulty%` renderer tag
- Removed ability to change Lock Delta mode (now it's always Accuracy)
- Keybind UI reworked
- Confirmation modals for overwriting replays and switching to Record mode mid level
- Added repeat frequency and hold delay settings for keybinds
- Added way to disable confirmation modals for overwriting replays
- Repeatedly using keybinds via holding and clicking your mouse does not stop the keybinds being repeated
- Redesigned disabled bot screen
- Fixed frame advance applying on the level load screen and not after the level actually loads
- Added confirmation modal to disabling overwriting replay confirmation modal
- Fixed hitboxes being wrong when rotating camera
- Slightly optimized hitboxes
- Reworked backwards stepping
  - Going backward now shows a ghost player
  - You may go forward / backward with your frame advance keybinds to move the trail back and forth
  - You may cancel the backstep by moving forward enough
  - You may confirm the backstep early by pressing your toggle frame advance button
  - The backstep is automatically confirmed after some time, and the save state at that frame is loaded
- Fixed circle hitboxes properly scaling when zooming in
- Fixed compatibility with Zoooom! mod and hitbox trail
- Added error modals for renderer failures and disabling bot when in level
- Made trajectory a tiny bit faster
- Fixed inconsistent randomness with teleport portals (aka it always just being the first portal that gets triggered lol)
- Made shake trigger randomness more consistent
- Added a warning modal to also enable Lock Delta if trying to enable backwards stepping without it
- Added a few checks to the renderer to disallow using invalid values
- Added a warning modal when disabling Lock Delta
- Renderer now properly scales background for weird aspect ratios
- Edit tab now doesn't draw items outside of view, so editing massive replays is lag free
- CPS is now saved between attempts with checkpoints
- Y velocity label now reacts to Velocity Unrounding having soft toggle on/off
- Downloading FFmpeg now also uses a more secure hash to verify file integrity
- Silicate now soft-toggles Superb Input Precision off
- Reworked autoclicker
  - Now settings apply per-player (there's an option to sync between both players)
  - Separated hold and release delays
  - Added option to perform multiple clicks per tick
- Fixed a few memory leaks and crashes
- Fixed labels global toggle not saving
- Fixed renderer hanging causing issues with window sizing
- Added a new theme
- Added something else... do not boop the cat.

## 1.0.2
- Fixed issues with replays not saving TPS properly sometimes
- Fixed rotating objects in editor not immediately updating the hitboxes
- Fixed backwards stepping occasionally going back 2 ticks in practice mode
- Fixed pause menu when rendering being the scale of the rendered canvas and not the window, and thus being inaccessible
- Updated the mod icon slightly

## 1.0.1
- Fixed being able to place checkpoints while dead
- Fixed utilities like Prevent Death being triggerable by multiple objects in one tick
- Fixed some replays not being loadable
- Fixed having weird window aspect ratios when rendering causing issues
- [EXPERIMENTAL] Added new "Visual FPS" option to renderer - allows you to divide large delta time updates into more, smaller dt updates
- Now when using the mod [Velocity Unrounding](<https://geode-sdk.org/mods/chizz.velocity-unrounding>), Y velocity label has higher precision (8 digits instead of 3)

## 1.0.0
- Added ability to keep labels on while rendering
- Added accurate CPS and Max CPS labels
  - They also flash red when exceeding 16 CPS on any player
  - Both labels work perfectly fine with speedhack, frame advance, renderer, etc.
- Added Teleport Random State label
- Added Time Warp, Ticks Since Last Input and Checkpoint Count labels
- Added renderer fade/in out options
- Added ability to automatically exit level after finishing rendering
- [EXPERIMENTAL] Added keybind UI
- Added SSB fix
- Added SSB factor label, replacing old SSB DT label
- Added new "Touching Orbs" label
  - Shows which orbs you're touching, and displays in order how you will trigger them when clicking
  - Does not show already activated orbs (unless they're multi activate)
  - With Toggle orbs, also shows the group that it'll toggle
  - Shows an asterisk after the name if it's multi activate
- Added option to show hitbox trail above player and rotated player hitboxes
- Fixed Frame Extrapolation not working with Lock Delta on
- Fixed Frame Extrapolation bugging wave trail out
- Fixed trajectory with dash orbs having legacy behavior
- Fixed going in and out of fullscreen breaking the UI
- Fixed unfocusing GD causing Silicate to sample an invalid FBO - causing the "window bug"
- Fixed the font atlas in UI being corrupted sometimes
- Fixed shift and ctrl being bugged in keybinds
- Fixed placed checkpoints' sprites not showing up until the next tick
- Fixed odd editor behavior when FPS != TPS, with Lock Delta on
- Fixed hitbox trail rendering improperly in dual mode after entering mirror portals
- Fixed levels such as Glungus Adventures not being deterministic because of teleport portal randomness
- Fixed editor playtest holding when starting playtest
- Fixed replaying low input macros having issues
- Fixed audio speedhack being toggleable when bot is off
- Fixed hitboxes being unable to show up above UI triggered objects
- Fixed player 2 inputs not being recorded when two player mode was off in a level (required for event triggers)
- Fixed rotated hitboxes in editor being wrong
- Fixed rotate trigger trajectory being wrong in some cases
- Fixed crash when closing the game after rendering anything
- Fixed TPS of replays being saved as the last TPS of the replay, not the first
- Fixed active interactable hitboxes triggering too early/late with rotated hitboxes
- Fixed active interactable hitboxes triggering even when disabled
- Improved renderer performance
- Improved hitbox performance
- Moved Frame Extrapolation into experimental features
- Hitbox trail can now store many more segments (69420 -> 999999)
- Frame advance now automatically turns off when entering level editor

## 1.0.0-alpha.87
- Significantly improved Renderer performance
- Reworked Hitbox Trail, should be much more performant with less trail skipping
- Fixed Frame Prediction
- Added Audio Preview to Renderer
- Collision blocks are now shown as Interactable hitboxes
- Interactable hitboxes that the player is intersecting now have a new "Interactable (Active)" category for which you can change the color
- Renderer no longer leaks resources after finished renders
- Improved Trajectory performance
- Added ability to see background in Layout Mode

## 1.0.0-alpha.86
- Added Dynamic UPR - now Silicate can slow the game down to reach an intended FPS target without fiddling with Max UPR manually
- Added a new label to display calculated UPR with Dynamic UPR
- Fixed a Backstepping memory leak
- Fixed Trajectory crashing with editor Save+Exit
- Fixed Input Editor being able to corrupt replays
- Fixed appending not pruning actions correctly
- Fixed Renderer not working with Cyrillic names
- Fixed TPS Changing being wrong by one tick

## 1.0.0-alpha.85
- Fixed Auto-Checkpoints breaking the bot
- Fixed playback in Editor
- (Hopefully) fixed Backwards Stepping causing inaccuracies (please test)
- Made the FFmpeg downloader compliant with Geode's guidelines (so we shouldn't get delisted anymore)
- Fixed Lock Delta off with Disable bot causing speed issues
- Added Noclip with player selector (Assist tab)

## 1.0.0-alpha.84
- Fixed FFmpeg downloader
- Fixed Backwards Stepping misaligning ticks
- Fixed Backwards Stepping going back 2 ticks in case of death
- Added ability to set fill opacity of Hitboxes
- Speedhack input now has greater precision
- Fixed Opacity in Labels going crazy
- Added Silicate button in pause menu (now has a dependency on Node IDs)
- Fixed dropdowns going outside of menu when scrolling
- Made CBS and COS automatically disable when entering a level
- Modified mod description, fixed Discord link
- Added Layout Mode
- Fixed rare crash with Trajectory and arrow triggers + slopes
- Fixed all *_qsv issues with chroma and luma misaligning
- Added ability to record 1st Attempt Pause (brief period when entering level)
- Added Player Circle Hitbox to Hitbox Trail
- Added ability to start rendering mid-level
- Interactable hitboxes now show touch triggered triggers

## 1.0.0-alpha.83
- Bumped Geode version to 5.0.0
- Fixed postprocess UI shader going twice as fast if dropdown was open
- Added "Seed Override" option to force a specific initial seed
- Pausing no longer releases inputs when replaying
- Fixed restarting level releasing all inputs when not in practice
- Fixed Silicate's behavior with Lock Delta off, high TPS
- Added "Open Silicate Folder" button in Settings
- Completely removed Scripting tab for now
- Fixed `h264_qsv` and `hevc_qsv` not working (Silicate now dynamically chooses between `yuv420p`, `nv12`, `rgb0` and `rgb24` pixel formats)
- Made dropdowns no longer run postprocess shaders
- Improved logging in some areas
- Advancing frames now turns on frame advance as well
- Made Edit tab no longer an experimental option
- Improved Edit tab UI
- Added ability to add/remove inputs in Edit tab
- Added ability to customize Music and SFX volume in renderer
- Added installer for FFmpeg built into Silicate
- Fixed DIVIDE_BY_ZERO crash
- Improved Disabled bot behavior - now disables all its hooks and patches and should have zero performance overhead when disabled (perfectly vanilla behavior)
- Fixed Frame Extrapolation freezing the game on frame 0
- Added ability to drag .slc replays onto Geometry Dash to import them

## 1.0.0-alpha.82
- Ported Silicate to GD version 2.2081
- Added Frame Extrapolation (use with care! may break some macros rarely) (requires Lock Delta to be off)
- Fixed Trajectory in editor crashing
- Fixed Silicate not loading on old Windows versions
- Fixed black bar on top of very low quality renders
- Added a new theme
- Renamed existing themes to match the Silicate vibe
- Fixed Hitbox Trail skipping segments
- Fixed Trajectory ignoring breakable blocks
- Fixed memory leak when rendering consecutive levels
- Fixed animation speed being able to be set to an illegal value
- Fixed the bot taking backups of replays while replaying
- Improved DPI scaling behavior
- Fixed Hitbox Trail being rendered after endscreen
- Added new labels
- Renamed "Frame" label to "Tick"
- Improved glass shader reflections

## 1.0.0-alpha.81
- Added Prevent Death - pause one frame before dying (note: requires backwards stepping enabled to use)
- Added Frame Prediction - predict the best frame for the next input (note: requires backwards stepping to use, only works with p1)
- Added Backups - your replay files will now get backed up in a separate peony.silicate/backups directory
- Added Auto-Backup - automatically backup the replay you're recording at a set interval (in seconds)
- Added DPI scaling - the UI now scales with your monitor DPI
- Added Hitbox Trail - also works in the editor!
- Added Render Args - customize your video's output using FFmpeg args! (currently doesn't support -vf, -af, -lavfi)
- Added color pickers for trajectory and hitboxes
- Added a way to toggle off specific hitboxes
- Added circle hitbox
- Added a way to toggle off using trajectory in Prevent Death (may or may not yield a performance increase in some aspects)
- Added Themes (currently only 3)
- Every overwritten replay is now backed up into Backups
- Completing a level with the Auto Save on Complete option and an already existing replay will create a new backup with the just recorded replay
- Reworked the glass shader - now reflects objects at its edges and allows for a more interesting UI
- Moved Backstepping from the Record tab into its own separate category in the Assist tab
- Editor playtesting also contributes to hitbox trail now
- Renderer no longer forces Lock Delta to be on - meaning you can render without Lock Delta
- Fixed a crash or two I think
- Fixed hitbox rendering in the editor
- Fixed black bar on top of renders
- Fixed text being cut off in text boxes
- Fixed editor playtest being slower/faster on different TPSes than your refresh rate
- Fixed editor song preview being slower/faster too
- Fixed frame skipping upon respawn

## 1.0.0-alpha.80
- Fixes backstepping offsetting frames and causing macro breaks
- Fixes non-lock delta speeding up on low framerates
- Fixes trajectory width not scaling with screen zoom
- Fixes HEVC (H.265) encoders producing video with artifacts after YouTube processing
- Fixes backstepping going back two frames in case of death
- Fixes backstepping incrementing the attempt counter
- Fixes backstepping showing the attempt counter again
- Fixes trajectory being inaccurate with speed portals
- Fixes freeze when attempting to enter editor while frame advance is on
- Improves performance when resizing GD window
- Adds rotated trajectory hitbox

## 1.0.0-alpha.79 (public release)
- Fixed hold on respawn issues
- Fixed glass shader breaking during and after rendering
- Fixed hitbox viewer breaking after entering and exiting the editor

## 1.0.0-alpha.78
- Updated to new replay format - slc3
- You may load slc2 replays as well, but new replays will use slc3 exclusively
- Added support for different seeds on different attempts with Intentional Death

## 1.0.0-alpha.77
- Fixed hold on respawn inconsistencies (dual mode, up arrow)
- Fixed trajectory incrementing jump counter

## 1.0.0-alpha.76
- Fixed crash when saving empty replay
- Added Backwards Stepping (default keybind: B)
- Added ability to customize how many backwards steps to save
- Fixed UI opacity not persisting between restarts

## 1.0.0-alpha.75
- Added ability to toggle other trajectories (no color customizing yet that's coming soon™)
- Fixed blur shader on AMD/Intel GPUs (finally)
- Fixed UI scaling having spacing issues on some items

## 1.0.0-alpha.74
- Added Show Hitboxes

## 1.0.0-alpha.73
- Added units to drag values
- Added the ability to change menu opacity
- Added the ability to input fractions in bitrate
- Fixed a crash when inputting 0 TPS
- Improved internal layout engine
- Hid SSB Fix behind "Experimental Features" toggle (using it is not recommended atm)

## 1.0.0-alpha.72
- Fixed inconsistent respawning on orbs while holding triggering them
- Fixed the editor slowing down on high TPS values
- Added a way to disable/enable all enabled labels (so now if you wanna disable them you don't have to go one by one)
- Fixed typo in "Label" section (was supposed to be "Labels")
- Added "Alive/Dead" label

## 1.0.0-alpha.71
- Complete UI overhaul - currently only missing trajectory editing
- Trajectory now simulates your macro's inputs
- Added Mirror Inputs, Mirror Inverted and Maintain Gravity
- Optimized trajectory by a lot
- Added experimental features - toggle on in settings - MAY CRASH or introduce unexpected behavior
- Overhauled Lock Delta
- Updated ffmpeg version

## 1.0.0-alpha.70
- Added conflicts to mod.json so it actually marks them now
- Fixed slope inaccuracies (again) (for real this time)
- Fixed "phantom" frame when respawning during frame advance
- Mitigated checkpoint lag when respawning
- Fixed trajectory not interacting correctly with slopes
- Added SSB Fix to renderer
- Fixed practice fix not saving breakable objects correctly
- Fixed audio not encoding properly on non-stereo audio configurations

## 1.0.0-alpha.69
- Color fix now uses yuv420p instead of nv12 (changing it isn't an option yet but this supports more codecs)
- Added "Extension" field to renderer in case you wanna use something other than mp4
- Autoclicker is now bindable - `autoclicker.enabled` is the tag
- Audio is now properly encoded and videos recorded using Silicate will now play on mobile (i did a silly don't ask)
- There's no longer any delay after pressing Stop and ending the recording - audio is now encoded alongside video and not after it
- Trajectory now predicts correctly in old ball swing copters

## 1.0.0-alpha.68
- Full Lock Delta mode is now forced while rendering (so going really fast won't kill you anymore) (this is temporary-ish, i'll rework this later probably)
- Fixed rare inconsistencies with portals with trajectory

## 1.0.0-alpha.67
- Renderer can now go FASTER than real time while still retaining perfect audio sync and ZERO additional quality loss
- Removed Audio Preview for now
- Pulsing should be perfect on any level now

## 1.0.0-alpha.66
- Fixed audio quality issues in incredibly loud sections
- Fixed cube rotation in trajectory
- Trajectory now interacts with Arrow Triggers, Teleport Triggers, Gravity Triggers and Force Blocks (both kinds)

## 1.0.0-alpha.65
- Fixed pulses being wrong on very high tps macros while rendering (only really happened on Necrotic Majesty, but like, yeah)
- Fixed pulses being too frequent on speedhacked renders

## 1.0.0-alpha.64
- Fixed renderer attaching the last frame of the previous render in a gd session to the next render

## 1.0.0-alpha.63
- Made renderer faster on TPSes over 60

## 1.0.0-alpha.62
- Massively improved renderer performance on higher resolutions (4K, 8K)
- Color fix is now faster
- (TEMPORARY) Pixel format is now always nv12 (yuv420p but slightly different)

## 1.0.0-alpha.61
- Changed ordering of labels (everything should stay the same tho)
- Fixed Intentional Death not removing checkpoints when using Full Restart
- Fixed Autoclicker holding forever when disabling it in a hold state
- Fixed Autoclicker not updating immediately after turning it on
- Fixed Autoclicker executing weird inputs at the very start

## 1.0.0-alpha.60
- Fixed Intentional Death being inconsistent with platformer checkpoints
- Fixed platformer checkpoints being treated as regular checkpoints
- Fixed platformer checkpoints being still on visually after respawning at an earlier checkpoint
- Fixed multi-trigger platformer checkpoints acting weird
- Fixed Intentional Death not registering Death inputs properly
- Fixed checkpoints being one frame later than expected
- Fixed slopes not being 100% accurate when respawning
- Fixed renderer pulsing on video fps'es higher than 60
- Made renderer pulsing closer to the original
- Added an intentional death label

## 1.0.0-alpha.59
- (Re)added renderer presets - save your rendering settings and reuse them
- Presets made before the rewrite will not work
- The last loaded preset is automatically applied upon game start
- The "Preview Audio" option now persists between restarts
- New keybind tag added: `renderer.audio_preview`

## 1.0.0-alpha.58
- Completely reworked label system
- Labels can now have their opacity, size and font customized, as well as position (top-left, top-right, etc)
- Added new labels - player x, y, x velocity, y velocity, gravity (flipped/normal), bot state (recording/playing), random shake state, action index
- Label settings are now saved in geode/config/peony.silicate/labels.json and persist between restarts
- Changing labels' position automatically creates a correct layout with proper spacing for all labels
- (was here before but felt the need to specify) Labels automatically hide themselves during rendering

## 1.0.0-alpha.57
- Fixed inconsistent behaviors with portals in trajectory

## 1.0.0-alpha.56
- Fixed crash when replaying from start positions
- Pausing while rendering actually shows the pause menu now
- Fixed inconsistent player 2 hold on release behaviors when using flipped controls
- Flipped controls no longer actually flip the inputs in the macro

## 1.0.0-alpha.55
- Added Audio Preview to renderer (adapts to your music volume)
- Fixed orb pulsing on irregular tickrates

## 1.0.0-alpha.54
- Minor changes

## 1.0.0-alpha.53
- Added support for shake trigger randomness
- FIXED SOUND EFFECTS CRASHING RANDOMLY (this took 4 FUCKING MONTHS)
- Renderer performance improvements

## 1.0.0-alpha.52
- Fixed improper trajectory caching

## 1.0.0-alpha.51
- Added option to change between Physics (Fast) delta locking and Full (Slow)
- Fixed delta locking for TPSes under 240

## 1.0.0-alpha.50
- Changed delta locking mechanism to the slower, but more consistent version (will add an option to bring back the old one in alpha.51)
  - THIS is unstable! Does not work for TPSes under 240tps (disable Lock Delta for those)
- Removed dependency on CBF (yeah, i know, stupid)

## 1.0.0-alpha.49
- Pausing while replaying will no longer release buttons
- Restarting from checkpoints while replaying will hold correctly
- CBF and its physics bypass are now automatically disabled when starting the bot
- Your mouse cursor is now visible when showing the UI during a level
- Inputting keys in textboxes no longer triggers keybinds
- Added the ability to customize automatic video titles using variables
- Added new `renderer.video_name_template` keybind tag
- Updated logo in title bar to the most up to date one

## 1.0.0-alpha.48
- Added auto video titles because fnm asked nicely
- Added default keybinds if no keybinds file is present
- Added new `renderer.auto_video_titles` keybinds tag

## 1.0.0-alpha.47
- Fixed inconsistencies in trajectory holding and objects

## 1.0.0-alpha.46
- Fixed frame advance causing bad rotations in dash orbs
- Bumped mod geode version to 4.4.0

## 1.0.0-alpha.45
- Fixed p2 not dying in trajectory

## 1.0.0-alpha.44
- Improved trajectory caching
- Added additional trajectory modes customizable in config menu
- Added trajectory color customizability in config menu
- Added swift click trajectory for all modes

## 1.0.0-alpha.43
- Fixed shaders being applied to the trajectory trail and death hitbox
- Fixed improper dual mode caching with trajectory

## 1.0.0-alpha.42
- Fixed trajectory not holding after interacting with orbs
- Added an improvement to trajectory that allows it to account for gravity changes to two players at once

## 1.0.0-alpha.41
- Fixed speedhack not working with Lock Delta on

## 1.0.0-alpha.40
- Audio recorder module temporarily disabled! Don't use this build for recording showcases
- Added new "Lock Delta" option to settings - disables some performance optimizations for stability purposes - enabled by default (hover over the option in the game settings to learn more)
- Added new `updater.lock_delta` key to keybind ids
- Fixed the UPR "Visual" checkbox not working

## 1.0.0-alpha.39
- (Hopefully) fixed some gpus freaking out about shaders
- Added "Use Visual Updates" to non-real time mode
- Added `updater.visual_updates` tag to keybinds
- Fixed bug that kept checkpoint sprites after toggling practice mode off

## 1.0.0-alpha.38
- Fixed sound effects sometimes freezing the game (for real this time)
- Fixed frame advance advancing two frames on time warp >= 2
- Updated the bot's geode version to 4.3.1 (latest)

## 1.0.0-alpha.37
- (Hopefully) fixed pulsing
- Fixed deadlocks in sfx-heavy areas
- Memory related improvements
- Updated ImGui to the latest version

## 1.0.0-alpha.36
- Added custom keybind support
- Added settings persistence

## 1.0.0-alpha.35
- Added proper end level layer restart animation when forcing restarts with Intentional Death

## 1.0.0-alpha.34
- Added ability to do "Full Resets" in Intentional Death
- Added ability to restart from endscreen with Intentional Death
- Fixed TPS not saving correctly sometimes

## 1.0.0-alpha.33
- Made seed saving work with triggers before the start of the level

## 1.0.0-alpha.32
- Added seed saving to replays
- Added non-real time mode
- Added the ability to cap updates per render in non-real time mode
- Added Audio Speedhack
- Added a seed state label
- Fixed the global GD directory being set to Silicate's root directory

## 1.0.0-alpha.31
- Fixed issues with UI feeling unresponsive
- Fixed practice mode resetting frames to double the actual amount with intentional death
- Fixed mirror portals causing issues with the player
- Fixed UI inputs being eaten by GD textboxes
- Improved keybind system (should feel more responsive)
- Moved existing keybinds over to new keybind system
- Added ability to replay and record in the level editor playback mode

## 1.0.0-alpha.30
- Fixed UI fallthrough

## 1.0.0-alpha.29
- Fixed sample rates on non-standard channel layouts

## 1.0.0-alpha.28
- Fixed ffmpeg not accounting for different sample rates/channels

## 1.0.0-alpha.27
- Added trans people (future natalie: what the fuck?)

## 1.0.0-alpha.26
- Fixed frame counting after death

## 1.0.0-alpha.25
- Fixed renderer memory leak
- Fixed "level entry" sound effect being present at the very start of recordings

## 1.0.0-alpha.24
- Made renderer not use cuda always
- Removed extension from video name

## 1.0.0-alpha.23
- New renderer
- New frame counting method (shouldn't break old macros)

## 1.0.0-alpha.22
- Fixed queued buttons not being cleared in between attempts

## 1.0.0-alpha.21
- REMOVED renderer for now
- Added Intentional Death (has issues with appending and doesn't work from endscreen as of now)
- Rewrote input handling code (should be more consistent and work with autoclicker)
- Rewrote TPS bypass (now supports <240TPS and should be slightly more performant)
- Improved practice fix (saves persistent items correctly now)
- Fixed holding while respawning when using mouse
- Fixed a few crashes
- Fixed macro breaks when having "Disable Checkpoints" off in game settings

## 1.0.0-alpha.20
- (hopefully) Fixed pulses when rendering
- Fixed trajectory interacting with direction change triggers

## 1.0.0-alpha.19
- Added autoclicker
- Fixed trajectory portal ypos bug

## 1.0.0-alpha.18
- Fixed slopes with trajectory
- Fixed dual mode trajectory bugs
- Fixed camera trajectory issue
- Added red "ALPHA" next to name (i know massive change)
- Optimized trajectory caching
- Fixed time warp impacting trajectory length

## 1.0.0-alpha.17
- Fixed trajectory drawing too infrequently
- Fixed frame stepper stepping too many frames on high fpses
- Added "block inputs" option to settings

## 1.0.0-alpha.16
- Significant TPS bypass performance improvements

## 1.0.0-alpha.15
- Fixed crashes when loading level from editor
- Fixed crash when saving replay with no name (does nothing now)
- Fixed a hold-related bug

## 1.0.0-alpha.14
- Made the renderer not create folders but instead just directly do video files
- Fixed "endscreen duration" starting at level complete animation

## 1.0.0-alpha.13
- Fixed audio deadlock
- Fixed volume setting correctly

## 1.0.0-alpha.12
- Fixed audio pitch (again)

## 1.0.0-alpha.11
- Fixed audio pitch

## 1.0.0-alpha.10
- Fixed renderer

## 1.0.0-alpha.9
- Fixed "jump when holding on frame when you died" bug
- Fixed trajectory not acknowledging objects that the player already activated
- Fixed macros not saving TPS correctly
- Fixed renderer producing flipped output

## 1.0.0-alpha.8
- Le platformeur accuracé

## 1.0.0-alpha.7
- Fixed trajectory not updating when advancing frames

## 1.0.0-alpha.6
- Frame counter (enable/disable in settings tab)
- Further accuracy improvements

## 1.0.0-alpha.5
- Accuracy improvements

## 1.0.0-alpha.4
- Fixed updateVisibility not being called on lower tpses
- Fixed level entry sound in renders
- Removed "Set" button next to TPS drag value
- Fixed trajectory being too short on low TPSes

## 1.0.0-alpha.3
- Fixed TPS bypass under 240 tps

## 1.0.0-alpha.2
- Fixed TPS bypass not working

## 1.0.0-alpha.1
- Complete rewrite

## 0.8.0-alpha.3
- Fixed a few minor bugs

## 0.8.0-beta.1
- Fix renderer audio desync
- Fix draw divide slowing the game down sometimes
- Add progress bar to renderer UI
- Make disabling "Show Preview" not render the preview at all
- Make UI always render, despite the renderer disabling drawing the game

## 0.8.0-alpha.2
- Fixes Silicate not creating log folders

## 0.8.0-alpha.1
- Add significant performance improvements
- Switch over to an amalgamation of draw divide and FPS multiplier
- Add improved logging

## 0.7.0
- Save seed with replays
- Fix trajectory not updating its settings

## 0.7.0-beta.2
- Fix minor renderer bugs

## 0.7.0-beta.1
- Fix renderer breaking with pixelate shaders
- Fix renderer not being able to be cancelled
- Fix crash when exiting a level when the renderer popup is open
- Fix renderer bringing back old audio volume too late
- Fix renderer recording after end time when stopping in the middle
- Fix renderer not recording properly with speedhack on
- Add legacy audio rendering

## 0.6.1
- Fix patch crash

## 0.6.0
- Improve trajectory rendering
- Fix trajectory taking portals into account

## 0.6.0-beta.10
- ACTUALLY ACTUALLY fix slopes

## 0.6.0-beta.9
- ACTUALLY fix slopes

## 0.6.0-beta.8
- Fix slopes changing player physics

## 0.6.0-beta.7
- Fix the bot not replaying correctly after a restart in practice mode

## 0.6.0-beta.6
- Fix slopes making player invisible

## 0.6.0-beta.5
- Make trajectory update after restart
- Make checkpoints place instantly
- Remove checkpoint count limit (seriously, why was this a thing?)

## 0.6.0-beta.4
- Fix trajectory not taking green portals into account
- Fix trajectory incorrectly assuming size
- Fix trajectory rapidly flashing orbs

## 0.6.0-beta.3
- Fix placing checkpoints causing all sorts of issues

## 0.6.0-beta.2
- Fix crash while reading last input of replay

## 0.6.0-beta.1
- Add show trajectory
- Optimize replaying

## 0.5.10
- Fix minor inaccuracies
- Don't turn on renderer if PlayLayer doesn't exist

## 0.5.9
- Change file format from .sc to .slc

## 0.5.8
- Fix renderer not rendering correctly on different aspect ratios

## 0.5.7
- Fix bot inaccuracies

## 0.5.6
- Fix frame label not being able to be turned off

## 0.5.5
- Fix renderer setting audio to 100% after end

## 0.5.4
- Frame and renderer labels

## 0.5.3
- Fix audio getting desynced in renderer
- Add audio args to renderer

## 0.5.3-beta.2
- Fix inputs double-registering (pt. 2)

## 0.5.3-beta.1
- Fix inputs double-registering (pt. 1)

## 0.5.2
- Add an option to start from the beginning of the level in the renderer

## 0.5.1
- Fix dual physics being modified

## 0.5
- Settings saving
- Custom presets for renderer
- Audio speedhack
- Block inputs on playback
- Autocomplete
- Fix copying and pasting
- Fix after end time not working correctly

## 0.4.5
- Fix audio being offsync while recording

## 0.4.4
- Fix audio not being 100% when recording

## 0.4.3
- Make UI somewhat decent
- Add after end time customization

## 0.4.3-beta.7
- Make disabling "Show Preview" not render the preview at all (UNSTABLE!)

## 0.4.3-beta.6
- Remove debug log spam

## 0.4.3-beta.5
- Fix audio being cut off at end
- Fix slight practice fix bug

## 0.4.3-beta.4
- INSANE accuracy improvements
- Fix FPS setting on replay load
- Fix silicate directories not being created

## 0.4.3-beta.3
- SFX/Audio recording support
- Time warp fix

## 0.4.3-beta.2
- Internal renderer
- Shader fix
- Major bugfixes
- UI bugfixes

## 0.4.3-beta.1
- Bugfixes and UI improvements

## 0.4.2
- Add auto hold toggle
- Insane stability improvements
- Accuracy improvements

## 0.4.1
- Minor bugfixes

## 0.4.0
- Temp-disable renderer
- Move back to pure Rust

## 0.4.0-beta.2
- End recording on level end

## 0.4.0-beta.1
- Add internal renderer
- Add song trigger support for renderer

## 0.3.1
- Remove Idle mode
- Fix sprite rendering

## 0.3.0
- Back to C++!
- UI ported
- Add auto hold toggle

## 0.2.0
- Rust rewrite

## 0.1.7
- UI improvements

## 0.1.6
- Change default FPS to 240
- Add frame advance that only freezes the playable area and not the animations
- Add frame advance checkbox

## 0.1.5
- Fix not being able to remove the first placed checkpoint

## 0.1.4
- Fix frame 0 inputs not being cleared when restarting the level fully

## 0.1.3
- Actually the practice fix shouldn't set time at all! (Fixed)

## 0.1.2
- Fix practice fix setting the wrong time variable

## 0.1.1
- Fixed FPS bypass
- Fixed frame advance sometimes advancing multiple frames

## 0.1.0
- Added recording/playback
- Added FPS bypass
- Added multiple updates per render
- Added frame advance
- Added practice fixes
