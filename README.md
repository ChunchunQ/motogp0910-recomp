# MotoGP 09/10 — ReXGlue v0.10.0

Xbox 360 sürümünün native PC yeniden derleme projesi. Ana hedef verilen stok XEX + title update ile **0.0.1.1** oyun sürümüdür. Windows x64 executable derlendi; **Linux üzerinde tam oyun verileriyle ana menüye ulaşma doğrulandı**. [Güncel Windows EXE + DLL paketi hazır](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-ps3-buttons-v3.zip). Hedef kullanıcı sistemi Windows 10/11; bu bulut makinesi Linux'tur.

[Geliştirme notları ve yol haritası](docs/roadmap.md): [game/ dosya düzeni tamamlandı](docs/game-layout.md); [Yenilenen İngilizce/Türkçe launcher; 720p/1440p/4K, FSR 1.0, ses ve sade tuş eşlemeleri](docs/launcher.md). Stabilite/kalite kontrol ve LAN multiplayer araştırması sonraki aşamalardır. Kullanıcı son yarış başlangıcı düzeltmesinden sonra oyunun düzgün çalıştığını bildirdi; tüm mod/pistlerin kalite kontrolü henüz tamamlanmadı.

**Güncel kullanım: [düzeltilmiş kaynak paketi](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-source-collision-data-fix.zip) ve [Windows yerel derleme rehberi](docs/windows-build.md).** Güncel paket önceki başlangıç düzeltmelerini ve yarışa geçişteki `0x82271650` callback kaydını içerir. Manifest, eksik CRT callback/initializer, kaynak karşılaştırıcısı ve video/arayüz fonksiyonlarını gerçek komut sınırlarıyla üretime dahil eder; oyunun rutinleri stub ile değiştirilmez. [Gerçek Linux açılış görüntüleri](docs/screenshots/) ve [Linux grafik denemesini yeniden çalıştırma adımları](docs/linux-startup.md) depoda bulunur.


## Doğrulananlar

- ReXGlue **v0.10.0** kod üretici ve API'leri, commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` kullanılıyor. 0.9.0 kurulumu ve çıktıları kaldırıldı.
- Verilen XEX ve LIVE/STFS title update: Title ID `434307F0`, Media ID `321F4634`.
- Title update SDK'nın STFS okuyucusuyla açıldı; `default.xexp` çıkarıldı. SDK'nın hash/anahtar kontrolleri korunarak patch uygulandı: `0.0.0.1` → `0.0.1.1`.
- Stok ve TU analizlerindeki birer eksik fonksiyon sınırı gerçek disassembly ile belirlendi. TU manifesti sıfır çözülemeyen çağrıyla kod üretti; `--force` kullanılmadı.
- TU sürümü için 113 C++ translation unit, 30.626 fonksiyon mapping girdisi üretildi; manifestte 63 doğrulanmış fonksiyon sınırı bulunur. Güncel Windows Release executable derlendi.
- Önceki menü düzeltmesinin Linux entegrasyon probunda **47/47 davranış kontrolü geçti**: 10 asin/acos, 5 CRT callback, 24 kayıt initializer, 6 kaynak karşılaştırıcısı ve 2 arayüz getter vakası. Gerçek XEX içindeki 7144 CRT initializer hedefinin tamamı kayıtlıdır. Bu probe tek başına menü/yarış doğrulaması değildir.
- SDK PowerPC altkümesinde **27/27 test, 79 assertion geçti**. Ek chrono testlerinde **12/16 geçti, 4 başarısız**: 1601 epoch'unun Linux nanosecond clock türüne dönüşümünde signed int64 taşması, bağımsız UBSan probuyla doğrulandı. Bu chrono bulgusu düzeltilmedi; SDK sürümü değiştirilmedi. Windows sunum yolu için FSR 1/CAS etkinleştirmesi ve açık video modu öncelik düzeltmesi ayrı, belgelenmiş bir runtime çiftiyle dağıtılır.
- Linux GUI için Xvfb + Mesa lavapipe Vulkan kuruldu. Tam disk verileriyle açılış videosu, başlık ekranı, ilk profil/zorluk ayarları geçildi ve **gerçek ana menüye ulaşıldı**. [Ekran görüntüsü](docs/screenshots/linux-main-menu.png) doğrudan Xvfb'den alındı. ALSA null endpoint ses backend'ini açar; duyulabilir ses kalitesi sınanmadı.
- Windows x64 PE executable Clang 20/MSVC ABI ve resmi Windows SDK ile çapraz derlendi. Güncel Windows PE regresyonunda Wine altında **50/50 kontrol geçti** (önceki 47 vaka ve yarış başlangıcındaki callback için 3 ek vaka); gerçek XEX 0.0.1.1 ve 7144 initializer hedefi doğrulandı. Windows STFS çıkarma aracının çıktısı da beklenen patch SHA-256 değeriyle eşleşti. GUI denemesi bu makinenin ekran/GPU erişimi olmadığı için Direct3D 12 initialization aşamasında durdu. Kullanıcı GTX 1650 SUPER üzerinde Windows ana menüsünü, motorcu/pist seçimini ve Time Trial hazırlık ekranını doğruladı. Start sonrasında bildirilen `0x82271650` kayıt dışı callback güncel pakette düzeltildi; kullanıcı son düzeltmeden sonra oyunun düzgün çalıştığını bildirdi.
- Genişletilmiş Windows launcher ve eşleşen FSR 1/CAS runtime çifti derlendi. **58 launcher + 140 runtime + 50 oyun kodu kontrolü geçti**; gerçek İngilizce/Türkçe GUI sekmeleri, kaydet/yeniden aç ve tuş yakalama Wine/Xvfb altında sınandı. FSR testleri gerçek SDK pass planlamasını doğrular; Windows GPU üzerinde yeni seçeneklerle yarış testi değildir. [Ayarlar, testler ve ekran görüntüleri](docs/launcher.md).


