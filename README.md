# MotoGP 09/10 — ReXGlue v0.10.0

Xbox 360 sürümünün native PC yeniden derleme projesi. Ana hedef verilen stok XEX + title update ile **0.0.1.1** oyun sürümüdür. Windows x64 executable artık derlendi; henüz oynanabilirlik doğrulanmadı. Hedef kullanıcı sistemi Windows 10/11; bu bulut makinesi Linux'tur.

## Doğrulananlar

- Resmi ReXGlue **v0.10.0**, commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` kullanılıyor. 0.9.0 kurulumu ve çıktıları kaldırıldı.
- Verilen XEX ve LIVE/STFS title update: Title ID `434307F0`, Media ID `321F4634`.
- Title update SDK'nın STFS okuyucusuyla açıldı; `default.xexp` çıkarıldı. SDK'nın hash/anahtar kontrolleri korunarak patch uygulandı: `0.0.0.1` → `0.0.1.1`.
- Stok ve TU analizlerindeki birer eksik fonksiyon sınırı gerçek disassembly ile belirlendi. TU manifesti sıfır çözülemeyen çağrıyla kod üretti; `--force` kullanılmadı.
- TU sürümü için 113 C++ translation unit, 30.568 fonksiyon mapping girdisi üretildi. Linux Release executable derlendi.
- Grafik gerektirmeyen entegrasyon probu gerçek güncellenmiş XEX'i, native fonksiyon tablosunu ve oyun verilerini yükledi; oyunun asin/acos rutininde **10/10 sayısal kontrol geçti**. Bu, oyunun açıldığı veya yarışın çalıştığı anlamına gelmez.
- SDK PowerPC altkümesinde **27/27 test, 79 assertion geçti**. Ek chrono testlerinde **12/16 geçti, 4 başarısız**: 1601 epoch'unun Linux nanosecond clock türüne dönüşümünde signed int64 taşması, bağımsız UBSan probuyla doğrulandı. SDK kaynakları değiştirilmedi.
- Linux GUI başlatma denemesi `SDL_INIT_VIDEO ... dummy not available` ile durdu. Bu ortamda ekran/GPU erişimi yok; oynanış sınanamadı.
- Windows x64 PE executable Clang 20/MSVC ABI ve resmi Windows SDK ile çapraz derlendi. Wine altında gerçek güncellenmiş XEX yüklendi; native fonksiyon tablosu ve 10 asin/acos kontrolü geçti. Windows STFS çıkarma aracının çıktısı da beklenen patch SHA-256 değeriyle eşleşti. GUI denemesi bu makinenin ekran/GPU erişimi olmadığı için Direct3D 12 initialization aşamasında durdu. Gerçek Windows GPU ve oynanış testi bekliyor.

## Hazır Windows paketi

[Windows x64 executable paketi](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-preview.zip). Özel depoda GitHub hesabına giriş gerekir. Dosya sayfasındaki **Download raw file** düğmesiyle indir. Alternatif olarak “Code → Download ZIP” ile depoyu indir; hazır executable paketi `downloads/MotoGP0910-Windows-x64-preview.zip` içindedir.

1. ZIP içindeki `.exe` ve DLL dosyalarını tam oyun klasöründe, `default.xex` dosyasının yanına çıkar.
2. Elindeki `TU_11K61VG_0000004000000.0000000000081` title update dosyasını da aynı klasöre koy.
3. `motogp0910.exe` dosyasını aç. Derleme araçlarını kurmak gerekmez.

Uygulama ilk açılışta title update'ten `default.xexp` dosyasını çıkarır; orijinal XEX'i değiştirmez, mevcut patch'i yeniden yazmaz. Üretilen kodun hedeflediği 0.0.1.1 sürümü yüklenmezse uygulama oyunu başlatmayı reddeder. Tam oyun, writable klasörde ve orijinal dizin yapısıyla kalmalıdır. Kayıt/cache konumu SDK'nın kullanıcı klasörüdür. Oyun ikilileri, patch ve 3 GB oyun verisi dağıtım paketinde bulunmaz.

Bu paket bir prototiptir: Windows ikilisi ve veri/kod yükleme zinciri doğrulandı; menü, yarış, ses ve kontrolcü henüz sınanmadı. Windows 10/11 x64 ve uygun Direct3D 12 GPU gerekir.

## Windows'ta kaynaktan derleme

3 GB'lık oyun verisini buluta yüklemek gerekmez. Proje paketini kendi bilgisayarında açıp tam çıkarılmış oyun klasörünü orada kullanabilirsin. Oyun verisi bu proje paketinde bulunmaz.

Gerekli araçlar: Visual Studio 2022'nin Desktop development with C++ bileşenleri, Windows SDK, **Clang 20+**, CMake 3.25+ ve Ninja. x64 VS Developer PowerShell aç; `clang++`, `cmake`, `ninja` PATH'te olmalı.

Proje klasöründe şu komutu kendi dosya yollarınla çalıştır:

```powershell
.\scripts\build-windows.ps1 `
  -GameRoot 'D:\Games\MotoGP0910' `
  -TitleUpdatePath 'D:\Updates\TU_11K61VG_0000004000000.0000000000081' `
  -Jobs 2
```

Betik resmi Windows AMD64 SDK **0.10.0** paketini indirip GitHub yayınındaki SHA-256 ile doğrular. Verilen XEX/TU hash'lerini kontrol eder; STFS çıkarma aracını derler, patch'i çıkarır, yalnızca XEX/patch'i yerel `assets/game-tu` altına hazırlar. Tam oyun klasörünü kopyalamaz. Kod üretimini, Release derlemesini ve headless entegrasyon probunu çalıştırır. Native komutların hataları saklanmaz.

