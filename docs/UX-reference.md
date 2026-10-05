# Capture overlay reference — inspected before implementation, 2026-10-05

Scope: Win+Shift+S overlay only. No Snipping Tool application window in the normal flow.

References inspected:
- Microsoft's current workflow: https://support.microsoft.com/en-us/office/copy-the-window-or-screen-contents — desktop dims, select mode, drag, clipboard, dismiss; editor is an optional later action.
- Current Windows features: https://support.microsoft.com/en-us/windows/apps/use-snipping-tool-to-capture-screenshots
- Lenovo's Windows 11 capture overlay screenshot (2024-12-10): https://download.lenovo.com/km/media/images/HT517078/snippingkeysWin11_20241210154207628.png
- Newer overlay screenshot: https://cdn.mos.cms.futurecdn.net/x7jYRcz6hq6oMf5wTjxtXo.jpg (Windows Central, https://www.windowscentral.com/how-get-started-snipping-tool-app-windows-11)

Measured source pixels, not an assertion about Windows device-independent units:
- Lenovo raster 619×384: toolbar visible x=320..610, y=3..59, height 56; rectangular button x=320..377, next centers ~406, 464, 522, close 581. Button pitch ~58 source pixels. Icons ~20 pixels, close separator x=552. This image is cropped and has red editorial outlines; those are excluded.
- Newer raster 1321×1000: toolbar x≈517..847, y≈10..58, 330×48, radius≈8. Mode dropdown x≈612..736. Icons ~16–18 pixels; vertical padding ~14 pixels. This newer variant includes recording/text actions outside the requested four capture modes.

Reproduction constants (logical pixels, normalized from image scale): 256×48 capture strip, 8 px corner radius, 4 px inset, five 48×40 buttons, 16–20 px original line icons, 1.25 px icon strokes, 12 px top inset. Four mode buttons in the requested order (rectangle, freeform, window, full screen), separator then close. Dark theme uses #2b2b2b surface, #393939 hover, #454545 pressed, #0078d4 selection; white foreground. One toolbar on the active monitor. Selection restores frozen pixels, 1 px white border; outside is black at 60% opacity. No size badge in the default Windows-parity flow (the earlier requested badge belongs to optional adjustment). No selection handles after ordinary release.

Confidence limits: screenshots establish geometry and general dimming, but do not expose Windows build, display scale, exact opacity, hover colors, or timing. 60% dimming and a 90 ms fade are reproduction estimates, not measured Windows timings. Exact current 1:1 parity cannot be certified without a Windows reference session at a known scale. The requested direct four-mode strip is the documented older Win+Shift+S variant; the newer dropdown layout above is recorded transparently instead of silently claiming both are identical.

Interaction: initial rectangle; crosshair for rectangle/freeform; selected region undimmed during drag; release copies and dismisses. Tab cycles toolbar, Enter/Space activates focus; Escape/right-click cancel. Freeform follows pointer and produces transparent pixels outside its closed path. Window mode highlights the topmost window under the pointer. Fullscreen captures the united workspace. No toast, editor, or main window by default. Ctrl-release/--annotate explicitly opens optional editing, Shift-release/--save saves and copies. Negative origins are translated through the workspace bounding rectangle; output coordinates use KWin's returned scale.

## Authoritative user reference (received during backend implementation)

User supplied a 1500×844 raster showing the desired overlay. The top strip, excluding the yellow editorial callout, spans approximately x=673..900, y=12..60: 228×48 pixels. Button centers approximately (701,36), (743,36), (785,36), (827,36), (877,36). Selected tile approximately 30×30, radius 4; toolbar radius ~8. Mode pitch 42; separator x≈852, y≈24..48; icons ~16 px. This overrides the earlier normalized 256×48/blue estimate. Implemented logical geometry: 228×48, 12 px top inset, 32×32 buttons at x=12,54,96,138,188 (centers 28,70,112,154,204), y=8; separator x=180, y=12, height=24. Selected purple #b58ac6; dark toolbar #111613; light line icons #d6d8d7, selected icon #261c2b. Small deviations of 1–2 pixels reflect raster antialiasing and unknown source display scale. No Microsoft assets are copied: vectors are original drawings.


## Opening-animation correction (0.1.1)

The user requested fully coordinated animation or none after observing whole-screen zoom. The implementation now uses no QML fade or toolbar motion. KWin maps unknown layer-shell scopes to Normal windows, causing its Scale effect to animate the entire frozen desktop. The capture scope is now `utility`, mapped to WindowType::Utility, which its stock Scale and Fading Popups effects exclude. This classification is per capture surface; desktop effects remain enabled for other apps.
