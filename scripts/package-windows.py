#!/usr/bin/env python3
"""Package native or cross-built Windows outputs; original game files stay out."""
from pathlib import Path
import hashlib
import shutil
import zipfile
import json
import argparse

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "out/build/win-amd64-release"
STAGE = ROOT / "out/package-windows"
DIST = ROOT / "out/dist"
RUNTIME = (
    "rexruntime.dll", "rexgpu-xenos.dll", "amd_fidelityfx_dx12.dll", "msvcp140.dll",
    "msvcp140_atomic_wait.dll", "vcruntime140.dll", "vcruntime140_1.dll",
)

def main():
    global BUILD
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'out/build/win-amd64-release')
    parser.add_argument('--archive',default='MotoGP0910-Windows-x64-stability-update.zip')
    parser.add_argument('--sdk-root', type=Path, help='Optional SDK root for additional dependency license notices')
    parser.add_argument('--lan-preview',action='store_true',help='Accepted for compatibility; production archives omit preview instructions')
    parser.add_argument('--online-preview',action='store_true',help='Accepted for compatibility; production archives omit preview instructions')
    args=parser.parse_args();BUILD=args.build_dir.resolve()
    if Path(args.archive).name!=args.archive or not args.archive.endswith('.zip'):
        parser.error('--archive must be a ZIP filename')
    # Recreate only our packaging directory so previous checksums or stale
    # files cannot leak into a new release.
    if STAGE.exists():
        shutil.rmtree(STAGE)
    STAGE.mkdir(parents=True, exist_ok=True)
    binaries=STAGE/"bin"
    binaries.mkdir()
    DIST.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((ROOT / "third_party/rexglue-runtime/manifest.json").read_text())
    for name, expected in manifest["dll_sha256"].items():
        if hashlib.sha256((BUILD / name).read_bytes()).hexdigest() != expected:
            raise SystemExit(f"Runtime mismatch: {name}; stage the spatial FSR pair first")
    for name in ("motogp0910.exe", "motogp0910-ui.dll", "motogp0910-update-helper.exe", "motogp0910-crash-monitor.exe", "motogp0910.map", *RUNTIME):
        source = BUILD / name
        if not source.is_file():
            raise SystemExit(f"Missing build output: {name}")
        shutil.copy2(source, binaries / name)
    shutil.copy2(BUILD / "MotoGP09-10Launcher.exe", STAGE / "MotoGP09-10Launcher.exe")
    (STAGE / 'licenses').mkdir()
    licenses = args.sdk_root / 'licenses' if args.sdk_root else None
    if licenses and licenses.is_dir():
        shutil.copytree(licenses, STAGE / "licenses", dirs_exist_ok=True)
    # The pinned runtime's tracked notices include the matching ReXGlue license.
    controller_db = ROOT / "third_party/SDL_GameControllerDB"
    shutil.copy2(controller_db / "gamecontrollerdb.txt", binaries / "gamecontrollerdb.txt")
    shutil.copy2(controller_db / "LICENSE.txt", STAGE / "licenses/SDL_GameControllerDB-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/tomlplusplus-LICENSE.txt", STAGE / "licenses/tomlplusplus-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/stb/LICENSE.txt", STAGE / "licenses/stb-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/open-sans/LICENSE.txt", STAGE / "licenses/OpenSans-LICENSE.txt")
    shutil.copy2(ROOT / "third_party/open-sans/NOTICE.txt", STAGE / "licenses/OpenSans-NOTICE.txt")
    shutil.copy2(ROOT / "third_party/launcher-fonts/OFL.txt", STAGE / "licenses/LauncherFonts-OFL.txt")
    shutil.copy2(ROOT / "third_party/launcher-fonts/NOTICE.txt", STAGE / "licenses/LauncherFonts-NOTICE.txt")
    shutil.copy2(ROOT / "third_party/nlohmann/LICENSE.MIT", STAGE / "licenses/Nlohmann-json-LICENSE.txt")
    shutil.copytree(ROOT / "third_party/rexglue-runtime/licenses", STAGE / "licenses", dirs_exist_ok=True)
    shutil.copy2(ROOT / "third_party/rexglue-runtime/manifest.json", binaries / "runtime-manifest.json")
    shutil.copytree(ROOT / "resources/controller", STAGE / "resources/controller",
                    ignore=shutil.ignore_patterns("readme*", "README*"))
    shutil.copytree(ROOT / "resources/localization/tr/overlay", STAGE / "resources/localization/tr/overlay")
    shutil.copy2(ROOT / "resources/localization/tr/manifest.json", STAGE / "resources/localization/tr/manifest.json")
    (STAGE / "resources/ui").mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / "third_party/open-sans/OpenSans-Semibold.ttf", STAGE / "resources/ui/OpenSans-Semibold.ttf")
    steam_target = STAGE / "resources/steam"
    steam_target.mkdir(parents=True, exist_ok=True)
    for name in ("icon.png", "logo.png", "banner.png", "cover.png", "wide.png"):
        source = ROOT / "resources/steam" / name
        if source.is_file():
            shutil.copy2(source, steam_target / name)
    archive_reader = ROOT / "third_party/7zip"
    archive_target = STAGE / "resources/archive"
    archive_target.mkdir(parents=True, exist_ok=True)
    archive_manifest = json.loads((archive_reader / "manifest.json").read_text())
    for entry in archive_manifest["files"]:
        source = archive_reader / entry["name"]
        if source.stat().st_size != entry["size"] or hashlib.sha256(source.read_bytes()).hexdigest() != entry["sha256"]:
            raise SystemExit(f"Archive reader integrity check failed: {entry['name']}")
        if entry["name"] in ("7z.exe", "7z.dll", "7zip-source.tar.xz"):
            shutil.copy2(source, archive_target / entry["name"])
    if (archive_reader / "License.txt").read_text() not in (ROOT / "resources/launcher/third-party-notices.txt").read_text():
        raise SystemExit("Missing embedded 7-Zip notice")
    # Runtime resources and legal notices are always included. User-facing
    # packages contain no developer instructions, test helpers or README files.
    (STAGE / "game").mkdir()
    (STAGE / "dlc").mkdir()
    # Required notices are embedded and accessible in the launcher. Do not
    # distribute a separate licenses folder, and reject stale launchers or
    # newly added dependencies whose notices have not been incorporated.
    notices_path = ROOT / "resources/launcher/third-party-notices.txt"
    notices = notices_path.read_text(encoding="utf-8")
    # RCDATA embeds exact checkout bytes, including CRLF on native Windows.
    if notices_path.read_bytes() not in (BUILD / "MotoGP09-10Launcher.exe").read_bytes():
        raise SystemExit("Rebuild the launcher with the embedded third-party notices before packaging")
    for notice in (STAGE / "licenses").rglob("*"):
        if notice.is_file() and notice.read_text(encoding="utf-8-sig") not in notices:
            raise SystemExit(f"Missing embedded notice: {notice.relative_to(STAGE / 'licenses')}")
    shutil.rmtree(STAGE / "licenses")
    archive = DIST / args.archive
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as package:
        # Preserve empty setup destinations without including any player content.
        package.writestr("game/", b"")
        package.writestr("dlc/", b"")
        for file in sorted(p for p in STAGE.rglob("*") if p.is_file()):
            package.write(file, file.relative_to(STAGE))
    with zipfile.ZipFile(archive) as package:
        assert package.testzip() is None
        assert "bin/motogp0910-launcher.exe" not in package.namelist()
        assert "bin/motogp0910-ui.dll" in package.namelist()
        assert "bin/motogp0910-update-helper.exe" in package.namelist()
        assert not any(p.startswith("logs/") for p in package.namelist())
        assert not any(p.endswith((".xex", ".xexp")) or p.startswith("assets/")
                       for p in package.namelist())
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    print(f"Windows package: {archive} ({archive.stat().st_size} bytes)")
    print(f"SHA-256: {digest}")

if __name__ == "__main__":
    main()
