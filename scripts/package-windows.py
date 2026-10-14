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
    args=parser.parse_args();BUILD=args.build_dir.resolve()
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
    shutil.copytree(ROOT / "third_party/rexglue-runtime/licenses", STAGE / "licenses", dirs_exist_ok=True)
    shutil.copy2(ROOT / "third_party/rexglue-runtime/manifest.json", STAGE / "runtime-manifest.json")
    shutil.copytree(ROOT / "resources/controller", STAGE / "resources/controller")
    milestone="Experimental LAN multiplayer" if args.lan_preview else "Stability update"
    preview_notes=(
        "EXPERIMENTAL LAN: Guest XNet/session and UDP/VDP socket integration is included.\n"
        "General > Enable LAN (Experimental); automatic rooms, optional host IPv4 fallback.\n"
        "Use the same package on both PCs and unranked Player Match on LAN / Radmin.\n"
        "No Xbox Live login or dedicated server. Offline remains the default.\n"
        "LAN startup fix: 17 targeted actual-SDK Windows/Wine checks passed.\n"
        "Empty Auto-IP command-line values no longer swallow the next option.\n"
        "This build is compiled, but synchronized racing and real Radmin are UNVERIFIED.\n"
        "Read lan-readme.txt for the exact two-PC steps and unsupported features.\n\n"
        "ICONS: Game EXE uses the supplied MotoGP 09/10 cover. Launcher EXE uses\n"
        "the supplied MotoGP logo. Both embed seven Windows icon sizes (16-256 px).\n\n"
    ) if args.lan_preview else ""
    if args.lan_preview:
        shutil.copy2(ROOT / "docs/lan-multiplayer-test.md", STAGE / "lan-readme.txt")
        shutil.copy2(ROOT / "docs/lan-startup-fix.md", STAGE / "lan-startup-fix.txt")
    shutil.copy2(ROOT / "docs/crash-reports.md", STAGE / "crash-readme.txt")
    (STAGE / "readme.txt").write_text(
        f"MotoGP 09/10 - Windows x64 {milestone} - ReXGlue {sdk['version']}\n\n" + preview_notes +
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
        "Earlier Windows CPU/UI regressions passed under Wine; LAN racing remains unverified.\n\n"
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
        "GENERAL: Game folder, Launcher language, Game language and Button prompts.\n"
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
        "SDK kaynaklari: https://github.com/rexglue/rexglue-sdk/tree/v0.10.0\n",
        encoding="utf-8",
    )
    (STAGE / "game").mkdir()
    (STAGE / "game/readme.txt").write_text(
        "Kendi diskinden cikardigin tum oyun dosyalarini bu klasore koy.\n"
        "default.xex, default.xexp veya TU paketi, xenondata/ ve nxeart/\n"
        "ayni game klasorunde olmali. EXE ve DLL dosyalari ust klasorde kalir.\n"
        "Bu paket oyun dosyalarini icermiyor.\n",
        encoding="utf-8",
    )
    files = sorted(p for p in STAGE.rglob("*") if p.is_file())
    hashes = "".join(
        f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(STAGE).as_posix()}\n"
        for p in files
    )
    (STAGE / "SHA256SUMS.txt").write_text(hashes)
    archive = DIST / args.archive
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as package:
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
