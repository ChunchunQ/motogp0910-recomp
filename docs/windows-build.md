# Windows 10/11 x64 üzerinde derleme

Güncel `main` kaynakları, ilk deneme paketindeki `0x8270DF08` startup callback keşif hatasını düzeltir. `downloads/` içindeki eski `v0.1.0-preview` executable bu düzeltmeyi içermez. Aşağıdaki betikler güncel kaynaklardan yeni executable üretir. ReXGlue **v0.10.0** kullanılır.

## Bir kez kurulacak araçlar

1. [Visual Studio 2022 Build Tools](https://aka.ms/vs/17/release/vs_BuildTools.exe) kur. **Desktop development with C++ / C++ ile masaüstü geliştirme** iş yükünü seç. MSVC v143 x64/x86 araçları ve Windows 10/11 SDK bileşenleri seçili olsun. Tam Visual Studio 2022 zaten varsa aynı bileşenleri Visual Studio Installer'dan eklemek yeterli.
2. [Resmi LLVM 20.1.8 Windows x64 installer](https://github.com/llvm/llvm-project/releases/download/llvmorg-20.1.8/LLVM-20.1.8-win64.exe) kur. PATH'e ekleme seçeneğini aç. Betik varsayılan `C:\Program Files\LLVM` kurulumunu ayrıca bulur. MSVC hedefli Windows LLVM gerekir.
3. **CMake 3.25+** ve **Ninja** kur. Windows Terminal/PowerShell'de winget varsa:

   ```powershell
   winget install --exact --id Kitware.CMake
   winget install --exact --id Ninja-build.Ninja
   ```

   Winget yoksa [CMake installer](https://cmake.org/download/) ve [Ninja Windows ZIP](https://github.com/ninja-build/ninja/releases) kullan; `cmake.exe` ve `ninja.exe` PATH'te olmalı.

Araç kurulumlarından sonra **yeni bir PowerShell penceresi** aç. Betik VS 2022'nin x64 ortamını kendisi açar; özel Developer PowerShell penceresi bulmak zorunda değilsin. LLVM'nin Windows x64 sürümünü kullan; MinGW GCC/Clang bu SDK ile uyumlu değildir.

SDK programı eksik `MSVCP140` / `VCRUNTIME140` DLL hatası verirse [Microsoft Visual C++ x64 Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) kur.

## Kaynakları indir

[Düzeltilmiş kaynak ZIP'ini](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-source-callback-fix.zip) GitHub hesabına giriş yaptıktan sonra **Download raw file** düğmesiyle indir. ZIP içindeki `motogp0910-recomp` klasörünü örneğin `D:\dev` altına çıkar. Bu klasörde `CMakeLists.txt`, `motogp0910_manifest.toml`, `src/`, `tools/` ve `scripts/` bulunmalı.

Alternatif: [proje deposunda](https://github.com/inflamesgo/motogp0910-recomp) **Code → Download ZIP** seç; bu daha büyük paket eski executable arşivini de içerir. Proje klasörü bu yöntemde `motogp0910-recomp-main` adını alır; aşağıdaki `cd` yolunu buna göre değiştir.

Git kullanıyorsan alternatif:

```powershell
git clone https://github.com/inflamesgo/motogp0910-recomp.git
cd motogp0910-recomp
```

Var olan klonu güncellemek için yerel değişikliklerini koruyarak `git pull --ff-only` kullan. ZIP kullandıysan yeni ZIP'i ayrı bir klasöre çıkar.

Gerekli proje dosyaları depoda bulunur. ReXGlue SDK betik tarafından indirilip SHA-256 ile doğrulanır. Oyun kodu senin XEX + TU dosyalarından yerel olarak üretilir; ayrı bir generated kod paketi gerekmez. Python, Linux, xwin veya SDK kaynaklarını derlemek gerekmez.

## Derle

PowerShell'de kaynak klasörüne geç. `GameRoot`, **stok `default.xex` dosyasının bulunduğu tam oyun klasörü**; `TitleUpdatePath`, elindeki orijinal TU dosyasının tam yoludur. Logundaki oyun köküne göre örnek:

```powershell
cd 'D:\dev\motogp0910-recomp'
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

$GameRoot = 'D:\x\motogp0910\MotoGP0910-Windows-x64-preview'
$TitleUpdate = Join-Path $GameRoot 'TU_11K61VG_0000004000000.0000000000081'

.\scripts\build-windows.ps1 `
  -GameRoot $GameRoot `
  -TitleUpdatePath $TitleUpdate `
  -Jobs 4 *>&1 | Tee-Object -FilePath '.\build-windows.log'
```

Yolları kendi bilgisayarına göre değiştir. Sadece XEX/TU ile derleme yapılabilir; çalıştırma için oyun kökünde tam veri dizinleri de bulunmalı. ExecutionPolicy ayarı yalnızca bu PowerShell penceresinde geçerlidir.

Başlangıç için 8 GB RAM'de `-Jobs 2`, 16 GB'da `-Jobs 4`, 32 GB ve üstünde `-Jobs 8` kullanabilirsin; işlemci çekirdek sayısı da uygun olmalı. Bellek yetersizliği olursa Jobs değerini düşür. İlk derleme tüm oyun kodunu derler; sonraki derlemeler değişen dosyaları kullanır.

Betik orijinal dosyaların hash'lerini kontrol eder, title update'i çıkarır, XEX/patch kopyalarını yalnızca proje içindeki `assets/` altında hazırlar ve orijinal XEX'i değiştirmez. 3 GB oyun verisini kopyalamaz. Kod üretiminden sonra uygulamayı ve grafik gerektirmeyen entegrasyon probunu derler. Hata halinde durur.

Beklenen test çıktısı:

```text
Patched XEX 0.0.1.1 loaded; native function mapping verified; 10 asin/acos and 5 startup callback checks passed. Gameplay was not exercised.
```

Yeni executable: **`out\build\win-amd64-release\motogp0910.exe`**. Runtime DLL'leri ve Xenos plugin'i aynı yerde hazırlanır. Derleme tamamlanmadan eski paketteki executable'ı çalıştırmak bu düzeltmeyi denemez.

## Yeni derlemeyi çalıştır

Aynı PowerShell penceresinde:

```powershell
.\scripts\run-windows.ps1 -GameRoot $GameRoot
```

Bu komut yeni `out\build\win-amd64-release\motogp0910.exe` dosyasını tam oyun köküyle çalıştırır. Doğru patch oyun kökünde yoksa ekler; farklı mevcut patch'i değiştirmez. Kayıtlar `out\userdata`, oyun logu **`out\motogp-windows.log`** altında olur.

Sonraki düzeltmelerde kaynakları güncelledikten sonra aynı build ve run komutlarını tekrar kullan. `out/`, `assets/` ve `generated/` klasörlerini silmek ilk derlemenin tekrar yapılmasına yol açar.

Derleme hatası olursa `build-windows.log`; oyun tekrar çökerse `out\motogp-windows.log` tanı için gereken dosyadır. Mevcut düzeltme bildirilen callback adresini kapsar; menü ve yarışın çalıştığı hâlâ doğrulanmalıdır.
