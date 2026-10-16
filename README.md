# MotoGP 09/10 — ReXGlue Nightly 0.10.0.24-dev.gbd833a2

Xbox 360 sürümünün native PC yeniden derleme projesi. Ana hedef verilen stok XEX + title update ile **0.0.1.1** oyun sürümüdür. Windows x64 executable derlendi; **Linux üzerinde tam oyun verileriyle ana menüye ulaşma doğrulandı**. [Güncel Windows EXE + DLL paketi hazır](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-community-online.zip). Hedef kullanıcı sistemi Windows 10/11; bu bulut makinesi Linux'tur.

[Türkçe oyun metinleri](docs/turkish-localization.md) eklendi: **General → Game language → Türkçe**. Menü, kariyer, eğitim, yarış mesajları, teknik açıklamalar ve 50 başarım çevrildi; sesler korunur. Orijinal yazı tiplerine Ğ/ğ, İ/ı ve Ş/ş eklendi. İşlevsiz **Xbox LIVE Marketplace** seçeneği Seçenekler menüsünden kaldırıldı. Yeni çevirinin Windows GPU üzerinde görsel kontrolü bekleniyor.

[Kontrol ekranı](docs/controller-prompts.md) artık orijinal PS3 DualShock 3 görselini veya koyu/turuncu canlı klavye listesini gösterir. Default / Alternative A / B açıklamaları seçili düzene göre değişir. Başarım kaydırma çubuğu gerçek fare sürüklemesiyle doğrulandı; launcher üst çubuğunda yalnızca küçültme/kapatma bulunur. QUIT GAME beyaz vurgusu ve Press Start çıkışı kullanıcı tarafından onaylandı.

[F1 oyun içi ayar menüsü](docs/in-game-settings.md) eklendi: başarım ekranıyla aynı koyu/turuncu tasarım, yalnızca anında uygulanan ses/grafik/efekt/simge/tuş atama seçenekleri. **Kaydet** launcher ayarlarını da günceller; yeniden başlatma gerektiren seçenekler menüde yok. **337 hedefli Windows/Wine kontrolü** geçti; native Windows görsel/oyun içi test bekleniyor.

[Ana menü QUIT GAME ve Press Start çıkış onayı](docs/quit-menu.md): QUIT GAME vurgusu beyaz; Press Start kontrolü gerçek açılış durumunu (`828DA060`) okuyacak şekilde düzeltildi. **98 hedefli Windows/Wine kontrolü geçti**. Native Windows görünüm/çıkış testi bekleniyor.

[Launcher başarımlar sayfası, F7 menüsü ve yumuşak ses](docs/achievements.md): **50 orijinal simge / 1000 G**, kazanılan/kilitli listesi, arama, ilerleme çubuğu ve ortak bildirim/ses tercihleri. Mevcut kayıtlar yalnızca okunur; oyun açıkken güncellenir. **286 runtime ve 59.199 launcher UI kontrolü geçti**. [Güncel proje durumu ve sıradaki işler](docs/project-status.md). [Mevcut çalışmadan en yüksek getiriyi çıkaracak görevler](docs/project-priorities.md).

[Klavye simgeleri yenilendi](docs/keyboard-prompts.md): koyu tuşlar, turuncu MotoGP detayları ve Open Sans yazı; gerçek tuş eşlemeleri korunur. Güncel arayüz talepleri sırayla, ayrı teslimatlar halinde ilerler.

[Geliştirme notları ve yol haritası](docs/roadmap.md): [game/ dosya düzeni tamamlandı](docs/game-layout.md); [Yenilenen İngilizce/Türkçe launcher; 720p/1440p/4K, FSR 1.0, ses ve sade tuş eşlemeleri](docs/launcher.md). [Stabilite düzeltmeleri ve Windows regresyon sonuçları](docs/stability-quality-control.md) mevcut; [Radmin VPN/LAN kaynak araştırması ve rota](docs/lan-research.md) hazır. [İlk LAN keşif/guest oturum altyapısı](docs/lan-lobby-preview.md) iki ayrı Windows süreciyle doğrulandı. [Yeni deneysel LAN entegrasyonu ve iki-PC kullanım adımları](docs/lan-multiplayer-test.md) hazır; **kullanıcı iki oyunculu Radmin yarışının sorunsuz çalıştığını bildirdi**. İlk lobi girişlerinde görülen iki hata [lobi düzeltmesinde](docs/lan-lobby-stability.md) giderildi; yeni EXE için Radmin testi bekleniyor. [Boş IP ile launcher başlangıç çökmesi düzeltildi](docs/lan-startup-fix.md); 17 hedefli Windows/Wine başlangıç kontrolü geçti. [Otomatik çökme raporları](docs/crash-reports.md) ve isteğe bağlı launcher izleyicisi eklendi. Kullanıcı son yarış başlangıcı düzeltmesinden sonra oyunun düzgün çalıştığını bildirdi; tüm mod/pistlerin kalite kontrolü henüz tamamlanmadı.

