# In-game rider license

The user accepted the design at commit `1b8e105` on 2026-10-22, then approved
the refinements finalized in main commit `8a33ea9` for launcher **1.1.11**. Use this
layout, typography, artwork and interaction pattern as
the baseline for subsequent fixes. Further visual redesign requires new user
direction. The user subsequently requested a limited extension: the website's
**Rider Rank**, **Tracks Held** naming and a clearer label for boards entered.
That extension preserves the existing three-column summary and other layout.
Acceptance does not change the documented data/context limits or constitute
independent runtime validation.

The native **View profile** action in Online lobbies and leaderboards now opens a
player card using the launcher's dark palette and orange accent (`#ff6a13`).
It is titled **RIDER LICENSE** (**PİLOT LİSANSI**) and has **Overview** and
**Records** tabs. A persistent left sidebar holds the selected player's avatar,
orange account name, country flag, inline membership date and optional bio. The
right pane holds the tabs and their contents. A compact **Open NOBK Online profile** action remains
in the shared footer; **B / Esc** closes the card without a separate Back button.
A 180 ms fade-in covers
the panel, backdrop, text and photo; Windows' reduced-animation preference is
respected. The left title stripe fades over 80% of its diagonal, the middle
stripe fades only over its bottom 18%, and the right stripe is fully orange.
The ramps use smoothstep opacity over 24 seamless bands. This permanent
difference between stripes is independent of the opening animation.
The user supplied native screenshots of both tabs and confirmed that account
data and avatars load in game. Windows build and Quick quality checks were
dispatched for `1b8e105`; that dispatch did not include local validation or
inspection of workflow results. The subsequent `8a33ea9` source milestone passed
[local phased checks and a Windows Release build](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md). The approved design is implemented on an 1100 by
580 design canvas, scaled uniformly to fit the display. Text and hit targets
share that canvas; the outer window has no scroll state. Overview fits without
scrolling. Records draws complete rows with a dedicated row-based scrollbar,
keeping its tabs, filter, headings, sidebar and footer fixed. At 1920 by 1080,
Records retains six rows and its existing circuit/lap/mode typography roles.
Rank/population pairs use a common size and baseline. Overview uses moderately
larger typography, including roughly 48-pixel summary values and a 44-pixel
personal-best time at that resolution. Summary
columns, the tab group and rank/population pairs are centered in their respective
areas. Each rank pair shares one baseline; circuit names remain left-aligned.

The pinned renderer uploads a static font atlas. F1 uses fonts prepared at 17
and 30 pixels, while the original license scaled the 17-pixel font to arbitrary
sizes. The application now registers 34 additional profile-specific sizes
(5–32 pixels at one-pixel intervals, then 36/40/44/48/52/56) during font setup,
before the SDK uploads the atlas. Profile drawing, measurement, wrapping and
widgets select the closest prepared size and render it at its actual pixel size;
canvas positions are rounded to screen pixels. F1 fonts, SDK/runtime pins and
the rendering backend are unchanged. Small prepared sizes prevent the old
12-pixel minimum from overlapping bio/Records baselines in small viewports;
ordinary display sizes still select the same sharp native faces. Native visual
verification remains pending.

## Identity and public data

Both native entry paths reach `82303260`, whose vtable entry is at `8206CB4C`.
It takes the selected player's record in `r5`, reads the XUID at offset 8 and
calls `8239C430`. That wrapper passes `r3 = local user index`, `r4 = selected
XUID` and `r5 = 0` to `XamShowGamerCardUIForXUID`. The SDK's previous stub for
that import is replaced by the client hook; original game data and generated
functions are unchanged.

The existing message getter hook replaces native bank-0 IDs 1137 and 1433
(formerly "Gamer Card") and 1416 (formerly "View gamer card") with **View profile**
or **Profili incele**, according to the game's written language. Menu action IDs
and the original message banks are preserved. The Turkish source catalogue is
kept consistent with these labels.

