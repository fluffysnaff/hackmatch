# Capture visibility

The Config module's **Streamproof** setting controls Windows display affinity for the Redmatch 2 window.

- Enabled: requests `WDA_EXCLUDEFROMCAPTURE`, hiding the whole game window from compatible Windows Window Capture and Display Capture paths.
- Disabled: restores `WDA_NONE`, allowing those capture paths to see the game and Hackmatch rendering normally.
- Unload always restores `WDA_NONE`.

This is a privacy hint, not a security boundary. Capture software using unsupported or lower-level methods may ignore it. Windows 10 version 2004 or newer is required for exclusion behavior.

OBS **Game Capture** hooks the graphics pipeline directly and does not follow window display affinity consistently. To include Hackmatch in Game Capture, enable OBS's **Capture third-party overlays** option. Without it, OBS may copy the game backbuffer before Hackmatch renders ImGui and ESP.
