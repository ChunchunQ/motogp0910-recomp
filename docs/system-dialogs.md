# In-game system dialog theme

Text-entry dialogs such as career team naming use the SDK's shared ImGui
keyboard dialog. The application now supplies its default style through
`OnConfigureStyle`, using the palette and spacing from the F1 settings and
F7 achievements menus in `overlay_theme.h`. The existing Open Sans Semibold
font is also the default for these dialogs, at 17 pixels.

The style uses a dark panel and title bar, rounded input fields and buttons,
warm focus accents, and a wider minimum panel size. Shared SDK confirmation
dialogs inherit these defaults as well. Existing F1/F7 panels retain their
own layouts and scoped style overrides.

The shared accent is now `#ff6a13`, matching the launcher. F1 settings, F7
achievements and unlock notifications, system dialogs, the quit confirmation,
F3 diagnostics and keyboard keycap accents use this orange. Hover, active and
selected backgrounds blend the same orange into the dark card color.

This changes presentation only. The SDK still owns input focus, editing,
maximum text length, confirmation/cancellation, and asynchronous delivery of
the entered text to the game. No guest strings, game data or runtime binaries
are changed.

This change is included in the compiled UI/log update. No runtime tests were
run for that build, at the user's request.