- Oyun içi kayıtlı tuş simgeleri için Xbox/PlayStation/klavye seçimi uygulandı. Güncel Windows PE kontrolleri Wine altında **337/337 geçti** (44 simge çizimi, 45 giriş/eşleme, 58 launcher, 140 runtime ve 50 oyun kodu). Klavye simgeleri gerçek kayıtlı eşlemeleri gösterir. [Kapsam, otomatik algılama ve XInput ile PS denemesi](docs/controller-prompts.md).

- Launcher arayüzü MotoGP kapak paletine uygun koyu kartlar ve turuncu/kırmızı vurgularla yenilendi. Tek iç çözünürlük seçimi ve 25 doğrudan tuş alanı bulunur. Üç sayfanın iki dilde, %75/%100/%125/%150 ölçekte yerleşimi doğrulandı; **72 launcher + 140 runtime kontrolü geçti**. Sayısal başlatma ayarlarının boolean olarak aktarılmasına neden olan TOML dönüşümü de düzeltildi. [Görünüm ve doğrulama](docs/launcher.md).

- Çarpışma kayıtlarının endian dönüşümünde sonlu koordinatları değiştiren bit kaybı giderildi. Gerçek XEX rutinleriyle Linux ve Windows/Wine altında **192/192 veri ve ABI kontrolü geçti**; eski Windows rutini aynı testte hatayı yeniden üretir. Kullanıcı güncel buildle Windows’ta Qatar Time Trial’da 4–5 tur boyunca pistin altına düşmediğini doğruladı; gönderdiği log gerçek çarpışma kayıtlarında onarımın devreye girdiğini gösteriyor. [Düzeltme ve test kapsamı](docs/collision-data.md).

- Kullanıcının PS3 dumpından orijinal buton kaplamaları eklendi: yüz tuşları, L1/R1, L2/R2, SELECT, analog ve D-pad yönleri. Kayıtlı 21 simge, Second Chance panelinin üstündeki 5 tuş ve tam tekrar panelindeki 8 tuş dönüştürülür. v2’de yanlışlıkla değiştirilen alt geri sarma/kamera satırı v3’te korunur. Windows/Wine altında **222 çizim/piksel aktarımı/tekrar paneli kontrolü geçti**; Kullanıcı v2’de menüdeki PS simgelerinin düzeldiğini doğruladı; v3 tekrar panelinin Windows oyun içi testi kullanıcıya bırakıldı. İlk PS3 buildinde parçalanmış simgelere yol açan 256/512 bayt satır aralığı uyumsuzluğu giderildi. `resources/controller/` klasörünü de ZIP’ten çıkar. Ayrı START, PS/Guide ve yazılı L3/R3 kaplaması bulunmadı. Kariyer kaplama düzeltmesi bu buildde bulunmuyor. [Kapsam](docs/controller-prompts.md).

## Hazır Windows paketi