[Deneysel LAN Windows build'i](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-community-online.zip): oyun EXE'si kullanıcıdan gelen MotoGP 09/10 kapağını, launcher ayrı MotoGP logosunu kullanır. Çevrimdışı oyun varsayılan kalır; Launcher her açılışta General sayfasında başlar. Temiz paket oyun verisi, kayıt, kişisel ayar ve log içermez. İsteğe bağlı [tek PC bağlantı testi](docs/lan-one-pc-test.md) betiği kaynak depoda bulunur. General sayfasındaki LAN seçeneği otomatik oda aramasını ve isteğe bağlı IP yedeğini açar. [Kapsam, sınırlamalar ve kullanım](docs/lan-multiplayer-test.md).

**Güncel kullanım: [güncel kaynak depo](https://github.com/inflamesgo/motogp0910-recomp/tree/main) ve [Windows yerel derleme rehberi](docs/windows-build.md).** Güncel paket önceki başlangıç düzeltmelerini ve yarışa geçişteki `0x82271650` callback kaydını içerir. Manifest, eksik CRT callback/initializer, kaynak karşılaştırıcısı ve video/arayüz fonksiyonlarını gerçek komut sınırlarıyla üretime dahil eder; oyunun rutinleri stub ile değiştirilmez. [Gerçek Linux açılış görüntüleri](docs/screenshots/) ve [Linux grafik denemesini yeniden çalıştırma adımları](docs/linux-startup.md) depoda bulunur.


## Doğrulananlar

- ReXGlue **Nightly 0.10.0.24-dev.gbd833a2**, commit `bd833a2ab69d305f99ddf57418fc38ffec8af93d` kullanılıyor. Sürüm ve resmi paket hashleri `sdk-version.json` ile sabitlenmiştir. 0.9.0 kurulumu ve çıktıları kaldırıldı.
- Verilen XEX ve LIVE/STFS title update: Title ID `434307F0`, Media ID `321F1634`.
- Title update SDK'nın STFS okuyucusuyla açıldı; `default.xexp` çıkarıldı. SDK'nın hash/anahtar kontrolleri korunarak patch uygulandı: `0.0.0.1` → `0.0.1.1`.
- Stok ve TU analizlerindeki birer eksik fonksiyon sınırı gerçek disassembly ile belirlendi. TU manifesti sıfır çözülemeyen çağrıyla kod üretti; `--force` kullanılmadı.
- TU sürümü için 113 C++ translation unit, 30.626 fonksiyon mapping girdisi üretildi; manifestte 63 doğrulanmış fonksiyon sınırı bulunur. Güncel Windows Release executable derlendi.
- Önceki menü düzeltmesinin Linux entegrasyon probunda **47/47 davranış kontrolü geçti**: 10 asin/acos, 5 CRT callback, 24 kayıt initializer, 6 kaynak karşılaştırıcısı ve 2 arayüz getter vakası. Gerçek XEX içindeki 7144 CRT initializer hedefinin tamamı kayıtlıdır. Bu probe tek başına menü/yarış doğrulaması değildir.
- SDK PowerPC + tarih testlerinde **43/43 vaka, 170 assertion geçti**. Eski chrono 1601-epoch taşması `sdk-stability.patch` ile düzeltildi; 1601–9999 tarih aralığı ve 100 ns hassasiyet için ayrıca **19 kontrol UBSan altında geçti**. Windows sunum yolu ve ek düzeltmeler checksum ile sabitlenmiş özel runtime çiftiyle dağıtılır.
- Güncel stabilite taraması: TLS sınır/taşma koruması, thread kimliği yarış koşulu, başlatma yolu metin ömrü ve işlemci önbelleği düzeltildi. Windows/Wine altında **168 gerçek runtime kontrolü**, 8.000 eşzamanlı kimlik ve launcher/giriş/fizik/FSR planlayıcı testleri geçti. [Kapsam ve tekrar çalıştırma](docs/stability-quality-control.md).

- Linux GUI için Xvfb + Mesa lavapipe Vulkan kuruldu. Tam disk verileriyle açılış videosu, başlık ekranı, ilk profil/zorluk ayarları geçildi ve **gerçek ana menüye ulaşıldı**. [Ekran görüntüsü](docs/screenshots/linux-main-menu.png) doğrudan Xvfb'den alındı. ALSA null endpoint ses backend'ini açar; duyulabilir ses kalitesi sınanmadı.
- Windows x64 PE executable Clang 20/MSVC ABI ve resmi Windows SDK ile çapraz derlendi. Güncel Windows PE regresyonunda Wine altında **50/50 kontrol geçti** (önceki 47 vaka ve yarış başlangıcındaki callback için 3 ek vaka); gerçek XEX 0.0.1.1 ve 7144 initializer hedefi doğrulandı. Windows STFS çıkarma aracının çıktısı da beklenen patch SHA-256 değeriyle eşleşti. GUI denemesi bu makinenin ekran/GPU erişimi olmadığı için Direct3D 12 initialization aşamasında durdu. Kullanıcı GTX 1650 SUPER üzerinde Windows ana menüsünü, motorcu/pist seçimini ve Time Trial hazırlık ekranını doğruladı. Start sonrasında bildirilen `0x82271650` kayıt dışı callback güncel pakette düzeltildi; kullanıcı son düzeltmeden sonra oyunun düzgün çalıştığını bildirdi.
- Genişletilmiş Windows launcher ve eşleşen FSR 1/CAS runtime çifti derlendi. **58 launcher + 140 runtime + 50 oyun kodu kontrolü geçti**; gerçek İngilizce/Türkçe GUI sekmeleri, kaydet/yeniden aç ve tuş yakalama Wine/Xvfb altında sınandı. FSR testleri gerçek SDK pass planlamasını doğrular; Windows GPU üzerinde yeni seçeneklerle yarış testi değildir. [Ayarlar, testler ve ekran görüntüleri](docs/launcher.md).


- Oyun içi kayıtlı tuş simgeleri için Xbox/PlayStation/klavye seçimi uygulandı. Güncel Windows PE kontrolleri Wine altında **337/337 geçti** (44 simge çizimi, 45 giriş/eşleme, 58 launcher, 140 runtime ve 50 oyun kodu). Klavye simgeleri gerçek kayıtlı eşlemeleri gösterir. [Kapsam, otomatik algılama ve XInput ile PS denemesi](docs/controller-prompts.md).

- Launcher arayüzü MotoGP kapak paletine uygun koyu kartlar ve turuncu/kırmızı vurgularla yenilendi. Tek iç çözünürlük seçimi ve 25 doğrudan tuş alanı bulunur. Üç sayfanın iki dilde, %75/%100/%125/%150 ölçekte yerleşimi doğrulandı; **72 launcher + 140 runtime kontrolü geçti**. Sayısal başlatma ayarlarının boolean olarak aktarılmasına neden olan TOML dönüşümü de düzeltildi. [Görünüm ve doğrulama](docs/launcher.md).

- Çarpışma kayıtlarının endian dönüşümünde sonlu koordinatları değiştiren bit kaybı giderildi. Gerçek XEX rutinleriyle Linux ve Windows/Wine altında **192/192 veri ve ABI kontrolü geçti**; eski Windows rutini aynı testte hatayı yeniden üretir. Kullanıcı güncel buildle Windows’ta Qatar Time Trial’da 4–5 tur boyunca pistin altına düşmediğini doğruladı; gönderdiği log gerçek çarpışma kayıtlarında onarımın devreye girdiğini gösteriyor. [Düzeltme ve test kapsamı](docs/collision-data.md).

- Kullanıcının PS3 dumpından orijinal buton kaplamaları eklendi: yüz tuşları, L1/R1, L2/R2, SELECT, analog ve D-pad yönleri. Kayıtlı 21 simge, Second Chance panelinin üstündeki 5 tuş ve tam tekrar panelindeki 8 tuş dönüştürülür. v2’de yanlışlıkla değiştirilen alt geri sarma/kamera satırı v3’te korunur. Windows/Wine altında **222 çizim/piksel aktarımı/tekrar paneli kontrolü geçti**; Kullanıcı v2’de menüdeki PS simgelerinin düzeldiğini doğruladı; v3 tekrar panelinin Windows oyun içi testi kullanıcıya bırakıldı. İlk PS3 buildinde parçalanmış simgelere yol açan 256/512 bayt satır aralığı uyumsuzluğu giderildi. `resources/controller/` klasörünü de ZIP’ten çıkar. Ayrı START, PS/Guide ve yazılı L3/R3 kaplaması bulunmadı. Kariyer kaplama readback düzeltmesi yeni pakete eklendi; Windows yarış görüntüsü kullanıcı tarafından sınanacak. [Kapsam](docs/controller-prompts.md).

- Kariyer modunun özel motorcu/motor kaplamalarını oluştururken GPU sonucunu CPU belleğine geri yazan kapsamlı readback yolu etkinleştirildi. **13 kapsam/ABI (16 guest çağrısı), 72 launcher ve 140 runtime kontrolü geçti**; güncel çiftle PS3 simge/giriş/çarpışma/başlangıç kontrolleri de geçti. Oyun içi kariyer denemesi kullanıcının isteğiyle yerel Windows cihazına bırakıldı. Launcher sol altındaki SDK/platform yazısı ve şeritleri örten kutu kaldırıldı. [Düzeltmenin kapsamı](docs/career-textures.md).

- DualSense / Edge adaptif tetikler **deneysel** olarak eklendi: Launcher → Controls ve F1 → Controls içinde sağ/sol mod, başlangıç, bitiş ve güç ayrı ayarlanır. Varsayılan kapalı; yerel SDL gerekir. Downpour’un SDL yaklaşımı kaynaklardan incelendi. 8BitDo/DS3/DS4’e efekt gönderilmez; fiziksel DualSense direnç testi henüz yapılmadı. [Kaynaklar ve kapsam](docs/adaptive-triggers.md).
- Kullanıcı Press Start’tan geri basınca çıkış onayının artık çalıştığını doğruladı. QUIT GAME’in üstte kalan mavi parçası da katman parlaklığı ve alfa korunarak beyaz/griye çevrildi; üst katmanın görünümü de kullanıcı tarafından onaylandı.

## Topluluk sunucusu: Offline / LAN / Online

General menüsünde **Connection mode** ile Çevrimdışı, LAN/Radmin veya deneysel Çevrimiçi seçilebilir. [Linux ve Windows VDS sunucu paketi](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Community-Server.zip) oda keşfi, doğrulanmış UDP aktarımı ve 378 yerel leaderboard görünümünde kalıcı sonuçları sağlar. [Kurulum ve kapsam](docs/community-online.md). Online için kendi HTTPS sunucu adresin gerekir; henüz ortak bir sunucu kurulmadı. Yarış simülasyonu oyunculardan birinde çalışır; **Player Match** kullanılır. Resmî Xbox Live ranked/arkadaş/ses hizmetleri geri getirilmez. Mevcut LAN seçeneği korunur. Yerel Windows/Wine kontrolleri geçti; gerçek VDS üzerinden yarış ve canlı leaderboard ekranı testi bekleniyor.

## Hazır Windows paketi

[Güncel Windows x64 EXE + DLL ZIP'i](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-community-online.zip). GitHub hesabına giriş yaptıktan sonra dosya sayfasındaki **Download raw file** düğmesiyle indir. Paket **ReXGlue Nightly 0.10.0.24-dev.gbd833a2** kullanır; Visual Studio/derleme araçları gerekmez.

1. ZIP'i program klasörüne çıkar; yeni EXE/DLL'ler üst dizinde kalır.
2. Tüm orijinal disk dosyalarını, `default.xex`, `default.xexp`, TU paketi, `xenondata/` ve `nxeart/` birlikte **`game/`** altklasörüne koy. [Taşıma rehberi](docs/game-layout.md) örnek düzeni gösterir.
3. `motogp0910-launcher.exe`yi aç, ayarlarını seç ve **PLAY** düğmesine bas. **General → Launcher language** menüsü Türkçeye geçer; dil seçimi kaydedilir. **Game language → Türkçe** ise oyunun yazılarını Türkçeye geçirir; iki dil seçimi bağımsızdır. İstersen oyunu `motogp0910.exe` ile doğrudan da açabilirsin. Klavye varsayılan olarak açıktır: Enter=Start, Space=A, Backspace=B; W/S/A/D=sol stick, Shift+oklar=D-pad.

[Doğrudan EXE açılışı ve giriş tercihi notları](docs/direct-launch.md). Yeni paket `.cmd` içermez; yönergeler `readme.txt` ve `game/readme.txt` adlarıyla gelir. Kullanıcının sağladığı `gamecontrollerdb.txt` dosyası EXE yanında paketlenir; SDL gamepad eşlemeleri için kullanılır. [Controller eşleme notları](docs/controller-mappings.md).

Yeni EXE açık bir oyun yolu verilmediğinde önce `game/default.xex` arar; yoksa eski düz yerleşimi kullanır. Her iki yerde XEX varsa `game/` önceliklidir. İlk çalıştırmada uygulama oyun kökündeki title update'ten `default.xexp` çıkarır; orijinal XEX'i ve mevcut patch'i değiştirmez. Oyun sürümü **0.0.1.1** olmalı. Oyun ikilileri, patch ve disk verileri ZIP'te bulunmaz. Kayıt/cache varsayılanı SDK'nın kullanıcı klasörüdür; varsayılan loglar EXE yanındaki `logs/` içindedir. Windows 10/11 x64 ve uygun Direct3D 12 GPU gerekir.

Linux'ta tam oyun ana menüsü ve en az iki dakikalık çökmesiz gözlem doğrulandı. Güncel Windows PE probunda 50 kontrol geçti. Kullanıcı önceki yarış düzeltmesini ve `game/` düzenini sorunsuz çalıştırdığını bildirdi. Bu teslimattaki giriş tercihi kontrolleri Windows PE probunda Wine altında 5/5 geçti; yeni EXE bu bulut ortamında Windows GPU üzerinde oynanmadı. Tüm mod/pistlerin kalite kontrolü ve duyulabilir ses kalitesi henüz doğrulanmadı. Eski `MotoGP0910-Windows-x64-preview.zip` bu düzeltmeleri içermez.

Paket SHA-256: `26b8c11b5258179a2823e871d2f26742f562f8a0f1148fb2084e35653eb8d281`.

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

Betik `sdk-version.json` ile sabitlenmiş resmi Windows AMD64 SDK paketini indirip GitHub yayınındaki SHA-256 ile doğrular. Verilen XEX/TU hash'lerini kontrol eder; STFS çıkarma aracını derler, patch'i çıkarır, yalnızca XEX/patch'i yerel `assets/game-tu` altına hazırlar. Tam oyun klasörünü kopyalamaz. Kod üretimini, Release derlemesini ve headless entegrasyon probunu çalıştırır. Native komutların hataları saklanmaz.

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
# SDK kaynak düzeltmeleri uygulandıktan sonra 43 vaka + 19 aralık kontrolü geçer.
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
| Linux AMD64 SDK ZIP | `0a56565221a4e75f91fc71f787d9299a0d0298127b970c8a88a292f8d3a3befe` |
| Windows AMD64 SDK ZIP | `1e19d7ec9be0d9d7e1a0088a34a33b068d4dea520181e81318528ef3b37627e7` |

## Dosya düzeni

`assets/game/default.xex` stok kopyadır. `assets/game-tu/` analiz/derleme için orijinal XEX + kardeş `default.xexp` içerir. `assets/updates/` orijinal title update ve çıkarılan patch'i tutar. `generated/default/` oyun kodudur. Bu dosyalar ve `out/` Git'e eklenmez. `generated/rexglue.cmake` SDK tarafından yönetilir.

`tools/motogp_inspect` SDK loader'larıyla XEX section dump ve STFS çıkarma sağlar; var olan çıktı dizinini/dosyaları değiştirmeyi reddeder. Analiz notları `docs/analysis.md`, bu ortamın ham raporları `out/analysis/` altındadır.

Resmi SDK: https://github.com/rexglue/rexglue-sdk/tree/bd833a2ab69d305f99ddf57418fc38ffec8af93d

## Linux bulutundan Windows derlemesi

```bash
cd /workspace/motogp0910-recomp
bash scripts/install-windows-cross.sh
bash scripts/build-windows-cross.sh
python3 scripts/package-windows.py
```

Microsoft SDK/CRT xwin 0.10.0 ile sabit VS17 manifestinden hazırlanır. Windows kaynakları Clang 20 ile MSVC ABI'sine derlenir; kod üretimi sdk-version.json ile sabitlenmiş aynı Nightly Linux host aracında yapılır. Compiler resource include yolu Microsoft include yollarından önce gelmelidir; aksi halde MMX ve constexpr offsetof hataları çıkar. SDK/library doğrulamaları ve static assertion'lar korunur. SDK paketleri checksum, LLVM paketleri APT imza kontrolünden geçer. Önbellek ve araçlar `/workspace/shared` altındadır.

Çıktı `out/build/windows-nightly-release/motogp0910.exe`; paket `out/dist/MotoGP0910-Windows-x64-stability-update.zip` olur. Wine yalnızca ek yükleme/CPU probu içindir; gerçek Windows grafik doğrulamasının yerine geçmez.

Experimental FSR 2/3 and horizontal-plus 21:9/32:9 camera options: [sources, implementation and limitations](docs/experimental-rendering.md). FSR quality modes do not lower scene rendering cost; correct temporal motion/depth integration remains unfinished.

Nightly migration and validation: [migration notes](docs/sdk-nightly-migration.md). [Stable rollback package](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-fsr-ultrawide-stable.zip) remains available.