The community service encodes a player as `0x0009000000000000 | id`, with a
nonzero 24-bit ID. This ID is resolved through the existing public endpoint
`https://www.nobkonline.com/api/motogp/player?id=<id>`. The response ID must match
the selection before its account name is used. The current launcher account,
leaderboard rank and the site's independent account ID are never substituted.

**Open NOBK Online profile** opens
`https://nobkonline.com/profile/?u=<percent-encoded-name>`, as requested, rather
than the MotoGP-only rider page. The general profile page itself uses
`/api/fifa09/player?name=<percent-encoded-name>` for shared account metadata,
including MotoGP users. The card follows that existing public contract to read
`country`, `member_since` and `avatar`, requiring the returned `player` name to match the
resolved MotoGP account. It never treats a MotoGP ID as a FIFA or site account ID.

A supplied avatar path must belong to `/api/avatar/` on NOBK's HTTPS host.
The query string is preserved for the site's versioned image URLs. An explicit
empty avatar means no uploaded photo. When the field is absent, the legacy
`/api/avatar/u/<percent-encoded-name>` route remains a fallback. Arbitrary hosts
and HTTP redirects are rejected.

Only Online mode with the production `https://motogp.nobkonline.com` service
uses this lookup. Offline, LAN and independent services are not mapped to NOBK
players, even if their numeric identities happen to resemble a production ID.
No account token, password or community credential is sent to either public
endpoint. No backend, protocol or SDK/runtime change is required for this card.

A valid two-letter country from the shared profile is displayed when available.
The current MotoGP endpoint does not supply country. If the optional shared
lookup fails, an otherwise valid MotoGP identity and its profile action remain
available. Country is not inferred from the current user, language or IP.

