#!/usr/bin/env python3
"""Package the cross-built Windows application; original game files stay out."""
from pathlib import Path
import hashlib
import shutil
import zipfile
import json
import argparse

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "out/build/windows-cross-release"
STAGE = ROOT / "out/package-windows"
DIST = ROOT / "out/dist"
RUNTIME = (
    "rexruntime.dll", "rexgpu-xenos.dll", "amd_fidelityfx_dx12.dll", "msvcp140.dll",
    "msvcp140_atomic_wait.dll", "vcruntime140.dll", "vcruntime140_1.dll",
)

def main():
    global BUILD
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'out/build/windows-nightly-release')
    parser.add_argument('--archive',default='MotoGP0910-Windows-x64-stability-update.zip')
    parser.add_argument('--lan-preview',action='store_true',help='Include the experimental LAN multiplayer integration and Windows test instructions')
    parser.add_argument('--online-preview',action='store_true',help='Include community Online/LAN/Offline instructions')
    parser.add_argument('--include-one-pc-test', action='store_true', help='Include optional loopback test helpers; omitted from clean two-PC packages')
    parser.add_argument('--include-readmes', action='store_true', help='Include optional release instructions; disabled for development builds')
    args=parser.parse_args();BUILD=args.build_dir.resolve()
    if args.online_preview:args.lan_preview=True
    if Path(args.archive).name!=args.archive or not args.archive.endswith('.zip'):
        parser.error('--archive must be a ZIP filename')
    sdk=json.loads((ROOT/'sdk-version.json').read_text())
    # Recreate only our packaging directory so previous checksums or stale
    # files cannot leak into a new release.
    if STAGE.exists():
        shutil.rmtree(STAGE)
    STAGE.mkdir(parents=True, exist_ok=True)
    DIST.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((ROOT / "third_party/rexglue-runtime/manifest.json").read_text())
    for name, expected in manifest["dll_sha256"].items():
        if hashlib.sha256((BUILD / name).read_bytes()).hexdigest() != expected:
            raise SystemExit(f"Runtime mismatch: {name}; stage the spatial FSR pair first")
    for name in ("motogp0910.exe", "motogp0910-launcher.exe", "motogp0910-crash-monitor.exe", "motogp0910.map", *RUNTIME):
        source = BUILD / name
        if not source.is_file():
            raise SystemExit(f"Missing build output: {name}")
        shutil.copy2(source, STAGE / name)
    licenses = Path("/workspace/shared/rexglue-nightly-packages/win-amd64/licenses")
    if licenses.is_dir():
        shutil.copytree(licenses, STAGE / "licenses", dirs_exist_ok=True)
    shutil.copy2("/workspace/shared/rexglue-sdk-nightly/LICENSE", STAGE / "licenses/ReXGlue-LICENSE.txt")
    controller_db = ROOT / "third_party/SDL_GameControllerDB"
    shutil.copy2(controller_db / "gamecontrollerdb.txt", STAGE / "gamecontrollerdb.txt")
    shutil.copy2(controller_db / "LICENSE.txt", STAGE / "licenses/SDL_GameControllerDB-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/tomlplusplus-LICENSE.txt", STAGE / "licenses/tomlplusplus-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/stb/LICENSE.txt", STAGE / "licenses/stb-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/open-sans/LICENSE.txt", STAGE / "licenses/OpenSans-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/open-sans/NOTICE.txt", STAGE / "licenses/OpenSans-NOTICE.txt")
    shutil.copy2(ROOT / "third_party/launcher-fonts/OFL.txt", STAGE / "licenses/LauncherFonts-OFL.txt")
    shutil.copy2(ROOT / "third_party/launcher-fonts/NOTICE.txt", STAGE / "licenses/LauncherFonts-NOTICE.txt")
    shutil.copytree(ROOT / "third_party/rexglue-runtime/licenses", STAGE / "licenses", dirs_exist_ok=True)
    shutil.copy2(ROOT / "third_party/rexglue-runtime/manifest.json", STAGE / "runtime-manifest.json")
    shutil.copytree(ROOT / "resources/controller", STAGE / "resources/controller",
                    ignore=None if args.include_readmes else shutil.ignore_patterns("readme*", "README*"))
    shutil.copytree(ROOT / "resources/localization/tr/overlay", STAGE / "resources/localization/tr/overlay")
    shutil.copy2(ROOT / "resources/localization/tr/manifest.json", STAGE / "resources/localization/tr/manifest.json")
    (STAGE / "resources/ui").mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / "third_party/open-sans/OpenSans-Semibold.ttf", STAGE / "resources/ui/OpenSans-Semibold.ttf")
    # Runtime resources and legal notices are always included. User-facing
    # instructions are opt-in so development builds contain no README files.
    (STAGE / "game").mkdir()
    if args.include_one_pc_test:
        shutil.copy2(ROOT / "scripts/test-lan-one-pc.ps1", STAGE / "test-lan-one-pc.ps1")
    if args.include_readmes:
        shutil.copy2(ROOT / "docs/achievements.md", STAGE / "achievements-readme.txt")
        shutil.copy2(ROOT / "docs/controller-prompts.md", STAGE / "controller-prompts-readme.txt")
        shutil.copy2(ROOT / "docs/quit-menu.md", STAGE / "quit-menu-readme.txt")
        shutil.copy2(ROOT / "docs/in-game-settings.md", STAGE / "in-game-settings-readme.txt")
        shutil.copy2(ROOT / "docs/adaptive-triggers.md", STAGE / "adaptive-triggers-readme.txt")
        shutil.copy2(ROOT / "docs/turkish-localization.md", STAGE / "turkish-localization-readme.txt")
        if args.online_preview:
            shutil.copy2(ROOT / "docs/community-online.md", STAGE / "online-readme.txt")
            shutil.copy2(ROOT / "docs/community-security.md", STAGE / "online-security.txt")
        milestone="Experimental LAN multiplayer" if args.lan_preview else "Stability update"
        preview_notes=(
            "EXPERIMENTAL LAN: Guest XNet/session and UDP/VDP socket integration is included.\n"
            "General > Connection mode > LAN / Radmin; automatic rooms, optional host IPv4 fallback.\n"
            "Use the same package on both PCs and unranked Player Match on LAN / Radmin.\n"
            "No Xbox Live login or dedicated server. Offline remains the default.\n"
            "LAN startup fix: 17 targeted actual-SDK Windows/Wine checks passed.\n"
            "Empty Auto-IP command-line values no longer swallow the next option.\n"
            "The user reported successful two-PC Radmin racing with the previous executable.\n"
            "LOBBY FIX: Retain selected search results; accept null join-only slots on leave.\n"
            "Targeted Windows regressions passed; revised Radmin gameplay needs user testing.\n"
            "Read lan-readme.txt for the exact two-PC steps and unsupported features.\n\n"
            "ICONS: Game EXE uses the supplied MotoGP 09/10 cover. Launcher EXE uses\n"
            "the supplied MotoGP logo. Both embed seven Windows icon sizes (16-256 px).\n\n"
        ) if args.lan_preview else ""
        if args.online_preview:
            milestone="Experimental community Online / LAN / Offline"
            preview_notes=(
                "COMMUNITY ONLINE: General > Connection mode > Online. Enter the community HTTPS server.\n"
                "The VDS provides room discovery, authenticated UDP relay and persistent native leaderboards.\n"
                "No Radmin or player router forwarding is required for Online. LAN / Radmin remains separate.\n"
                "The race simulation stays on a player's PC. Use Player Match; official Xbox Live ranked/friends/voice are not restored.\n"
                "There is no default public server yet. Deploy the separate Linux/Windows community server package first.\n"
                "SECURITY UPDATE: Online MGO2 / v2; update the game and server together. Old Online builds are rejected.\n"
                "UDP endpoint proof, strict room admission, HTTP/TLS/storage quotas and authenticated Caddy proxy are included.\n"
                "See online-security.txt. Keep existing profiles/database; LAN02 and Offline are preserved.\n"
                "Local Windows/Wine protocol and original stats-wrapper checks passed; public internet racing still needs testing.\n"
                "Offline is the default. See online-readme.txt for deployment, scope and remaining validation.\n\n"
            )+preview_notes
        if args.lan_preview:
            shutil.copy2(ROOT / "docs/lan-multiplayer-test.md", STAGE / "lan-readme.txt")
            shutil.copy2(ROOT / "docs/lan-startup-fix.md", STAGE / "lan-startup-fix.txt")
            shutil.copy2(ROOT / "docs/lan-lobby-stability.md", STAGE / "lan-lobby-fix.txt")
        shutil.copy2(ROOT / "docs/crash-reports.md", STAGE / "crash-readme.txt")
        shutil.copy2(ROOT / "docs/keyboard-prompts.md", STAGE / "keyboard-prompts-readme.txt")
        if args.include_one_pc_test:
            shutil.copy2(ROOT / "docs/lan-one-pc-test.md", STAGE / "lan-one-pc-readme.txt")
        one_pc_notes = "ONE-PC LAN TEST: Run test-lan-one-pc.ps1; see lan-one-pc-readme.txt.\n" if args.include_one_pc_test else ""
        (STAGE / "readme.txt").write_text(
            "TURKISH GAME TEXT: General > Game language > Turkce. Voices stay original.\n"
            "Keep resources/localization. Xbox LIVE Marketplace is removed from Options.\n"
            "See turkish-localization-readme.txt for scope and verification limits.\n\n"
            f"MotoGP 09/10 - Windows x64 {milestone} - ReXGlue {sdk['version']}\n\n" + preview_notes +
            "DUALSENSE (EXPERIMENTAL): Controls > DualSense adaptive triggers...\n"
            "Separate L2/R2 resistance modes and positions/strength; default off.\n"
            "F1 > Controls changes and saves the same settings live. Requires native\n"
            "DualSense / Edge with SDL. 8BitDo / DS3 / DS4 cannot provide resistance.\n"
            "Software/SDL transport verified; physical DualSense testing pending.\n"
            "See adaptive-triggers-readme.txt.\n\n"
            "QUIT UPDATE: The user confirmed Press Start Back opens the exit dialog.\n"
            "QUIT GAME now recolors its upper blue cap too, preserving brightness\n"
            "layers, alpha and geometry. Final GPU appearance needs user review.\n"
            "See quit-menu-readme.txt.\n\n"
            "LIVE SETTINGS: F1 opens the MotoGP settings menu (English/Turkish).\n"
            "Only immediately applied controls appear: audio, FPS/VSync, prompts,\n"
            "upscaling/sharpness, native engine effects and all 25 keyboard mappings.\n"
            "Save settings also updates launcher values; Return keeps live changes for this session.\n"
            "The race/network keeps running; pause offline gameplay before adjusting.\n"
            "See in-game-settings-readme.txt for scope and native Windows verification limits.\n\n"
            "CONTROL SETTINGS: Xbox, original PS3 DualShock 3 diagram or dark/orange\n"
            "keyboard key list, following automatic/manual button-prompt selection.\n"
            "Default / Alternative A / B keep native action labels dynamic; keyboard\n"
            "bindings update live. Keep resources/controller/ps3-controls.bin.\n"
            "See controller-prompts-readme.txt for scope and native visual checks.\n\n"
            "LAUNCHER: Achievement scrollbar supports native mouse dragging; the title\n"
            "bar contains only minimize and close. Existing saves stay read-only.\n\n"
            "QUIT GAME: Native main-menu entry and B/back on the initial Press Start screen.\n"
            "Confirmation defaults to Cancel; pad A/B, keyboard and mouse are supported.\n"
            "Held dismissal buttons stay masked; native input checks passed under Wine.\n"
            "Native Windows menu appearance and save/exit playthrough need local verification.\n"
            "See quit-menu-readme.txt.\n\n"
            "ACHIEVEMENTS: Launcher > Achievements displays your real progress, gamerscore,\n"
            "original icons, search and All / Unlocked / Locked filters (50 achievements / 1000 G).\n"
            "Progress refreshes while the game runs. Existing achievement saves are read-only.\n"
            "Pop-up in game and Unlock sound are independently shared with the F7 menu.\n"
            "F7 opens the dark MotoGP list with search, unlocked/locked filters,\n"
            "progress and original achievement icons. Esc or Close returns to the game.\n"
            "Unlock notifications fade in at the bottom right with a soft 450 ms cue.\n"
            "F7 > Unlock sound / volume can disable or adjust it; game master/mute applies.\n"
            "English/Turkish follows Launcher language at game startup.\n"
            "Keep resources/ui with the complete build. See achievements-readme.txt.\n\n"
            "CRASH REPORTS: General > Extended crash diagnostics (default ON).\n"
            "The separate monitor captures fatal errors including fast-fail 0xC0000409.\n"
            "Reports: logs/crash-*/report.txt and crash.dmp, with latest log and settings.\n"
            "Error report also copies the latest native crash folder.\n"
            "Keep motogp0910-crash-monitor.exe and motogp0910.map with the other files.\n"
            "A minidump can contain memory fragments and local paths.\n"
            "Read crash-readme.txt for coverage, fallback location and sharing steps.\n\n" +
            "STABILITY: Reject out-of-range/overflowing TLS slots, preserve owned launch paths,\n"
            "use unique atomic thread IDs and a thread-safe host processor count cache.\n"
            "FILETIME calendar conversions preserve the 1601 epoch and 100 ns precision.\n"
            "Unpinned CPU updates avoid host topology queries and repeated warnings.\n"
            "FPS pacing and ultrawide HUD diagnostics avoid redundant hot-path work.\n"
            "Earlier Windows CPU/UI regressions passed under Wine; revised LAN fix needs native testing.\n\n"
            "ADVANCED: Motion blur (speed), depth of field, dynamic shadows, shadow render distance.\n"
            "Game default keeps the original behavior; Off disables the selected effect.\n"
            "Shadow distance offers game default / 45 / 90 / 180.\n"
            "The shadow override is scoped and preserves the original track tweak on return.\n"
            "FSR 2/3: Experimental Windows D3D12 providers; no frame generation.\n"
            "Synthesized far-depth/zero-motion inputs, history reset each frame.\n"
            "This does not provide proper temporal reconstruction or lower scene render cost.\n"
            "Quality modes resample the resolved scene; FSR 1 remains the safe default.\n"
            "ULTRAWIDE: Auto / 16:9 / 21:9 / 32:9, horizontal-plus camera and central HUD.\n"
            "Wide modes are experimental; broad gameplay and Windows GPU testing is pending.\n\n"
            "GENERAL: Opens on General every launch. Game folder, language and Button prompts.\n"
            + one_pc_notes +
            "The folder and launcher-language controls appear only on General.\n"
            "Game languages: English, French, German, Spanish and Italian.\n"
            "Prompt styles: Automatic, PlayStation 3, Xbox 360 and Keyboard.\n"
            "OUTPUT: Window sizes are capped at the physical monitor's native resolution.\n"
            "An oversized previous window setting falls back to Automatic / desktop in the UI.\n"
            "Internal resolution labels now use 1x / 2x / 3x.\n\n"
            "CAREER TEXTURES: Synchronous GPU resolve readback is enabled only during\n"
            "custom rider/bike livery generation. BOTH matching runtime DLLs are required.\n"
            "13 scope/ABI checks (16 guest calls) passed on Linux and Windows/Wine.\n"
            "The first career race needs local Windows visual verification.\n"
            "LAUNCHER: Removed the lower-left SDK/platform label and its covering box.\n"
            "The decorative stripes remain visible to the bottom of the sidebar.\n\n"
            "PHYSICS DATA: Preserves serialized collision coordinates during endian conversion.\n"
            "192 Linux and Windows/Wine field/record/ABI checks passed.\n"
            "The user verified 4-5 Qatar Time Trial laps without falling through the track.\n"
            "Repair messages in logs contain PHYSICS_ASSET_FIX.\n\n"
            "UPGRADE: Extract the complete ZIP, including resources/controller.\n"
            "Replace the EXEs AND both rexruntime.dll / rexgpu-xenos.dll.\n"
            "Keep your existing game folder, configuration and saves.\n\n"
            "1. ZIP'i program klasorune cikart; motogp0910.exe ve DLL'ler burada kalsin.\n"
            "2. Tum orijinal oyun dosyalarini game altklasorune koy:\n"
            "   game/default.xex, game/xenondata/, game/nxeart/ ve diger disk dosyalari.\n"
            "3. default.xexp patch'ini ve kendi title update dosyan\n"
            "   TU_11K61VG_0000004000000.0000000000081 dosyasini da game icine koy.\n"
            "4. motogp0910-launcher.exe dosyasini ac, ayarlarini sec ve PLAY'e bas.\n"
            "   Arayuz varsayilan olarak Ingilizcedir; General > Launcher language menusunden Turkce secilebilir.\n"
            "   Dil secimi sonraki acilislarda korunur. Derleme gerekmez.\n\n"
            "GRAPHICS: GPU, output size, 720p / 1440p (2K) / 4K internal resolution, native 2x MSAA,\n"
            "FSR 1.0 / CAS sharpness, anisotropic filtering, FXAA and shader cache.\n"
            "FSR is spatial: choose 720p internal resolution for 720p to 1080p upscaling.\n"
            "DISPLAY: Fullscreen, borderless or windowed. Fullscreen can switch monitor Hz.\n"
            "Borderless uses the desktop resolution/Hz; windowed keeps desktop Hz.\n"
            "FPS limit is independent, off by default, and does not unlock the game cap.\n"
            "Selected Hz must be supported at the selected output resolution.\n"
            "AUDIO: Real master volume 0-100% and mute.\n"
            "CONTROLS: Edit any binding directly, or use its ... button to capture a key.\n"
            "Right-click a binding to unbind it or restore its action default.\n"
            "Examples: Space, Return, Ctrl+K, Shift+Up, Q,I. Esc cancels capture.\n"
            "Saved settings apply on next launch, including direct motogp0910.exe launch.\n"
            "Keep both matching runtime DLLs for this nightly stability build.\n\n"
            "REPLAY REPAIR: Replace the upper controller row, preserving the lower media row.\n"
            "All eight full-replay controller sprites now switch Xbox/PS/keyboard styles.\n"
            "The v2 build incorrectly replaced rewind/camera icons instead of the button row.\n\n"
            "PS3 ARTWORK REPAIR: Match the GPU loader's 512-byte BC3 row alignment.\n"
            "The previous PS3 button build uploaded tightly packed 256-byte rows.\n"
            "This build pads rows without changing any visible compressed pixel block.\n\n"
            "IN-GAME PROMPTS: Automatically switch Xbox / PlayStation / keyboard\n"
            "symbols after meaningful input. Keyboard caps show your actual saved bindings.\n"
            "KEYBOARD UPDATE: Rounded dark keycaps, orange MotoGP accents and Open Sans Semibold.\n"
            "The font is embedded; no font installation or additional texture pack is needed.\n"
            "Native Windows/Wine drawing, fade, replay and input checks passed; local visuals need testing.\n"
            "PlayStation: A=cross, B=circle, X=square, Y=triangle; L1/R1 and L2/R2.\n"
            "Original PS3 stick, stick direction, D-pad and SELECT artwork is included.\n"
            "The Second Chance/replay LT/RT/LB/RB/B row also switches styles.\n"
            "Keep resources/controller/ps3-prompts.bin beside the EXE in that subfolder.\n"
            "All 42 controller textures found in the supplied sharedfrontend bank are included.\n"
            "There is no separate START or PS/Guide bitmap in that set; literal START text stays.\n"
            "Native sticks use the original stick pictures; separate L3/R3 labels were not found.\n"
            "To test PS graphics with an XInput pad, open PowerShell here and run:\n"
            ".\\motogp0910.exe --input_backend=xinput --button_prompt_style=playstation\n"
            "Fixed styles: --button_prompt_style=xbox / playstation / keyboard\n"
            "Automatic mode: --button_prompt_style=auto (default).\n"
            "Use the SDL backend for automatic Sony controller recognition; XInput\n"
            "wrappers expose an Xbox controller, so use the manual PlayStation style.\n"
            "Native glyph rendering and virtual Sony/Xbox input checks passed;\n"
            "physical DualSense detection and Windows gameplay visuals need local testing.\n\n"
            "Klavye ve gamepad icin dogrudan motogp0910.exe dosyasini ac; CMD gerekmez.\n"
            "Enter=Start, Space=A, Backspace=B, W/S/A/D=sol stick, Shift+oklar=D-pad.\n"
            "Klavye varsayilan olarak aciktir. Ozel mnk_mode=false ayarin varsa korunur.\n\n"
            "gamecontrollerdb.txt EXE ile ayni klasorde kalmali; game icine koyma.\n"
            "Bu dosya SDL gamepad eslemelerini ekler; input_backend=xinput seciminde kullanilmaz.\n"
            "Kaynak: https://github.com/mdqinc/SDL_GameControllerDB\n\n"
            "Ilk acilista uygulama title update icinden default.xexp dosyasini olusturur;\n"
            "orijinal default.xex degismez. Mevcut default.xexp dosyasi yeniden yazilmaz.\n"
            "game/default.xex varsa game klasoru onceliklidir. Eski duz yerlesim de desteklenir.\n"
            "Kayit/cache konumu onceki EXE ile aynidir; kayit dosyalarini tasimak gerekmez.\n"
            "Bu build 0.0.1.1 oyun surumunu bekler. Windows 10/11 x64 ve uygun Direct3D 12 GPU gerekir.\n\n"
            "This package includes only the requested controller artwork, not full game data or TU.\n"
            f"SDK kaynaklari: https://github.com/rexglue/rexglue-sdk/tree/{sdk['source_commit']}\n",
            encoding="utf-8",
        )
        (STAGE / "game/readme.txt").write_text(
            "Kendi diskinden cikardigin tum oyun dosyalarini bu klasore koy.\n"
            "default.xex, default.xexp veya TU paketi, xenondata/ ve nxeart/\n"
            "ayni game klasorunde olmali. EXE ve DLL dosyalari ust klasorde kalir.\n"
            "Bu paket oyun dosyalarini icermiyor.\n",
            encoding="utf-8",
        )
    # Required notices are embedded and accessible in the launcher. Do not
    # distribute a separate licenses folder, and reject stale launchers or
    # newly added dependencies whose notices have not been incorporated.
    notices = (ROOT / "resources/launcher/third-party-notices.txt").read_text(encoding="utf-8")
    if notices.encode("utf-8") not in (BUILD / "motogp0910-launcher.exe").read_bytes():
        raise SystemExit("Rebuild the launcher with the embedded third-party notices before packaging")
    for notice in (STAGE / "licenses").rglob("*"):
        if notice.is_file() and notice.read_text(encoding="utf-8-sig") not in notices:
            raise SystemExit(f"Missing embedded notice: {notice.relative_to(STAGE / 'licenses')}")
    shutil.rmtree(STAGE / "licenses")
    archive = DIST / args.archive
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as package:
        # Preserve the player's game-data destination even without a README.
        package.writestr("game/", b"")
        for file in sorted(p for p in STAGE.rglob("*") if p.is_file()):
            package.write(file, file.relative_to(STAGE))
    with zipfile.ZipFile(archive) as package:
        assert package.testzip() is None
        assert not any(p.endswith((".xex", ".xexp")) or p.startswith("assets/")
                       for p in package.namelist())
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (DIST / (archive.name + ".sha256")).write_text(f"{digest}  {archive.name}\n")
    print(f"Windows package: {archive} ({archive.stat().st_size} bytes)")
    print(f"SHA-256: {digest}")

if __name__ == "__main__":
    main()