Başarılı yerel derlemenin çıktısı: `out/build/win-amd64-release/motogp0910.exe`. SDK runtime DLL'leri ve Xenos GPU plugin'i yanında hazırlanır. Aynı hedef için bulutta ayrıca Windows çapraz derlemesi yapıldı; yerel betik kendi Windows makinesinde tekrar derlemek içindir.

Tam oyun klasörüyle çalıştırma denemesi:

```powershell
.\scripts\run-windows.ps1 -GameRoot 'D:\Games\MotoGP0910'
```

Çalıştırma betiği orijinal XEX hash'ini kontrol eder. Çıkarılan/doğrulanan `default.xexp` dosyasını tam oyun klasörüne yalnızca yoksa ekler; var olan farklı patch'i değiştirmez. Orijinal `default.xex` ve oyun veri dosyaları korunur. SDK patch'i çalışma sırasında tekrar uygular; üretilmiş fonksiyon tablosu bu sürümle eşleşir. Kayıtlar `out/userdata`, log `out/motogp-windows.log` altına gider.

İlk Windows çalıştırmasının loguna göre grafik, input, ses, oyun verisi erişimi ve varsa eksik kernel davranışları ayrıca incelenmelidir. Oynanabilirlik iddiası için menü ve gerçek yarış doğrulaması gerekir.

## Bulut ortamında devam

Görev zaten izole bir makinededir; mevcut checkout'u kullan, ayrıca Git worktree oluşturma.

```bash
cd /workspace/motogp0910-recomp
bash scripts/install-cloud.sh
source scripts/cloud-env.sh
python3 scripts/check-game-inputs.py
rexglue codegen --ignore-stamp motogp0910_manifest.toml
cmake --preset linux-amd64-release -DCMAKE_CXX_COMPILER="$CXX"
cmake --build --preset linux-amd64-release --parallel 4
cmake --build --preset linux-amd64-release --target motogp_headless_probe --parallel 4
out/build/linux-amd64-release/motogp_headless_probe assets/game-tu
```

Kurulum Debian 13'te root gerektirmeden `/workspace/shared` altında yapılır. Debian paket imzaları ve SDK checksum'ları kontrol edilir. Bulut Clang 19'u SDK'nın Clang 18+ gereksinimini karşılar; gerçek oyun ve SDK testleriyle sınandı. Her yeni shell'de `cloud-env.sh` tekrar source edilmelidir. Yayınlama canlı süreçleri korumaz.

SDK testleri:

```bash
bash scripts/smoke-sdk.sh
# Tam komut bilinen dört chrono hatası nedeniyle nonzero döner.
# PowerPC altkümesini ayrı çalıştır:
out/smoke/build/ppc_smoke '[ppc]'
```

SDK'nın kod üretimi fingerprint'inin patch dosyasını izlediğini varsayma. XEX/patch değişince hash kontrolünü ve `--ignore-stamp` ile kod üretimini tekrar yap; başka oyun sürümü için adres sınırlarını yeniden analiz et. Stok sürüme ait sınır TU manifestinde kullanılmaz.

## Girdi hash'leri

| Dosya | SHA-256 |
|---|---|
| Stok `default.xex` | `54245417e996ed0aa561a5a803fd810e87684bd0b43082f316db711b3ee786ae` |
| Verilen TU paketi | `441df6cc2e20ac8aeeed475138c1714d477f965423d8c5c9b0202e3a18d8141f` |
| Çıkarılan `default.xexp` | `0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079` |
| Linux AMD64 SDK ZIP | `0388edb3f8fb1444f1b0ad5d440cb02d9690ac20a76b04fd6b7dcc19f59328a6` |
| Windows AMD64 SDK ZIP | `ad4485da853fbe932e9f5dbb643f9b5fe85dc73775ebbad4df6879fe6df0c11b` |

## Dosya düzeni

`assets/game/default.xex` stok kopyadır. `assets/game-tu/` analiz/derleme için orijinal XEX + kardeş `default.xexp` içerir. `assets/updates/` orijinal title update ve çıkarılan patch'i tutar. `generated/default/` oyun kodudur. Bu dosyalar ve `out/` Git'e eklenmez. `generated/rexglue.cmake` SDK tarafından yönetilir.

`tools/motogp_inspect` SDK loader'larıyla XEX section dump ve STFS çıkarma sağlar; var olan çıktı dizinini/dosyaları değiştirmeyi reddeder. Analiz notları `docs/analysis.md`, bu ortamın ham raporları `out/analysis/` altındadır.

Resmi SDK: https://github.com/rexglue/rexglue-sdk/tree/v0.10.0

## Linux bulutundan Windows derlemesi

```bash
cd /workspace/motogp0910-recomp
bash scripts/install-windows-cross.sh
bash scripts/build-windows-cross.sh
python3 scripts/package-windows.py
```

Microsoft SDK/CRT xwin 0.10.0 ile sabit VS17 manifestinden hazırlanır. Windows kaynakları Clang 20 ile MSVC ABI'sine derlenir; kod üretimi aynı ReXGlue 0.10.0 Linux host aracında yapılır. Compiler resource include yolu Microsoft include yollarından önce gelmelidir; aksi halde MMX ve constexpr offsetof hataları çıkar. SDK/library doğrulamaları ve static assertion'lar korunur. SDK paketleri checksum, LLVM paketleri APT imza kontrolünden geçer. Önbellek ve araçlar `/workspace/shared` altındadır.

Çıktı `out/build/windows-cross-release/motogp0910.exe`; paket `out/dist/MotoGP0910-Windows-x64-preview.zip` olur. Wine yalnızca ek yükleme/CPU probu içindir; gerçek Windows grafik doğrulamasının yerine geçmez.