Country codes select flags in an embedded 768 by 544 PNG atlas; no flag download
or emoji/font support is required at runtime. The atlas is rasterized from
[flag-icons](https://github.com/lipis/flag-icons/tree/ac7e0eb3259eb62aa6100c435607e2fe03896a97)
(MIT). Its attribution is in `third_party/flag-icons-LICENSE.txt` and the launcher's
embedded third-party notices. Missing country data is left hidden. Membership
dates use the shared API's Unix timestamp and localized month/year, never the
illustrative date from the concept image.

## Overview and personal records

The identity response already includes the selected player's `records` array.
The card validates and publishes an immutable record snapshot on the same
worker; switching tabs, filtering and scrolling add no HTTP calls. Invalid or
missing records do not prevent a valid identity, avatar or profile action.

Overview shows distinct circuits where the player leads at least one board,
the number of boards where the player is first, and total published leaderboard
entries. Its percentage uses the 20 playable circuits supplied by the base/DLC
content. Native slot 17 (reserved Balaton) is excluded; Silverstone, Shanghai and
Istanbul remain indices 18, 19 and 20. This differs deliberately from the live
site's older 17-track catalogue. After comparing the user's screenshots with the
site's `leaders` / `withTracks` calculation, the user selected **DLC included:
tracks led / 20**. The first statistic is now **Tracks Held**, and the **Total Track
Domination** percentage uses that same distinct-first-place circuit count. In
the supplied example, 17 boards led across 16 circuits means 16/20 = 80%; 19 is
the total board count. The site's 16/17 uses its smaller catalogue. The previous
17/20 counted circuits with any saved result, a different statistic. The initial
2009 season filter still applies; multiple first-place boards on one circuit
increase Records held but count only once toward Tracks Held. The third caption
is **Boards Entered** (**Katıldığı tablolar**), replacing **Leaderboards**. This
counts separate published circuit/class boards with a personal best, not unique
circuits ridden: two class entries on the same circuit count as two boards.
The existing numerator and percentage calculations are unchanged by the rename.

## Rider Rank

The header's right-hand space displays **RIDER RANK** (**PİLOT SIRASI**) and a
compact `#<position>` on both tabs. It is separate from the selected-board rank
in Personal Best and from the three existing summary counters. This addition
does not add another summary column or alter the Records table.

The website does not display the raw `pos` from `/api/motogp/ranking` directly.
Its current `withTracks` code joins that endpoint's players with leaders from
`/api/motogp/views`, counts distinct circuits led, sorts by that count descending,
then `firsts` descending, then `points` descending, and finally raw `pos`
ascending. The client follows that exact ordering and looks up the selected
MotoGP player ID, never the current launcher user or a display-name guess.
The live ranking page, its current JavaScript and both public endpoints were
retrieved successfully during this implementation; the responses contained 15
ranked riders and 38 populated boards at that time.

This rank intentionally follows the site's current 17-base-circuit catalogue
and full ranking scope. It does not apply the card's separate DLC denominator
or temporary 2009 summary filter. The public ranking API currently returns at
most 100 riders; an absent rider is shown with a hyphen, not an invented rank.
The hover explanation distinguishes a rider not listed from a lookup failure.
Loading shows an ellipsis; a failed or malformed response shows a hyphen while
the rest of the profile remains available.

Both read-only requests run on the existing cancellable worker after identity,
record, metadata and avatar results have been published, before optional map
artwork. They use the fixed public host without credentials, with 256 KiB and
512 KiB response limits respectively, and reject invalid or duplicate IDs,
positions and board indices. A single immutable, memory-only ranking cache is
shared across players for 60 seconds. Each opening selects its own player ID
from that list, including when the profile itself is cached. No polling occurs;
failed lookups wait 60 seconds before a subsequent opening can retry. Closed or
superseded requests cannot publish into a different card. Local checks are
recorded in [the validation report](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md); native visual
verification remains pending.

## Records table

Records has five columns: rank/board population on one line, circuit with
class/2009 underneath, best lap, stored mode, and **Set**. Results use the site's
`display.text`; game mode comes from uploaded property 6. Unknown data uses an
ASCII hyphen to avoid a missing-glyph diamond. The empty bike column is removed.
Class labels follow published metadata, preserving the existing server-side
Moto2 integration limits. As requested, this initial view is scoped to 2009:
known 2010 rows (property 13 = 1) are excluded from the list, filters and summary;
legacy rows without a season remain in the initial 2009 presentation. The source
season property is retained for the later server integration.

**Set** uses each row's existing `updated` Unix timestamp, the same field used
by the site's results table. It shows localized relative time below seven days,
then a short date; hovering reveals local date and time. Missing timestamps show
a hyphen. The server's best-score write path keeps the prior timestamp for equal
or worse submissions, so this represents the accepted personal best's set time,
not the latest profile read. No additional HTTP request is needed.

The class filter is built from returned records. Empty records and failed lookups
have separate messages. Its popup has dedicated vertical padding, a smaller
corner radius, complete 24-unit selectable rows and height based on every
available class plus padding and rounding headroom. It no longer inherits the
main card's zero vertical window padding or caps the option list at eight rows;
all current classes fit without a popup scrollbar. The redundant X/F hint next
to the selector is removed. Sorting uses
rank, then latest update. A card opened
through the verified leaderboard caller `8223EDD4` copies the current native view
from `833362D8` (written by selector `82300088`). That exact result is shown first
and highlighted; other results on the selected circuit follow. Overview also
shows the exact selected-board best when the public API exposes it. Lobby calls
have no board context and cannot inherit a previous leaderboard selection.
The public player endpoint currently lists group-0 results, so unpublished ghost
or other groups are not represented as if they were returned.

## Membership and bio

The membership date shares one line with **Member since**, for example
**Member since October 2026**. English month names can be abbreviated when a
long label would not fit; the text does not become smaller to force a fit.
Turkish uses **Üyelik: Ekim 2026**. Missing membership information is hidden.

The existing shared public account response also contains `bio`. It is read
after the account name has been verified and uses the same request and cache as
the avatar metadata; no new endpoint, download or periodic refresh is added.
Only plain UTF-8 text is displayed, up to 300 Unicode characters / 1200 bytes.
Line breaks are preserved, tabs become spaces and embedded ASCII controls are
excluded. Empty, oversized or unavailable bios remain hidden. Bio contents are
never written into diagnostics.

The sidebar shows a word-wrapped three-line preview below the membership date.
If more text remains, a clickable **...** button at the end of the last line
opens a centered **About me** modal with the full text. **Y** on the gamepad or
**R** on the keyboard can open it for any nonempty bio. Preview wrapping preserves
UTF-8 character boundaries, including when a single word exceeds the column.

The modal sizes to the text up to a bounded height; only its text area scrolls
for unusually long paragraphs or many explicit line breaks. Mouse wheel,
scrollbar, D-pad and keyboard arrows can scroll it. A **Close** button,
**A / Enter** or **B / Esc** closes the bio while retaining the underlying card,
its selected tab and Records scroll position. Closing input cannot also open
the website or dismiss the card in the same frame. Tabs, filters and profile
actions are blocked while the modal is open. Each reopening starts the text at
the top, and a new player selection resets its open state. The modal footer has
only a centered Close button; its redundant shortcut sentence is removed. The
preview tooltip also omits its redundant Y/R hint. The main license footer shows
controller hints only (LB/RB, A, B, D-pad and Y); keyboard bindings remain active.
The complete hint string is measured and centered within the same right-pane
width on both tabs. Adding Records' D-pad hint or the optional bio hint changes
the string's width without moving its center. The profile action keeps its
existing sidebar alignment.

### Embedded emoji rendering

Both the preview and the full bio modal render the embedded Noto emoji catalogue:
5,709 sequences sharing 4,068 sprites, including non-BMP characters, flags,
skin-tone variants, keycaps and supported ZWJ families. The original UTF-8 bio
remains intact in the account snapshot. A display-only longest-sequence match
maps each supported cluster into a BMP private-use glyph, preserving the pinned
SDK's 16-bit `ImWchar` ABI. Unknown future emoji require a catalogue update.

The transparent 32-pixel sprites are stored once in a shared atlas rectangle;
all prepared profile sizes reuse those UVs with native glyph metrics and color
rendering. There are no emoji HTTP requests or per-profile texture downloads.
Font faces now include sizes 5–11 so small viewports do not force a 12-pixel
minimum into shorter bio and Records baselines. F1 font sizes remain unchanged.

`scripts/build-profile-emoji.py` reproducibly builds the PNG and JSON from a
local Noto Color Emoji font using fontTools and Pillow. The metadata records
the input font's SHA-256. Its SIL OFL attribution is retained in
`third_party/noto-emoji-OFL.txt` and the launcher's embedded third-party notices.

## Personal-best circuit panel

The user subsequently approved the circuit-panel layout. Personal Best now has
a small circuit outline on the left, circuit name and lap/class/2009/mode in
the middle, and rank/board population plus **Set** on the right. The middle
column has equal insets between its dividers: circuit name at the top, the
larger lap below, then class/2009/mode on one line, all centered on the same
axis. A missing-lap message shares that center. The date uses
the same accepted-result timestamp and formatter as Records; hovering it shows
the full local date and time. All lap details come from the exact selected
leaderboard view, never another circuit's faster lap. An unpublished result
keeps the selected circuit name and available map, with an explicit missing-lap
message and no invented rank or date. Without a selected leaderboard, the panel
shows **No track selected**; lobby context integration remains a separate task.

The site's public track catalogue maps native circuit slots 0–16 to PNG outlines
under `https://www.nobkonline.com/assets/motogp/tracks/`. The client uses an explicit
filename table with the original slot indices. Losail's `omap-qatar.png` was
retrieved and visually inspected. Reserved slot 17 has no map. Published map
paths for Silverstone, Shanghai and Istanbul were not found; the ignored local
game assets were also searched, including `frontend_track.mgb`, but no directly
usable DLC outline was identified. These slots show **Map unavailable** while
their lap data remains usable. No guessed substitute circuit or original game
texture is committed or packaged.

Outlines are optional, fetched only after the existing identity, metadata and
photo work has published its results. A map cannot delay the profile's data or
browser action. The existing cancellable HTTP client requests only a known
filename on the fixed public host, caps responses at 256 KiB and validates PNG
dimensions (maximum 1024 square) before decoding on the worker. Successful maps
remain in a bounded, memory-only cache keyed by native circuit for the game
session. Failed requests wait at least 60 seconds before another opening can
retry; there is no polling. Cached profiles still select artwork from the new
opening's circuit, and closed or superseded requests cannot publish into another
card. The UI uploads each changed image once and fits its original aspect ratio
inside the map area. See [local validation](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md) for the
subsequent checks; native visual review remains pending.

## Avatar loading and fallback

The default avatar is a transparent side-profile racing helmet adapted from the
user's supplied reference. Its off-white shell and light silver outer outline
remain visible against the dark card; the visor and vents remain charcoal.
`resources/gamer-card/default-helmet.png` is embedded as Windows RCDATA 201,
decoded once on the profile worker and uploaded once by the UI renderer. This
trusted 1254-square embedded image is independent of the download size limits
below. It is available before identity HTTP completes and needs no network
request or sidecar file. Its image is drawn directly with transparent padding,
without the old geometric helmet or an opaque square/frame. Uploaded photos
retain their existing fit and border. Embedded-image decoding and CPU previews
are covered by the subsequent local validation; native visual review remains.

Name lookup starts only when the card opens, on one worker. The account name,
records and profile action become available before the optional account metadata
and avatar download end. There is no periodic network refresh. Stale responses
from a closed card or another selected player cannot replace the current card's
name or image.

The worker reuses a WinHTTP session and connection across identity, metadata,
avatar, optional circuit outlines and subsequent player requests, allowing HTTPS
keep-alive instead of
recreating the entire session for each download. Requests use asynchronous
WinHTTP completion callbacks with a cancellable worker wait. Closing the card,
selecting another player or shutting down cancels the current request; the next
player does not wait for a closed card's network timeout. Request context and
read buffers remain alive until the final handle-closing callback. Each HTTP
exchange has a five-second deadline shared by send, headers and body reads,
instead of accumulating a separate timeout for every stage.

The two supplied native logs show the first identity request timing out, followed
by a successful manual reopen of the same player; later lookups in the second
log complete in roughly 0.3 seconds including metadata and photo. This points to
an initial-request problem, but the old combined send/receive timing cannot
identify DNS, proxy discovery, TLS or server response as the cause. The first
identity request now gets a two-second initial attempt and one automatic retry
for transient transport failures, using only the remainder of the same
five-second total budget. Cancellation, HTTP error responses, identity mismatch
and malformed JSON do not trigger this retry. Later player lookups do not add
retries, and there is no background warm-up or polling request.

A bounded, memory-only cache retains up to eight profiles. A complete profile
reopened within 60 seconds is shown immediately without new identity, metadata
or avatar HTTP requests. Optional ranking/map work has its own cache. An older or partial
cached profile can be shown for up to five minutes while the worker refreshes it
in the background. The selected leaderboard view is always taken from the new
opening, never from the cached card. Refresh failures keep the available cached
profile visible; a verified account rename clears the previous account's photo.
Partial or failed loads are eligible for another attempt on reopening. The cache
does not survive game shutdown, and a first visit still depends on network and
server response time. Subsequent local checks are recorded in
[the validation report](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md).

Avatar downloads are limited to 512 KiB and image dimensions to 1024 by 1024
before decoding. HTTP errors, missing photos, invalid or oversized images leave
the default helmet visible; they do not disable an otherwise resolved profile.
Windows Imaging Component decodes JPEG and PNG on the worker. The site accepts
JPG, PNG and WEBP uploads but can serve a re-encoded JPEG; decoding uses the
actual image format rather than the upload extension. No optional WEBP codec
is required by the card. Texture creation and release
use the existing immediate renderer on the UI thread. The full photo fits the
avatar square without stretching or cropping. Avatars are not written to disk.

## Diagnostics

The user requested removal of the temporary Rider License debug instrumentation.
Routine opening/closing, successful HTTP phases/results, timings, JSON field
availability, cache hits, retry notices, normal cancellation/discard, successful
image decoding/upload and bio/browser action traces are removed. Counters and
content-type queries used only by those messages are removed as well. A normal
successful visit, including an account without a photo, emits no `PLAYER_CARD`
informational messages.

Actionable warnings remain in the existing game log with the `PLAYER_CARD`
prefix: invalid identity or response data, transport/HTTP failures, rejected
avatar paths, image/resource decode failures, failed GPU texture creation,
missing profile fonts and a failed browser action. HTTP errors retain the
request number, stage and Windows error code or HTTP status; routine 404 avatar
fallbacks are silent. Normal cancellation is still handled separately from
failures. Loading, cache freshness, retry, deadlines and cancellation behavior
are unchanged. No separate logging service, polling or debug toggle is added.
Warnings do not contain response bodies, passwords, tokens, cookies or bio text.

For a missing avatar/country, open that player's card, wait for loading to finish,
then share the newest game log from the install's `logs/` directory. Search for
`PLAYER_CARD` warnings to identify failures; successful request stages no longer
emit informational traces. A default helmet is expected for an account
whose public profile explicitly reports no photo.

If the player name cannot be resolved, the card explains the failure and keeps
**B / Esc** available; its profile button is disabled. It never opens a guessed
username. There is no new modal error box.

## Controls and lifecycle

- **A / Enter** opens the selected player's general NOBK profile in the browser.
- **B / Esc** closes the card; there is no visible Back button.
- **Y / R** opens the full bio when present; the preview's **...** does the same.
- With the bio open, **A / Enter**, **B / Esc** or **Close** closes only that modal.
- **LB / RB** or **Q / E** changes tabs.
- **X / F** cycles the Records class filter.
- **D-pad / Up / Down** scrolls Records or the open bio; a held D-pad repeats at
  a bounded rate.
- Mouse controls support tabs, the class dropdown, wheel/scrollbar navigation,
  the bio preview and the compact profile action. The license itself never
  scrolls; scrolling is limited to Records rows or the bio modal's text area.

The card uses a separate instance of the existing settings input gate. Opening
F1 and the card is serialized so concurrent requests cannot acquire both gates.
The opening press must return to neutral before card actions become active. Focus
changes rearm this guard, and dismissal remains blocked from the underlying
menu until held inputs are released. The existing guest-frame observer polls
only an active/held gate; no device or HTTP work is added while it is inactive.
The card sends the usual `XN_SYS_UI` open/close notification, prevents opening
F1 settings over it, and participates in the existing quit/keystroke gate.

## FIFA 09 reference and limits

The inspected reference is
[ChunchunQ/fifa09-recomp at `711da394`](https://github.com/ChunchunQ/fifa09-recomp/blob/711da3941d81879db06ea4f2c36c3a519ac826d9/patches/nobk-sdk-1.3.0.patch).
Its social channel supplies friend/account metadata including country and
avatar paths. Avatar download and UI-thread texture creation are separate.
This card uses the shared public avatar service without importing FIFA's
social protocol, friends list, invitations or SDK patch.

The current card does not add account editing, friends, invites or replay
transfers. Native Windows review should cover both tabs, class filtering,
gamepad/keyboard scrolling, country flags, inline membership date, short/long/empty
and multiline bios, bio modal input isolation, the three distinct stripe ramps
and selected-board ordering, as well as a lobby member,
multiple leaderboard rows including the local player, accounts with/without a
photo, failed lookups, opening/dismissal presses and switching to the browser
and back. Native playthrough checks remain pending; local CPU/UI coverage is
recorded in [the validation report](https://github.com/inflamesgo/motogp0910-recomp/blob/5e91cf5/docs/rider-license-validation.md).

## Delivery and superseded design notes

Main commit `8a33ea9` contains the approved Rider License, Rider Rank, centered
controller footer and Personal Best details, general emoji support, diagnostic
cleanup and launcher version 1.1.11. The accepted design requirements from the
former follow-up checklist are consolidated above; no new redesign is pending.
The feature and its required resources/licenses were selectively pushed to
`preview` as `ab7d3f7` for the existing upstream PR. The original frozen handoff
worktree was retained. See [the handoff](chunchunq-handoff.md) for exact scope.
