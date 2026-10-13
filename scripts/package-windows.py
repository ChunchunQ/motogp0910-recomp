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
    shutil.copytree(ROOT / "third_party/rexglue-runtime/licenses", STAGE / "licenses", dirs_exist_ok=True)
    shutil.copy2(ROOT / "third_party/rexglue-runtime/manifest.json", STAGE / "runtime-manifest.json")
    (STAGE / "readme.txt").write_text(
        "MotoGP 09/10 - Windows x64 advanced launcher - ReXGlue v0.10.0\n\n"
        "1. ZIP'i program klasorune cikart; motogp0910.exe ve DLL'ler burada kalsin.\n"
        "2. Tum orijinal oyun dosyalarini game altklasorune koy:\n"
        "   game/default.xex, game/xenondata/, game/nxeart/ ve diger disk dosyalari.\n"
        "3. default.xexp patch'ini ve kendi title update dosyan\n"
        "   TU_11K61VG_0000004000000.0000000000081 dosyasini da game icine koy.\n"
        "4. motogp0910-launcher.exe dosyasini ac, ayarlarini sec ve PLAY'e bas.\n"
        "   Arayuz varsayilan olarak Ingilizcedir; Language / Dil menusunden Turkce secilebilir.\n"
        "   Dil secimi sonraki acilislarda korunur. Derleme gerekmez.\n\n"
        "GRAPHICS: GPU, output size, internal X/Y scale, native 2x MSAA,\n"
        "FSR 1.0 / CAS sharpness, anisotropic filtering, FXAA and shader cache.\n"
        "FSR is spatial: use 1x internal scale for native 720p to 1080p upscaling.\n"
        "Fullscreen uses the desktop size; output window size is for windowed mode.\n"
        "AUDIO: Real master volume 0-100% and mute.\n"
        "CONTROLS: Select a row; edit the binding or click Press a key.\n"
        "Examples: Space, Return, Ctrl+K, Shift+Up, Q,I. Esc cancels capture.\n"
        "Saved settings apply on next launch, including direct motogp0910.exe launch.\n"
        "Keep both runtime DLLs: this modified v0.10.0 pair enables spatial FSR.\n"
        "In-game PlayStation/keyboard prompt replacement is not implemented yet.\n"
        "Windows GPU rendering with the new options still needs local testing.\n\n"
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
        "Bu paket orijinal oyun dosyalarini veya title update'i icermiyor.\n"
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
    archive = DIST / "MotoGP0910-Windows-x64-advanced-launcher.zip"
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
