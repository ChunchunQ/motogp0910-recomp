#!/usr/bin/env python3
"""Package the cross-built Windows application; original game files stay out."""
from pathlib import Path
import hashlib
import shutil
import zipfile
import json

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "out/build/windows-cross-release"
STAGE = ROOT / "out/package-windows"
DIST = ROOT / "out/dist"
RUNTIME = (
    "rexruntime.dll", "rexgpu-xenos.dll", "msvcp140.dll",
    "msvcp140_atomic_wait.dll", "vcruntime140.dll", "vcruntime140_1.dll",
)

def main():
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
    for name in ("motogp0910.exe", "motogp0910-launcher.exe", *RUNTIME):
        source = BUILD / name
        if not source.is_file():
            raise SystemExit(f"Missing build output: {name}")
        shutil.copy2(source, STAGE / name)
    licenses = Path("/workspace/shared/rexglue-win-sdk/win-amd64/licenses")
    if licenses.is_dir():
        shutil.copytree(licenses, STAGE / "licenses", dirs_exist_ok=True)
    shutil.copy2("/workspace/shared/rexglue-sdk/LICENSE", STAGE / "licenses/ReXGlue-LICENSE.txt")
    controller_db = ROOT / "third_party/SDL_GameControllerDB"
    shutil.copy2(controller_db / "gamecontrollerdb.txt", STAGE / "gamecontrollerdb.txt")
    shutil.copy2(controller_db / "LICENSE.txt", STAGE / "licenses/SDL_GameControllerDB-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/tomlplusplus-LICENSE.txt", STAGE / "licenses/tomlplusplus-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/stb/LICENSE.txt", STAGE / "licenses/stb-LICENSE.txt")
    shutil.copytree(ROOT / "third_party/rexglue-runtime/licenses", STAGE / "licenses", dirs_exist_ok=True)
    shutil.copy2(ROOT / "third_party/rexglue-runtime/manifest.json", STAGE / "runtime-manifest.json")
    shutil.copytree(ROOT / "resources/controller", STAGE / "resources/controller")
    (STAGE / "readme.txt").write_text(
        "MotoGP 09/10 - Windows x64 career texture readback repair - ReXGlue v0.10.0\n\n"
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
        "   Arayuz varsayilan olarak Ingilizcedir; Language / Dil menusunden Turkce secilebilir.\n"
        "   Dil secimi sonraki acilislarda korunur. Derleme gerekmez.\n\n"
        "GRAPHICS: GPU, output size, 720p / 1440p (2K) / 4K internal resolution, native 2x MSAA,\n"
        "FSR 1.0 / CAS sharpness, anisotropic filtering, FXAA and shader cache.\n"
        "FSR is spatial: choose 720p internal resolution for 720p to 1080p upscaling.\n"
        "Fullscreen uses the desktop size; output window size is for windowed mode.\n"
        "AUDIO: Real master volume 0-100% and mute.\n"
        "CONTROLS: Edit any binding directly, or use its ... button to capture a key.\n"
        "Right-click a binding to unbind it or restore its action default.\n"
        "Examples: Space, Return, Ctrl+K, Shift+Up, Q,I. Esc cancels capture.\n"
        "Saved settings apply on next launch, including direct motogp0910.exe launch.\n"
        "Keep both runtime DLLs: this v0.10.0 pair enables spatial FSR and shared SDL input.\n\n"
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
    archive = DIST / "MotoGP0910-Windows-x64-career-texture-fix.zip"
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
