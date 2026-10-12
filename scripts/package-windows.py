#!/usr/bin/env python3
"""Package the cross-built Windows application; original game files stay out."""
from pathlib import Path
import hashlib
import shutil
import zipfile

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
    for name in ("motogp0910.exe", *RUNTIME):
        source = BUILD / name
        if not source.is_file():
            raise SystemExit(f"Missing build output: {name}")
        shutil.copy2(source, STAGE / name)
    licenses = Path("/workspace/shared/rexglue-win-sdk/win-amd64/licenses")
    if licenses.is_dir():
        shutil.copytree(licenses, STAGE / "licenses", dirs_exist_ok=True)
    shutil.copy2("/workspace/shared/rexglue-sdk/LICENSE", STAGE / "licenses/ReXGlue-LICENSE.txt")
    (STAGE / "BASLA.txt").write_text(
        "MotoGP 09/10 - Windows x64 game klasoru duzeni - ReXGlue v0.10.0\n\n"
        "1. ZIP'i program klasorune cikart; motogp0910.exe ve DLL'ler burada kalsin.\n"
        "2. Tum orijinal oyun dosyalarini game altklasorune koy:\n"
        "   game/default.xex, game/xenondata/, game/nxeart/ ve diger disk dosyalari.\n"
        "3. default.xexp patch'ini ve kendi title update dosyan\n"
        "   TU_11K61VG_0000004000000.0000000000081 dosyasini da game icine koy.\n"
        "4. Ust klasordeki motogp0910.exe dosyasini ac. Derleme gerekmez.\n\n"
        "Klavye ile kontrol icin BASLAT-KLAVYE.cmd dosyasini ac: Enter=Start, Space=A,\n"
        "Backspace=B, W/S/A/D=sol stick, Shift+oklar=D-pad. Gamepad icin exe'yi acabilirsin.\n\n"
        "Ilk acilista uygulama title update icinden default.xexp dosyasini olusturur;\n"
        "orijinal default.xex degismez. Mevcut default.xexp dosyasi yeniden yazilmaz.\n"
        "game/default.xex varsa game klasoru onceliklidir. Eski duz yerlesim de desteklenir.\n"
        "Kayit/cache konumu onceki EXE ile aynidir; kayit dosyalarini tasimak gerekmez.\n"
        "Bu build 0.0.1.1 oyun surumunu bekler. Windows 10/11 x64 ve uygun Direct3D 12 GPU gerekir.\n\n"
        "Bu paket orijinal oyun dosyalarini veya title update'i icermiyor.\n"
        "Bu paket eksik CRT callback/initializer, kaynak karsilastiricisi ve Bink/arayuz\n"
        "fonksiyon kesiflerini duzeltir. Gercek oyun kodu uretilir; oyun rutinleri stub degildir.\n"
        "Start sonrasi logdaki 0x82271650 kayit disi callback de bu surumde uretilmistir.\n"
        "Windows PE probunda Wine altinda 50 davranis kontrolu gecti. Tam yaris testi bekliyor.\n"
        "Linux'ta tam oyun verileriyle grafik acilisi ve native regresyon kontrolleri sinandi.\n"
        "Guncel Windows GPU davranisi kendi bilgisayarinda ayrica dogrulanmalidir.\n"
        "Yaris ve duyulabilir ses kalitesi henuz dogrulanmadi.\n"
        "SDK kaynaklari: https://github.com/rexglue/rexglue-sdk/tree/v0.10.0\n",
        encoding="utf-8",
    )
    (STAGE / "BASLAT-KLAVYE.cmd").write_bytes(
        b'@echo off\r\n"%~dp0motogp0910.exe" --mnk_mode=true\r\n'
    )
    (STAGE / "game").mkdir()
    (STAGE / "game/OYUN-DOSYALARINI-BURAYA.txt").write_text(
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
    archive = DIST / "MotoGP0910-Windows-x64-game-layout.zip"
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