[Güncel Windows x64 EXE + DLL ZIP'i](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-ps3-buttons-v3.zip). GitHub hesabına giriş yaptıktan sonra dosya sayfasındaki **Download raw file** düğmesiyle indir. Paket **ReXGlue v0.10.0** kullanır; Visual Studio/derleme araçları gerekmez.

1. ZIP'i program klasörüne çıkar; yeni EXE/DLL'ler üst dizinde kalır.
2. Tüm orijinal disk dosyalarını, `default.xex`, `default.xexp`, TU paketi, `xenondata/` ve `nxeart/` birlikte **`game/`** altklasörüne koy. [Taşıma rehberi](docs/game-layout.md) örnek düzeni gösterir.
3. `motogp0910-launcher.exe`yi aç, ayarlarını seç ve **PLAY** düğmesine bas. Üstteki **Language / Dil** menüsü Türkçeye geçer; dil seçimi kaydedilir. İstersen oyunu `motogp0910.exe` ile doğrudan da açabilirsin. Klavye varsayılan olarak açıktır: Enter=Start, Space=A, Backspace=B; W/S/A/D=sol stick, Shift+oklar=D-pad.

[Doğrudan EXE açılışı ve giriş tercihi notları](docs/direct-launch.md). Yeni paket `.cmd` içermez; yönergeler `readme.txt` ve `game/readme.txt` adlarıyla gelir. Kullanıcının sağladığı `gamecontrollerdb.txt` dosyası EXE yanında paketlenir; SDL gamepad eşlemeleri için kullanılır. [Controller eşleme notları](docs/controller-mappings.md).

Yeni EXE açık bir oyun yolu verilmediğinde önce `game/default.xex` arar; yoksa eski düz yerleşimi kullanır. Her iki yerde XEX varsa `game/` önceliklidir. İlk çalıştırmada uygulama oyun kökündeki title update'ten `default.xexp` çıkarır; orijinal XEX'i ve mevcut patch'i değiştirmez. Oyun sürümü **0.0.1.1** olmalı. Oyun ikilileri, patch ve disk verileri ZIP'te bulunmaz. Kayıt/cache varsayılanı SDK'nın kullanıcı klasörüdür; varsayılan loglar EXE yanındaki `logs/` içindedir. Windows 10/11 x64 ve uygun Direct3D 12 GPU gerekir.

Linux'ta tam oyun ana menüsü ve en az iki dakikalık çökmesiz gözlem doğrulandı. Güncel Windows PE probunda 50 kontrol geçti. Kullanıcı önceki yarış düzeltmesini ve `game/` düzenini sorunsuz çalıştırdığını bildirdi. Bu teslimattaki giriş tercihi kontrolleri Windows PE probunda Wine altında 5/5 geçti; yeni EXE bu bulut ortamında Windows GPU üzerinde oynanmadı. Tüm mod/pistlerin kalite kontrolü ve duyulabilir ses kalitesi henüz doğrulanmadı. Eski `MotoGP0910-Windows-x64-preview.zip` bu düzeltmeleri içermez.

Paket SHA-256: `f1a67401e6cac1a8918a50bbdb85f988320be2d6233332b8e519653a53eee9f3`.

## Windows'ta kaynaktan derleme

3 GB'lık oyun verisini buluta yüklemek gerekmez. Proje paketini kendi bilgisayarında açıp tam çıkarılmış oyun klasörünü orada kullanabilirsin. Oyun verisi bu proje paketinde bulunmaz.

Gerekli araçlar: Visual Studio **2022 veya daha yeni (2026 dahil)** Desktop development with C++ bileşenleri, Windows SDK, **Clang 20+**, CMake 3.25+ ve Ninja. VS 2026 içindeki Clang 22 ve C++ CMake araçları bu gereksinimleri karşılar.

Güncel betik normal PowerShell'den kurulu VS sürümünün x64 ortamını otomatik hazırlar; VS içindeki Clang, CMake ve Ninja'yı bulur. Uygun harici LLVM/PATH araçlarını da kullanabilir. Kurulum bağlantıları, RAM'e göre Jobs önerileri ve log alma komutu [ayrıntılı rehberde](docs/windows-build.md).

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

GTX 1650 SUPER üzerindeki gerçek Windows denemesi menü ve Time Trial hazırlık ekranını doğruladı. Start sonrasında eksik callback güncel kaynaklarda eklendi; yeni paketle yarış doğrulaması ayrıca yapılmalıdır.

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

Çıktı `out/build/windows-cross-release/motogp0910.exe`; paket `out/dist/MotoGP0910-Windows-x64-collision-data-fix.zip` olur. Wine yalnızca ek yükleme/CPU probu içindir; gerçek Windows grafik doğrulamasının yerine geçmez.
