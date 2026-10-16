# MotoGP 09/10 — Claude, web sitesi ve VPS devir belgesi

Bu belge mevcut projeyi başka bir geliştirme ortamına/Claude'a devretmek içindir. Oyun paketinin README'si değildir ve build ZIP'lerine konulmaz.

**Hazırlanma:** 16 Ekim 2026. Uygulama kodu için başlangıç noktası: `83fd30bc6c3b4647cafd3c013bb5a0dd6a3c1031`. Bu belgenin eklendiği değişiklik ayrıca paketleme politikasını günceller; oyun motorunu değiştirmez. Sonraki geliştirici önce `git status` ve `git log` ile daha yeni değişiklikleri kontrol etmelidir.

## İçindekiler

1. Projenin amacı ve kesin kararlar
2. Mevcut durum ve doğrulama sınırları
3. Depo, klasörler ve kaynak dosyası haritası
4. Sürüm ve runtime bağımlılıkları
5. Yeni ortama taşıma
6. Windows derleme, çalıştırma ve paketleme
7. Ayarlar, kayıtlar, kimlik ve loglar
8. Linux/Windows VPS kurulumu
9. Web sitesi ile entegrasyon planı
10. Testler ve teşhis
11. Açık işler ve devralma sırası
12. Kaynak belge dizini

## 1. Projenin amacı ve kesin kararlar

Xbox 360 MotoGP 09/10 oyununun ReXGlue ile native PC yeniden derlemesidir. Hedef oyun title update uygulanmış **0.0.1.1**, Title ID **434307F0**. Ana kullanıcı platformu **Windows 10/11 x64 / Direct3D 12**. Linux mevcut bulut derleme, araç ve sunucu ortamıdır; Linux grafik uyumluluğuna Windows'tan fazla zaman ayrılmamalıdır.

Kullanıcının korunmasını istediği kararlar:

- **Offline, LAN/Radmin ve Online ayrı seçenekler olarak kalacak.** Varsayılan Offline.
- LAN otomatik oda keşfi öncelikli; elle host IPv4 girme alternatifi korunacak.
- Bağlantı kalitesi, kolay kullanım ve yeniden bağlanma öncelikli. Güvenlik değişiklikleri normal Windows kullanıcılarını, aynı modem arkasındaki oyuncuları veya tekrar bağlantıyı gereksiz engellememeli. TLS doğrulamasını kapatmak çözüm değildir.
- SDK sürümü aşağıda belirtilen nightly'ye sabit. Kendiliğinden başka nightly/stable sürüme geçilmemeli.
- Kullanıcı FPS'e bağlı fizik şüphesini geri çekti. **Fizik/timestep/FPS davranışına bu gerekçeyle müdahale etmeyin.** `physics_assets.cpp` içindeki gerçek veri/endian düzeltmesi farklı bir iştir ve korunmalıdır.
- FSR 2/3 seçenekleri deneysel etiketiyle kalabilir. Frame generation istenmiyor; bu çalışma iptal edildi.
- Oyun içi **F1** menüsünde yalnızca anında uygulanan ayarlar bulunacak. Sonraki açılışta etkili olan bağlantı/dil/cihaz ayarlarını buraya eklemeyin. Başarımlar **F7**.
- Launcher açılış sayfası General. Launcher language, Game language, button prompt seçimi ve Game folder burada. Başka sayfalara ortak Game folder kutusu geri getirilmemeli.
- Launcher büyütme düğmesi kaldırıldı. Başarım listesi gerçek fareyle kaydırılabilir olmalı.
- QUIT GAME tamamen büyük harf ve beyaz/gri; katmanların saydamlık/parlaklık geçişleri korunur. Press Start ekranında B/Backspace çıkış onayı çalışır durumda korunmalı.
- Türkçe kapsamı bütün yazılar/altyazılar; mevcut sesler korunur. Launcher dili ile oyun dili bağımsız.
- DualSense desteği diğer kullanıcılar içindir. Proje sahibinin 8BitDo Ultimate koluyla adaptif tetik direnci donanımsal olarak test edilemez.
- Hesap/kullanıcı adı/şifre/kurtarma sistemi şimdilik **tasarım aşamasında**. Uygulamaya geçirildiği varsayılmamalı.
- **Yeni geliştirme build'lerine README ve özellik/kurulum açıklama dosyaları eklenmeyecek.** Son sürümün README'sini kullanıcı hazırlayacak. Lisanslar, gerekli runtime kaynakları ve checksum dosyaları korunacak. Bu devir belgesi depo içinde ayrı kalacak.

## 2. Mevcut durum ve doğrulama sınırları

| Alan | Mevcut uygulama | Kanıt / henüz eksik olan |
|---|---|---|
| Temel oyun | Windows EXE, launcher, crash monitor; SDK nightly geçişi | Windows PE/Wine hedefli testler; Linux tam veriyle menü açılışı. Bütün pist/oyun modları için Windows GPU test matrisi yok |
| LAN/Radmin | Yerel XNet/session/socket köprüleri; keşif ve doğrudan IP | Kullanıcı iki PC arasında Radmin üzerinden yarışın çalıştığını bildirdi |
| Lobi tekrar bağlantısı | Arama sonucunu koruma ve join-only leave düzeltmeleri | Hedefli regresyonlar geçti; önceki “lobi çökmesi” süreç kapanması değil, bağlantı kesilip menüye dönüş idi |
| Community Online | HTTPS kontrol, doğrulanmış UDP relay, SQLite leaderboard | Yerel Linux/Windows Python sunucusu ve Windows PE istemcisi testleri var. Gerçek genel erişimli VPS yarış testi yok |
| Türkçe | 2.102 metin ID'si, Lua gösterim alanları, dört font, Türkçe başarımlar | Son düzeltme çalışan oyunda 2.102 metin eşleşmesi ve Linux menü görüntüsüyle doğrulandı. Kullanıcı son Windows paketini henüz doğrulamadı |
| Kontrol/UI | PS3 simgeleri/DS3 görseli, dinamik klavye listesi, başarımlar, F1, çıkış | Birçok hedefli Windows/Wine testi; Press Start çıkışı ve beyaz QUIT GAME kullanıcı tarafından onaylandı |
| FSR 2/3 | FidelityFX sağlayıcıları, güvenli hata dönüşü | Deneysel uyumluluk yolu. Gerçek hareket vektörü/derinlik/jitter ve tam temporal birleştirme yok; Windows GPU dispatch görsel testi eksik |
| Ultrawide | Yatay görüş alanını genişleten projeksiyon ve HUD düzeltmesi | Projeksiyon testleri ve Linux 21:9 menü açılışı var. Her yarış kamerası/sahne için native Windows testi yok |
| DualSense | SDL üzerinden ayrı L2/R2 efekt ayarları | Varsayılan kapalı; yazılım taşıma testleri var, fiziksel direnç hissi test edilmedi |
| Web sitesi | **Henüz yok** | Frontend, herkese açık JSON leaderboard API'si, hesap paneli veya web deployment hazırlanmış değil |

Test sayıları gerçek yarış oturumu sayısı değildir. Wine testi native Windows GPU/TLS doğrulamasının yerine geçmez. Ayrıntılı tarihsel sonuçlarda ilgili özellik belgesini esas alın.

### Son Türkçe düzeltmesinin nedeni

Önceki sürüm çeviri dosyalarını VFS içinde görünür kılsa da oyunun gerçek `OpenFile → parent.GetChild` yolu bunları atlıyordu. Bu yüzden yalnızca bizim eklediğimiz çıkış satırı çevrilmiş görünebiliyordu. `src/localization.cpp` artık birleşik, salt okunur Entry/File ağacıyla gerçek dosya açma yolunu kapsar. Küçük fonttaki İ/ı atlas eşik sorunu da giderildi.

Çeviriyi yeniden tasarlamadan önce **son paketin bütün dosyalarını** kullanıp General → Game language → Türkçe seçildiğini ve logları kontrol edin. Yalnızca launcher'ı Türkçeye almak oyun dilini değiştirmez. Eski `turkish-language` adlı ZIP son VFS düzeltmesini içermeyebilir.

### Mevcut yayımlanmış paketler

| Dosya | SHA-256 |
|---|---|
| `downloads/MotoGP0910-Windows-x64-community-online.zip` | `d7de8a7ca1abda4b8245d096f56ceef40d42dc9753dd3aa5a20c78fb472724f8` |
| `downloads/MotoGP0910-Community-Server.zip` | `3303bd429dd2d61d4c458833420c39d24a118c280ac7ef3badaf4eececc0b9fd` |

Yanlarında `.zip.sha256` dosyaları vardır. Bunlar belge hazırlanırken mevcut olan arşivlerdir; sonraki paketlerde hash doğal olarak değişir. **Bu mevcut arşivler eski politikadan kalan README'leri içerir.** Yeni varsayılan paketleme politikası gelecekte üretilecek paketler içindir; eski ZIP'ler bu belge işi sırasında yeniden yayımlanmadı.

## 3. Depo, klasörler ve kaynak dosyası haritası

Depo: <https://github.com/inflamesgo/motogp0910-recomp>

Mevcut bulut checkout'u: `/workspace/motogp0910-recomp`. Bu yol başka bilgisayarda zorunlu değildir. Aşağıdaki göreli yollar depo köküne göredir.

```text
motogp0910-recomp/
├── CMakeLists.txt                  C++ hedefleri, runtime/patch entegrasyonu
├── CMakePresets.json               Native platform derleme preset'leri
├── sdk-version.json               Gerçek SDK sürümü, commit ve paket hash'leri
├── motogp0910_manifest.toml        Oyun/codegen manifesti, elle keşfedilen fonksiyonlar
├── CLAUDE_HANDOFF.md               Bu devir belgesi; build'e girmez
├── src/                           Uygulama, launcher, oyun hook'ları, ağ
├── resources/                     Kontrol görselleri, ikonlar, Türkçe overlay
├── server/                        Bağımsız Python community servisi
├── scripts/                       Kurulum, derleme, paketleme, doğrulama
├── tools/                         C++ test/probe hedefleri ve asset araçları
├── cmake/                         SDK patch'leri ve bridge ekleme kuralları
├── third_party/                   Sabitlenmiş runtime, font/SDL/FSR bağımlılıkları
├── docs/                          Özellik belgeleri, araştırma ve test kayıtları
├── downloads/                     Paylaşılan sürüm arşivleri
├── generated/rexglue.cmake         Takip edilen codegen entegrasyonu
├── generated/default/             Yerelde üretilen oyun C++; Git dışında
├── assets/                        Özel oyun dosyaları; Git dışında
└── out/                           Araçlar, build, test ve staging; Git dışında
```

### Uygulama ve launcher

| Dosya / grup | Sorumluluk |
|---|---|
| `src/main.cpp`, `src/motogp0910_app.h` | Giriş, ReXGlue uygulama yaşam döngüsü, hook/resource kurulumları, overlay ve kapanış |
| `src/launcher/main.cpp` | Native Win32 launcher penceresi, sayfalar ve olaylar |
| `src/launcher/core.h` | Ayar yükleme/kaydetme, doğrulama, CLI üretimi, oyun yolu ve süreç açma |
| `src/launcher/{display_modes,hardware,render_presets,engine_options,keyboard_bindings}.h` | Görüntü modu/cihaz, grafik ve tuş seçenekleri |
| `src/launcher/{achievements.cpp,achievements.h,achievements_page.inc}` | Launcher başarımlar sayfası; mevcut kayıtlardan okuma |
| `src/launcher/adaptive_triggers_dialog.h`, `caption.inc`, `ui_ids.h` | Adaptif tetik penceresi, başlık çubuğu ve kontrol kimlikleri |
| `src/{in_game_settings.cpp,in_game_settings_input.cpp,overlay_theme.h}` | F1 canlı ayar ekranı ve ortak görünüm |
| `src/{achievement_ui.cpp,achievement_sound.cpp,achievement_preferences.h}` | F7 başarımlar, bildirim/ses tercihleri |
| `src/{quit_menu.cpp,quit_accent.inc,options_menu.cpp}` | Çıkış satırı/onayı, beyaz vurgu ve Marketplace kaldırma |
| `src/{crash_report.cpp,crash_monitor.cpp,window_icons.h}` | Windows hata raporu, ayrı izleyici ve pencere ikonları |

### Render, giriş ve yerelleştirme

| Dosya / grup | Sorumluluk |
|---|---|
| `src/{display_settings.cpp,video_settings.h,frame_pacing.cpp}` | Görüntü/FPS/VSync davranışı |
| `src/{engine_settings.cpp,engine_settings_options.h,audio_settings.h}` | Gerçek oyun efektleri ve ses ayarları |
| `src/fsr_provider_bridge.cpp` | Deneysel FidelityFX sağlayıcı köprüsü |
| `src/{ultrawide.cpp,presentation_aspect_bridge.cpp}` | Horizontal-plus kamera projeksiyonu ve sunum oranı |
| `src/{livery_readback.cpp,livery_readback_bridge.cpp}` | Kariyerde sürücü/motor kaplama readback düzeltmesi |
| `src/physics_assets.cpp` | Serileştirilmiş collision verisi float/endian düzeltmesi; FPS fizik deneyi değildir |
| `src/button_prompt*.{cpp,h}` | Xbox/PlayStation/klavye simge seçimi, eşleme ve geometri |
| `src/{ps3_prompt_textures.cpp,control_diagram.cpp}` | PS3 texture bankası, DS3 diyagramı ve canlı klavye listesi |
| `src/{keyboard_binding_specs.h,keyboard_font_data.h,input_defaults.h}` | Gerçek tuş atamaları, gömülü tuş fontu ve varsayılanlar |
| `src/{shared_sdl_bridge.cpp,adaptive_triggers.cpp,adaptive_trigger_config.h}` | SDL erişimi ve DualSense efektleri |
| `src/{game_language.cpp,localization.cpp,localization.h}` | Oyun dili ve salt okunur Türkçe VFS overlay |
| `src/turkish_achievements.h` | Türkçe başarım ad/açıklamaları |

### Ağ

| Dosya / grup | Sorumluluk |
|---|---|
| `src/lan/protocol.{cpp,h}`, `node.{cpp,h}`, `metadata.{cpp,h}` | LAN02 paketleri, keşif/oda düğümü ve özgün oda metadatası |
| `src/lan/{guest_bridge,socket_bridge,runtime}.{cpp,h}` | Guest XNet/XSession/socket ABI'si ve mod yönlendirme |
| `src/online/client.{cpp,h}` | WinHTTP HTTPS, DPAPI profil kimliği, kontrol API'si |
| `src/online/relay.{cpp,h}` | MGO2 UDP envelope, HMAC, adres doğrulama ve aktarım |
| `src/online/stats.{cpp,h}` | Orijinal leaderboard/stat ABI'sini community servisine taşıma |
| `server/community_server.py` | Async HTTP/UDP servis, oda/üyelik ve SQLite kayıtları |
| `server/security.py` | Sınırlar, HTTP ayrıştırma ve güvenlik yardımcıları |
| `server/leaderboards.json` | 378 orijinal leaderboard görünümünün şeması |
| `server/proxy_config.py`, `Caddyfile.example` | Özel proxy anahtarı ve TLS reverse proxy başlatma |
| `server/start-server.{sh,ps1}`, `*.service.example` | Linux/Windows başlangıç ve örnek Linux servis tanımları |
| `server/test_*.py` | Stdlib sunucu regresyonları |

### Asset ve üçüncü taraf kaynakları

- `resources/controller/ps3-prompts.bin` ve `.json`: kullanıcının kendi PS3 dump'ından çıkarılan simgeler. Bulunan 42 texture dahil; ayrı START/PS ve L3/R3 bitmap'i bulunduğu iddia edilmemeli.
- `resources/controller/ps3-controls.bin` ve `.json`: orijinal DualShock 3 kontrol ekranı.
- `resources/icons/game.ico`: kullanıcının kapak resmi; `launcher.ico`: ayrı MotoGP logosu. `sources/` özgün girdiler, `app.rc.in` Windows resource şablonu.
- `resources/localization/tr/messages.json`: 2.102 ID'lik katalog; 1.700 çeviri, 402 özel ad/birim vb. korunmuş değer. `manifest.json` ve `overlay/`: runtime dağıtım verisi.
- Türkçe overlay tam 10 dosyayı değiştirir: `text/english.msg`, beş `.english.lua` gösterim dosyası ve dört `.font`. Özgün ses/diğer dosyalar alttaki oyun ağacından okunur.
- `third_party/rexglue-runtime/manifest.json` ve runtime ZIP'i: proje patch'li Windows DLL'leri ve hash'leri.
- `third_party/fidelityfx/`: FidelityFX sağlayıcıları/başlıkları. `third_party/SDL_GameControllerDB/`: controller DB ve lisans. `third_party/open-sans/`: UI fontu/lisansı. Diğer lisanslar da dağıtımda korunur.

### Yalnız mevcut bulutta bulunan yollar

| Yol | İçerik / taşınabilirlik |
|---|---|
| `/workspace/shared/rexglue-nightly-packages/win-amd64` | Resmî SDK paketi; Git clone ile gelmez |
| `/workspace/shared/rexglue-sdk-nightly` | Sabit commit'teki SDK kaynak ağacı ve patch'ler |
| `/workspace/shared/sysroot` | Rootless LLVM/Wine/Vulkan/Xvfb vb. araçlar |
| `/workspace/shared/windows-sysroot` | MSVC/Windows SDK çapraz derleme sysroot'u |
| `/workspace/shared/msvc-redist` | Runtime destek DLL'leri |
| `/workspace/shared/python-windows-embed` | Windows sunucu QA'sı için Python; üretim kurulumu değildir |
| `/workspace/shared/wine-prefix` | QA Wine profili; kullanıcı verisi/kimlik dosyaları kopyalanmamalı |
| `assets/game-tu` | Bu bulutta yalnızca XEX + XEXP staging'i; tam oyun değildir |
| `assets/disc/extracted/MotoGP 09-10` | Kullanıcıya ait tam disk çıkarımı; Git dışında |
| `out/turkish/game` | Güncel TU XEX/XEXP + tam xenondata bağlantısı bulunan özel test fixture'ı |
| `out/turkish` | Çözümlenmiş orijinal font/metin/script araç girdileri; runtime paketi değildir |
| `out/localization-fix`, `out/security2` | Son yerelleştirme/güvenlik test kayıtları; Git dışında |
| `out/build/windows-nightly-release` | Mevcut çapraz derleme çıktısı |
| `out/build/win-amd64-release` | Native Windows derleme betiğinin çıktı yolu |
| `out/build/linux-nightly-release` | Destekleyici Linux menü/grafik doğrulama build'i |
| `out/package-windows`, `out/dist` | Yeniden üretilen staging ve arşivler; kaynak değildir |

## 4. Sürüm ve runtime bağımlılıkları

**Tek sürüm kaynağı:** [sdk-version.json](sdk-version.json).

```text
SDK version : 0.10.0.24-dev.gbd833a2
CMake       : SDK package version 0.10.0.24
SDK commit  : bd833a2ab69d305f99ddf57418fc38ffec8af93d
Release tag : nightly-20261002-bd833a2a
Game        : 0.0.1.1 / Title ID 434307F0
```

Manifest içindeki `sdk_version = "0.10.0"` codegen API alt sınırıdır; projenin stable SDK kullandığı anlamına gelmez. Eski belgelerdeki stable milestone açıklamaları tarihsel kayıttır. Nightly geçişi [SDK migration notlarında](docs/sdk-nightly-migration.md) anlatılır.

Çalışan EXE, DLL'ler ve bridge export'ları birlikte uyumlu olmalıdır. Resmî SDK DLL'lerini proje patch'li DLL'lerin yerine koymayın; hash doğrulamasını geçmek için manifesti rastgele değiştirmeyin.

Runtime arşivi: `third_party/rexglue-runtime/rexglue-0.10.0.24-dev.gbd833a2-custom-windows-x64.zip`.

| Bileşen | Beklenen SHA-256 |
|---|---|
| Runtime ZIP | `0c0c17bb8ee0b22770ac035482c3e5ec31daa8ee5590fb831918f3f9bbac3735` |
| `rexruntime.dll` | `6599aacfa58980a6757242510c978e21741f9b21ea5b589a08c8be7f2085770f` |
| `rexgpu-xenos.dll` | `bc343d689980d056dfa28990a9ed457575ce38fe10d0ad1a15594c40a777f9f7` |
| `amd_fidelityfx_dx12.dll` | `12a5081257ec95b0b53ad51b4a87fb3c03f97fe0bbb59f9496968f8d50ef93a6` |

Asıl hash/provenance doğrulaması için [runtime manifestini](third_party/rexglue-runtime/manifest.json) okuyun. Yeni derleyiciyle yeniden oluşturulan DLL farklı hash verebilir; bu, yeni provenance ve test gerektirir.

Korunacak SDK patch grupları `cmake/` altında: stability, explicit-video-mode, symmetric-deadzone, livery-readback, experimental-temporal-fsr, ultrawide-presentation. `scripts/build-spatial-sdk.py` sabit SDK ağacına bunları uygular. `sdk-spatial-fsr.cmake` / `sdk-temporal-fsr.cmake` köprü kaynaklarını ekler.

Beklenen kullanıcı oyun girdileri:

```text
stock default.xex SHA-256
54245417e996ed0aa561a5a803fd810e87684bd0b43082f316db711b3ee786ae
default.xexp SHA-256
0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079
TU_11K61VG_0000004000000.0000000000081 SHA-256
441df6cc2e20ac8aeeed475138c1714d477f965423d8c5c9b0202e3a18d8141f
```

## 5. Yeni ortama taşıma

### Git ile alınacaklar

```sh
git clone https://github.com/inflamesgo/motogp0910-recomp.git
cd motogp0910-recomp
git status --short
git log -5 --oneline
```

Repo özel ise mevcut Git erişimiyle kimlik doğrulayın. Token'ı URL içine koymayın. Yeni checkout; `assets/`, `out/`, üretilmiş oyun C++'ı, haricî SDK/toolchain ve özel test loglarını getirmez.

### Ayrı hazırlanacaklar

1. Kullanıcının tam Xbox 360 disk çıkarımı ve doğru TU; yalnız XEX yeterli değil. PS3 dump ana oyun girdisi olarak kullanılmaz; yalnız seçili kontrol görsellerinin kaynağıdır.
2. Windows build toolchain'i veya mevcut Linux çapraz derleme ortamının yeniden kurulması.
3. Gerekirse Git dışındaki özel test kanıtları ve çeviri araç girdileri. Bunlar dağıtımın parçası değildir.
4. Sunucu devri varsa SQLite durum verisi, proxy anahtarı, domain/cert durumu ve servis yöneticisi ayarları. Bunları yalnız yetkili sunucu yöneticisi taşımalı.

Tüm `/workspace` dizinini arşivleyip devretmeyin. Başka depolar, bulut kimlik bilgileri, Wine profilleri, cache'ler ve özel oyun verisi karışabilir. Dosyaları amaçlarına göre açıkça seçin. Git'teki `.gitignore` taşıma/yedekleme politikası yerine geçmez.

`generated/default/` elle düzenlenecek kaynak değildir; manifest/codegen'den yeniden üretilir. Elle keşfedilmiş callback/CRT fonksiyonlarını manifestten silmeyin. Yeni makinede “dosya eksik” hatasını kaynak kodu rastgele silerek çözmeyin.

## 6. Windows derleme, çalıştırma ve paketleme

### Native Windows derleme

Gerekli: Visual Studio 2022+ Desktop development with C++, Windows SDK, **MSVC hedefli LLVM Clang 20+**, CMake 3.25+ ve Ninja. MinGW toolchain'i kullanmayın. VS 2026 araç keşfi betikte desteklenir; bu bulutta native VS 2026 tam derlemesi doğrulanmış değildir.

Depo kökünde, yolları kendi makinenize göre değiştirin:

```powershell
.\scripts\build-windows.ps1 `
  -GameRoot 'D:\Games\MotoGP0910' `
  -TitleUpdatePath 'D:\Updates\TU_11K61VG_0000004000000.0000000000081' `
  -Jobs 2
```

Betik SDK'yı sürüm/hash kontrolüyle indirir; XEX/TU'yu doğrular; STFS aracını ve patch'i hazırlar; codegen, Release ve headless probunu çalıştırır. Tam diski kopyalamaz. Resmî DLL staging'inden sonra proje patch'li runtime'ı son kez yerleştirir. Çıktı `out/build/win-amd64-release/` altında oyun, launcher, crash monitor ve DLL'lerdir.

```powershell
.\scripts\run-windows.ps1 -GameRoot 'D:\Games\MotoGP0910'
```

Bu geliştirme çalıştırıcısı kendi `out` log/userdata alanını kullanabilir; dağıtılmış EXE'nin normal Documents kaydıyla karıştırmayın. Ayrıntılı araç kurulumları: [Windows build rehberi](docs/windows-build.md).

### Mevcut Linux bulutundan Windows çapraz derleme

```sh
bash scripts/install-windows-cross.sh
source scripts/cloud-env.sh
python3 scripts/check-game-inputs.py
bash scripts/build-windows-cross.sh
```

Kurulum betikleri mevcut Debian 13/rootless bulut ve `/workspace/shared` düzenine göre yazılmıştır. Başka distroya veya üretim VPS'ine aynı şekilde uygulanacağı varsayılmamalıdır. Son betik dört paralel iş kullanır; RAM kısıtlıysa yapılandırılmış build'de `cmake --build out/build/windows-nightly-release --parallel 2` tercih edilebilir. VPS'in oyun derleme toolchain'ine ihtiyacı yoktur.

### Oyuncunun klasör düzeni

```text
MotoGP0910/
├── motogp0910.exe
├── motogp0910-launcher.exe
├── motogp0910-crash-monitor.exe
├── motogp0910.map
├── rexruntime.dll / rexgpu-xenos.dll / diğer runtime DLL'leri
├── gamecontrollerdb.txt
├── resources/controller/...
├── resources/localization/tr/...
├── resources/ui/OpenSans-Semibold.ttf
├── licenses/...
└── game/                       Kullanıcı kendi diskinden ekler
    ├── default.xex
    ├── default.xexp veya doğru TU paketi
    ├── xenondata/...
    ├── nxeart/...
    └── diskteki diğer gerekli dosyalar
```

Game folder launcher'dan da seçilebilir. Açık `--game_data_root` varsa o kullanılır; yoksa EXE yanında `game/default.xex`, sonra eski düz yerleşim aranır. İlk açılış TU'dan eksik patch'i çıkarabilir; özgün XEX ve mevcut patch'i yeniden yazmaz. Yalnız EXE güncellemek yeterli değildir: uyumlu DLL ve `resources/` birlikte dağıtılmalıdır.

### Geliştirme paketi üretimi — yeni README politikası

Mevcut bulutta:

```sh
python3 scripts/package-windows.py \
  --build-dir out/build/windows-nightly-release \
  --archive MotoGP0910-Windows-x64-community-online.zip \
  --online-preview

python3 scripts/package-community-server.py
```

Windows ZIP ve yan hash `out/dist/` içine çıkar. Sunucu ZIP/hash varsayılan `downloads/` içine çıkar; test için `--archive out/dist/isim.zip` verilebilir. Windows paketini paylaşmak için test sonrası ZIP ve `.sha256` birlikte `downloads/` altına taşınır/kopyalanır. Bu adım eski arşivi gerçekten değiştireceğinden bilinçli yapılmalıdır.

**Varsayılan:** README, kontrol paketi README'si ve özellik/kurulum yönergesi yok. Gerekli `.bin`, `.json`, font, DLL, `.map`, controller DB, lisans/NOTICE ve checksum'lar kalır. `--include-one-pc-test` ayrıca istenirse yalnız yerel test yardımcı betiğini ekler; açıklama dosyası eklemez. `--include-readmes` eski açıklamaları açıkça ekleyen opsiyonel yoldur; normal geliştirme teslimatında kullanılmamalıdır. Kullanıcı final README'sini kendisi hazırlayacak.

`--online-preview`/`--lan-preview` artık esasen opsiyonel açıklamaların kapsamını belirler; C++ özelliklerini derleme sırasında açan anahtarlar değildir.

**Taşınabilirlik borcu:** Windows paketleyicide lisans toplama için `/workspace/shared/...` mutlak yolları var. Native Windows'ta yalnız `--build-dir` vermek paketleyiciyi tamamen taşınabilir yapmaz. Yeni ortam için bu kaynakların yolları düzenlenmeli; lisanslar eksiltilmemeli. Kaynak derleme ile dağıtım paketleme farklı adımlardır.

Paketleyici runtime hash'lerini ve ZIP bütünlüğünü denetler. Orijinal XEX/XEXP/TU/tam disk verisi dağıtım arşivine eklenmemelidir. Sunucu paketleyici 56 stdlib regresyonunu geçmeden arşiv üretmez; veritabanı/anahtar yerine yalnız beyaz listedeki kaynak/şablonları alır.

## 7. Ayarlar, kayıtlar, kimlik ve loglar

| Veri | Varsayılan konum / not |
|---|---|
| Oyun ayarları | EXE yanındaki `motogp0910.toml` |
| Launcher tercihleri | EXE yanındaki `motogp0910-launcher.toml` |
| SDK kullanıcı verisi / oyun kayıtları | Windows Documents altında `motogp0910`; Known Folder/OneDrive yönlendirmesi veya `user_data_root` / `REX_USER_DATA_ROOT` ile değişebilir |
| Başarımlar | Kullanıcı veri kökünde `achievements/434307F0.toml` |
| Başarım bildirim/ses tercihi | Kullanıcı veri kökünde `achievement-ui.toml` |
| Online profil | `%LOCALAPPDATA%\MotoGP0910\Online\<sunucu-adresi-hashi>.profile` |
| Oyun logları | Varsayılan EXE yanında `logs/motogp0910_001.log` vb. |
| Crash raporu | `logs/crash-*/report.txt`, `crash.dmp`, log kopyası; gerekirse LocalAppData/TEMP fallback |
| Sunucu durumu | Çalışma dizinine göre `data/community.sqlite3`, SQLite WAL/SHM ve özel `data/proxy.key` |

Tanı için kariyer kaydı, başarımlar veya Online profili silmeyin. Aynı `.map` dosyasını EXE ile saklayın; farklı build'in map'iyle crash adresi çözmeyin. Dump bellek parçaları ve özel yollar içerebilir; herkese açık site deposuna koymayın.

Temel anahtarlar: `network_mode = "offline" / "lan" / "online"`, `online_server = "https://play.example.com"`, `game_text_language = "turkish"`. Eski `lan_enabled` yalnız yeni mod anahtarı yoksa geriye uyumluluk içindir. Türkçe için SDK `user_language=1` korunur; launcher'ın iç Türkçe değeri `1000` native dil enum'u olarak gönderilmez.

Giriş: Enter=Start, Space=A, Backspace=B. F1 canlı ayarlar, F7 başarımlar. PS simgesini XInput koluyla zorlamak için:

```powershell
.\motogp0910.exe --input_backend=xinput --button_prompt_style=playstation
```

Otomatik Sony tanıma için native SDL kullanılır; XInput wrapper cihazı Xbox gibi sunabilir. Simge seçimi fiziksel DualSense adaptif tetik desteği kazandırmaz.

### Online kimliği gerçekte nasıl çalışıyor?

Rastgele 256-bit profil sırrı Windows DPAPI ile kullanıcı/makineye bağlı saklanır. Sunucu yalnız SHA-256 özetini ve kalıcı oyuncu ID'sini tutar. Görünen ad veya IP kimlik değildir. Aynı isim farklı kişilerce seçilebilir; IP/ad değişince aynı profilin skorları korunur.

Sunucu SQLite oyuncu tablosunda IP sütunu yoktur; aktif endpoint'ler ve geçici kotalar RAM'de IP kullanır. Proxy/işletim sistemi log politikası ayrıdır.

**Hesap girişi ve taşınabilir kurtarma yok.** DPAPI dosyasını başka PC'ye kopyalamak taşınabilir yedek değildir. Windows yeniden kurulumu/profil silme kimliğe erişimi kaybettirebilir. Profil seçimi tam girilen server URL'sine bağlı olduğundan adres yazımı/hostname/port değişikliği başka profil seçebilir. Yayına başlamadan sabit bir domain seçmek değerlidir.

Gelecek tasarım: [community-accounts-plan.md](docs/community-accounts-plan.md). Kullanıcı adı/şifre, Argon2id, güvenli oturum yenileme/kurtarma ve mevcut ID'ye bağlama planı henüz uygulanmadı. Web sitesi açılınca bunların kendiliğinden hazır olacağı varsayılmamalıdır.

## 8. Linux/Windows VPS kurulumu

### VDS'nin yapacağı iş

Oda rehberi, authenticated UDP relay ve kalıcı native leaderboard kayıtları. Yarış simülasyonu oyunculardan birinin PC'sinde kalır; host çıkarsa yarış oturumu biter. VDS Windows oyun EXE'sini çalıştırmaz. Bu sistem resmî Xbox Live ranked, arkadaş, davet, ses veya Marketplace hizmetlerini geri getirmez; **Player Match** kullanılır.

Sunucu Python **3.11+ stdlib** kullanır; pip/npm/Docker zorunluluğu yok. Üretimde desteklenen, güvenlik güncellemeleri uygulanmış Python/Caddy kullanın. İki işletim sistemi de desteklenir. Henüz bu projeye ait gerçek domain/IP/VDS kurulumu yapılmış değildir.

| Port | Erişim | Kullanım |
|---|---|---|
| TCP 80 | Public | Caddy standart sertifika/HTTP yönlendirme akışı |
| TCP 443 | Public | HTTPS kontrol API'si |
| UDP 30821 | Public | Oyunun relay trafiği |
| TCP 30820 | Yalnız localhost | Python kontrol backend'i; internete açmayın |

DNS `A` kaydı `play.example.com → VDS IPv4` olmalı. Bu bir örnek adres; gerçek IP koda gömülmez. Oyuncu launcher'dan HTTPS origin'i girer. Oyun domain'i **DNS only/doğrudan** olmalı; yalnız web trafiği taşıyan CDN proxy'si UDP relay'i taşımaz. Şimdiki istemci IPv4 rota kullanır; yalnız AAAA kaydı yeterli değildir.

### Linux — elle ilk kurulum

Sunucu paketini ayrı dizine çıkarın veya depodaki `server/` dizinini kullanın. Python/Caddy'yi OS'nin normal paket mekanizmasıyla kurun. Aşağıdaki komutların çalışma dizini sunucu dosyalarının yanıdır:

```sh
python3 proxy_config.py init
python3 community_server.py --proxy-secret-file data/proxy.key
```

`init` bir kez çalıştırılır; var olan anahtarın üstüne yazmaz, anahtarı ekrana basmaz. İkinci terminalde:

```sh
cp Caddyfile.example Caddyfile
# Caddyfile içindeki play.example.com değerini gerçek domain ile değiştirin.
python3 proxy_config.py run
```

Caddy'nin Linux'ta 80/443 bind izni ayrı sağlanmalı: distro servisinin dar `CAP_NET_BIND_SERVICE` yetkisi veya hazırlanmış servis şablonu kullanılabilir. Python backend root olarak çalıştırılmamalı. ZIP'te shell execute biti korunmayabileceğinden doğrudan Python komutu en açık yoldur.

Kalıcı işletim için [motogp-online.service.example](server/motogp-online.service.example) ve alternatif [motogp-caddy.service.example](server/motogp-caddy.service.example) var. Bunlar otomatik kurulmaz. Yönetici `motogp`/`caddy` kullanıcılarını, `/opt/motogp-online`, `/etc/motogp-online` ve özel state dizinlerini hazırlamalıdır. Backend örneği DB/key için `/var/lib/motogp-online`, proxy örneği `/var/lib/motogp-caddy` kullanır. İki süreçte **aynı anahtar** okunmalı; ayrı kullanıcılara özel 0600 kopyalar sağlanabilir. Anahtarı paylaşmak için state dizinini herkese okunur yapmayın. Distro Caddy servisiyle alternatif Caddy unit'ini aynı anda çalıştırmayın.

### Windows Server — elle ilk kurulum

Python/Caddy PATH'te olmalı. Sunucu dizininde:

```powershell
python .\proxy_config.py init
powershell -NoProfile -ExecutionPolicy Bypass -File .\start-server.ps1 --proxy-secret-file data/proxy.key
```

İkinci terminalde `Caddyfile.example` dosyasını `Caddyfile` olarak kopyalayıp domain'i değiştirin:

```powershell
python .\proxy_config.py run
```

TCP 80/443 ve UDP 30821 hem sağlayıcı firewall'ında hem Windows Defender Firewall'da izinli olmalı. Windows ACL'leriyle `data/` ve key erişimi servis/yedek yöneticisine sınırlandırılır; Linux chmod Windows ACL'si yerine geçmez. Kalıcı servis kurulumu yöneticinin seçtiği service manager ile yapılır; betikler bunu veya firewall değişikliğini otomatik yapmaz. Backend/proxy Administrator veya LocalSystem olarak çalıştırılmamalı.

### TLS ve bağlantı kontrolü

Caddy normal herkese güvenilir TLS sertifikasını sağlar. Özel CA kullanılıyorsa istemci makinelerinde güven zinciri ayrıca gerekir; doğrulamayı kapatmayın. Doğrudan Python TLS modu da `--cert/--key` ile mevcut; ayrıntıları [kurulum belgesinde](docs/community-online.md). HTTP yalnız loopback geliştirmesinde kabul edilir.

```sh
curl --fail https://play.example.com/health
```

Gerçek domain ile JSON `service: motogp0910` beklenir. Sağlıklı HTTP yanıtı UDP relay'in dışarıdan erişildiğini veya yarışın senkron olduğunu kanıtlamaz. Sonrasında iki Windows PC ile host/arama/join/yarış ve yeniden bağlantı testi gerekir.

Kullanıcının SSH erişimi olması oyuncu bağlantısı için gerekmez. VDS'yi yöneten arkadaş şu işleri yapabilmeli: dosyaları yükleme/güncelleme, servisleri başlatma, DNS/firewall/TLS, log inceleme ve yedek. Geliştiriciye en az public origin, kullanılan paket sürümü ve temizlenmiş hata logu yeterli olur; yönetici anahtarları veya DB kopyası herkese paylaşılmaz.

### Kalıcı veri ve yükseltme

- Varsayılan sınırlar: 256 aktif istemci, 128 oda, 10.000 kayıtlı oyuncu, 256 MiB DB. Bunlar kapasite garantisi değildir; gerçek VDS kaynaklarına göre ölçülmeli.
- Oyun/sunucu **MGO2 / `/v2`** uyumlu birlikte güncellenir. Eski Online build'ler reddedilir; LAN02 ayrı kalır.
- DB/profilleri koruyun; sunucu yeniden başlayınca kalıcı skorlar kalır ama aktif odalar/bağlantılar yenilenir.
- Canlı DB dosyasını tek başına kopyalamak WAL'daki son kayıtları kaçırabilir. SQLite online backup API'si veya servis durdurularak tutarlı kopya kullanın. Yedek geri yükleme testi ayrıca yapılmalıdır; otomatik yedek sistemi henüz eklenmedi.
- Proxy anahtarı ve gerekiyorsa TLS özel anahtarı ayrı, erişimi sınırlı yedeklenir. Git'e/oyun ZIP'ine girmez.

## 9. Web sitesi ile entegrasyon planı

**Bu bölüm önerilen sonraki mimaridir; mevcut özellik gibi sunulmamalıdır.** Depoda şu anda frontend, site framework'ü, Docker/Compose dağıtımı, public leaderboard JSON endpoint'i veya web hesabı yok.

```text
Oyuncu Windows uygulaması
  ├─ HTTPS → play.example.com:443 → Caddy → 127.0.0.1:30820 Python /v2
  └─ UDP   → play.example.com:30821 → authenticated relay

Tarayıcı
  └─ HTTPS → www.example.com → ayrı site
                            └─ gelecekte sınırlı salt okunur leaderboard API'si

Python service → özel SQLite veritabanı
```

Site ve game service aynı VPS'te farklı host adlarıyla veya ayrı makinelerde olabilir. Siteyi CDN üzerinden sunmak mümkün; oyun UDP domain'i doğrudan ulaşılabilir kalmalıdır. Yeni siteyi eklerken mevcut oyun endpoint'lerinin path/port/anahtar doğrulamasını bozmamak gerekir.

### Mevcut API, browser API'si değildir

| Endpoint | Mevcut anlamı |
|---|---|
| `GET /health` | JSON servis sağlık/oda/oyuncu sayıları |
| `POST /v2/connect` | Native istemci bağlantısı ve kısa ömürlü relay verisi |
| `POST /v2/disconnect` | Native bağlantı kapatma |
| `POST /v2/stats/read` | Binary native leaderboard okuma |
| `POST /v2/stats/write` | Binary native skor yazma |

`/v2` gövdeleri **application/octet-stream**, kimlikleri oyuncunun 256-bit profil sırrına dayanır. Bunlar doğrudan tarayıcının kullanacağı JSON REST/login uçları değildir. DPAPI sırrını tarayıcıya taşımayın; proxy anahtarını web JavaScript'ine koymayın. SQLite dosyasını statik site kökünde sunmayın.

Site için önerilen sıra:

1. Var olan paketleri, sürüm/hash ve kullanıcıya ait oyun verisi gereksinimini gösteren statik indirme/durum sayfası. “Resmî Xbox Live geri geldi” veya “dedicated yarış sunucusu” denmemeli.
2. Ayrı, salt okunur JSON leaderboard API'si: sabit view ID doğrulaması, sayfalama/üst sonuç sınırı, timeout, cache, yalnız izinli alanlar. Görünen ad, public oyuncu ID ve skor yeterli; credential/hash/ticket/proxy key dönmemeli.
3. API'nin testleri: bozuk/çok büyük parametre, olmayan view, sıralama/tie, boş tablo, pagination, eşzamanlı yazım ve origin/cache davranışı.
4. Daha sonra kullanıcı onayıyla hesap tasarımını uygulama; mevcut profillerin skorlarını yeni hesaba atomik bağlama ve geri dönüş planı. Yeni web hesabı eski DPAPI kimliğiyle kendiliğinden aynı kişi sayılmaz.

Caddy game host'u şu anda yalnız yukarıdaki API path'lerini kabul edip kalanına 404 döner. Site route'ları ayrı host bloklarında hazırlanmalı. Mevcut API'nin 2 KB body ve kısa timeout sınırlarını site upload/formlarına düşünmeden aynen uygulamayın; aynı şekilde site için gevşetilen limitleri oyun API'sine yaymayın.

Henüz kararlaştırılmamış: gerçek domain, VPS OS/kaynakları, site stack'i ve site yöneticisi. Bunlar kurulumdan önce netleştirilmeli; belgeyi hazırlamak için adres uydurmak veya hesabı uygulamaya başlamak gerekmiyor.

## 10. Testler ve teşhis

### Hızlı ve anlamlı kontroller

Depo kökünden sunucu regresyonu:

```sh
python3 -m unittest discover -s server -p 'test_*.py' -v
```

Windows native test hedefleri ve temel doğrulama:

```powershell
cmake --build --preset win-amd64-release --target windows_validation_probes --parallel 2
python scripts/verify-windows.py --build-dir out/build/win-amd64-release --game-root 'D:\Games\MotoGP0910' --skip-ui
```

UI testleri için `--skip-ui` kaldırılır; kullanılabilir desktop gerekir. Probe'lar paketlenen normal oyuncu araçları değildir. Komut çalışmadan önce ilgili C++ hedefleri derlenmiş olmalı.

Mevcut bulutta gerçek Windows PE online istemcisi + yerel backend:

```sh
python3 scripts/verify-online.py out/build/windows-nightly-release \
  --wine /workspace/shared/sysroot/usr/lib/wine/wine64 \
  --game-root out/turkish/game
```

Windows Python backend'i ayrıca denemek için `--server-python /workspace/shared/python-windows-embed/python.exe`. Bu test HTTP loopback kullanır; public WinHTTP TLS testi diye raporlanmamalıdır.

Türkçe katalog/overlay:

```sh
python3 scripts/build-turkish-localization.py --check
cmake --build out/build/windows-nightly-release --target localization_probe --parallel 2
```

`localization_probe.exe` iki konum alır: **doğru TU XEX/XEXP + tam oyun verisi bulunan kök**, `resources/localization/tr/overlay`. Eksik `assets/game-tu` fixture'ını kullanmak yalancı hataya yol açar. `scripts/build-turkish-fonts.py` yeniden üretiminde özel çözülmüş font girdileri ve Pillow 11.2+ gerekir; normal oyun build'i Pillow istemez. `tools/localization_data_check.lua` beş Lua dosyasının gösterim dışındaki verilerinin değişmediğini karşılaştırır.

Diğer hedefli araçlar:

| Yol | Kapsam |
|---|---|
| `scripts/verify-lan-service.py` | Ayrı süreçlerle LAN keşif/doğrudan bağlantı |
| `scripts/verify-online-security.py` | Sahip olunan yerel servis üzerinde HTTP/TLS/Caddy negatif kontrolleri |
| `scripts/verify-online-properties.py` | Model tabanlı protokol denemeleri; Hypothesis yalnız QA bağımlılığı |
| `scripts/verify-online-load.py` | Sınırlı yerel UDP yük denemesi; üretim servisine izinsiz uygulanmaz |
| `scripts/verify-online-winhttp.py` | Windows istemcisinin TLS yolu; Wine sertifika sınırını aşağıda okuyun |
| `scripts/verify-crash-reports.py` | Crash monitor/rapor üretimi |
| `tools/*_probe.cpp` | Gerçek C++ UI, native import/ABI, font, input ve render durum kontrolleri |

### Güvenlik testlerinin gerçek kapsamı

Kaynak: [community-security.md](docs/community-security.md). Eski belgelerdeki daha küçük test sayıları tarihsel snapshot'tır.

- 56 stdlib regresyonu Linux ve Windows Python 3.12.8 Proactor/Wine üzerinde geçmiş durumda.
- Gerçek Windows PE kontrol/relay/stat istemcisi ve native stats ABI testleri hem Linux hem Windows backend ile geçmiş; restart sonrası kalıcılık kontrol edilmiş.
- TLS/proxy testleri Caddy ve Python TLS ile yerelde geçti. **Wine WinHTTP pozitif sertifika güveni testi 12157/invalid CA nedeniyle tamamlanamadı.** TLS doğrulaması kapatılmadı; native Windows + public trusted domain doğrulaması hâlâ gerekli.
- Aynı NAT arkasında 20 istemci ve tekrar bağlanma; bozuk MAC/replay'in başka oyuncunun kotasını harcamaması; proxy başlıkları; gövde/soket/depolama sınırları test edildi.
- Windows/Wine 1.000 paket/sn denemesi geçti. 8.000 paket/sn stresinde 914 echo'nun 396'sı döndü; health çalışsa da bu yük testi **başarısız**. Selector alternatifinin sonucu da iyi olmadığından bu değişiklik gönderilmedi.
- Model/property testleri yüz binlerce sınırlı eylem/invariant denedi. Bu, sıfır açık veya DDoS dayanıklılığı garantisi değildir; gerçek VPS OS/provider güvenliği burada taranmadı.
- Mevcut skorlar istemciden gelir: değiştirilmiş istemci **kendi kimliği adına sahte skor** gönderebilir. Yetkili race simulation/anti-cheat uygulanmış değildir.

### Tekrarlanan hatalardan çıkarılan çalışma kuralı

Önce kullanıcının logunu ve çalışan kod yolunu okuyun. Başlangıçta kapanan süreç, menüye dönen bağlantı ve görsel bozulma ayrı olaylardır. VFS'de tek `ResolvePath` testi gerçek oyun `OpenFile` yolunu kanıtlamadı. Sentetik Press Start fixture'ı gerçek boot durumunu kaçırdı. Doğru formatlı PS3 texture bile yanlış row pitch veya replay satırına uygulanınca bozuldu.

Yeni regresyon, mümkün olduğunca gerçek çağıranı/native ABI'yi ve paketlenmiş dosyaları çalıştırmalı. Başarılı CPU testi GPU görsel testi diye, yerel relay testi Internet yarışı diye sunulmamalı. Kullanıcı logları/özel anahtarları çıktılara gereksiz dökmeyin.

## 11. Açık işler ve devralma sırası

### İlk oturumda yapılacaklar

1. Depo/commit/runtime manifestini karşılaştır; kullanıcıya özel kaydedilmemiş değişiklikleri koru. Bu belge kaynak snapshot'ıdır, sonraki commit'leri ezmek için kullanılmaz.
2. Son Windows paketinin Türkçe VFS düzeltmesini içerdiğini ve kullanıcının gerçekten onu denediğini netleştir. Native Windows'ta ana menü, kariyer/eğitim, küçük İ/ı ve oyun içi metin görünümünü kontrol et.
3. Ortam taşındıysa tam özel oyun girdileri ile SDK/toolchain yollarını doğrula; önce küçük probe, sonra gerekli build. Her arızada SDK'yı yeniden kurma.
4. Site/VPS için yalnız eksik dağıtım bilgilerini iste: domain, IPv4, OS, yöneten kişi, DNS/firewall erişimi. Kullanıcı yerine VDS arkadaş yönetiyorsa ona uygulanabilir kurulum dosyaları ver.
5. Önce mevcut Online servisini sabit domain/public TLS ile ayağa kaldır, sonra iki Windows istemciyle oda/join/yarış/reconnect/leaderboard persistence testi yap. Site API'si ve hesap sistemi bundan ayrı teslimatlar olsun.

### Gerçekten açık kalanlar

- Son Türkçe paketin kullanıcı Windows GPU onayı ve oyun genelinde metin taşması/kapsam kontrolü.
- Native Windows FSR 2/3 dispatch/görüntü kalitesi; doğru motion/depth/jitter olmadığından deneysel kapsamın açık kalması. Kalite modları mevcut görüntüyü yeniden örnekler; native sahne maliyetini kendiliğinden azaltmaz. History her kare resetlenir; FSR 1 fallback korunur.
- Ultrawide yarış kameraları, HUD, video/cutscene ve culling kenar durumları. Dikey görüş/derinlik korunarak yatay genişletme korunmalı.
- Fiziksel DualSense/Edge USB/Bluetooth efekt hissi ve cihaz ayrılması. Mevcut efektler yapılandırılmış dirençtir; RPM/lastik tutuşu gibi telemetriyle sürülen bir sistem olduğu iddia edilmemeli. Menü kullanıcı tarafından şimdilik kabul edildi; yeniden tasarım acil değil.
- Public VPS üzerindeki native WinHTTP HTTPS ve Windows oyuncularla Online yarış/leaderboard testi.
- Paketleyici lisans yollarının yeni makinelere taşınabilir hale gelmesi; README'siz varsayılanın korunması.
- Web sitesi ve sınırlı public JSON leaderboard API'si: sıfırdan yapılacak, mevcut native binary API bozulmadan.
- Taşınabilir hesap/kurtarma: yalnız kullanıcı tekrar bu işe öncelik verdiğinde, mevcut oyuncu ID/skor geçişiyle.
- Operasyon: otomatik backup/restore provası, gözlemleme, servis restart ve disk dolması prosedürü; gerçek VDS'de henüz kurulmadı.

### Yapılmaması gereken varsayımlar

“Bütün güvenlik açıkları kapandı”, “bütün menüler Windows'ta görsel olarak doğrulandı”, “FSR3 var yani frame generation var”, “sunucu yarış fiziklerini çalıştırıyor”, “LAN çalıştı yani public Online da test edildi”, “oyuncu adı hesap kimliğidir”, “site bağlanınca hesap sistemi hazır olur” ifadeleri mevcut kanıtı aşar.

Bu belgeyi alan agent önce mevcut işleri tamamlasın; kullanıcı istemeden Linux oyun optimizasyonu, SDK sürüm yükseltmesi veya fizik değişikliği başlatmasın. Uzaktan yayınlama/production DB geçişi somut hedef ve yönetici erişimi belli olduğunda ayrıca ele alınmalı.

## 12. Kaynak belge dizini

| Konu | Öncelikli kaynak |
|---|---|
| Güncel durum / tarihçe | [project-status](docs/project-status.md), [roadmap](docs/roadmap.md), [README](README.md) |
| Native Windows kurulum | [windows-build](docs/windows-build.md), [game-layout](docs/game-layout.md), [direct-launch](docs/direct-launch.md) |
| SDK/runtime | [sdk-nightly-migration](docs/sdk-nightly-migration.md), [stability-quality-control](docs/stability-quality-control.md) |
| Türkçe | [turkish-localization](docs/turkish-localization.md) |
| Online kurulum / kimlik | [community-online](docs/community-online.md) |
| Güvenlik ve test sınırları | [community-security](docs/community-security.md) |
| Gelecek hesap tasarımı | [community-accounts-plan](docs/community-accounts-plan.md) |
| LAN/Radmin ve lobi | [lan-multiplayer-test](docs/lan-multiplayer-test.md), [lan-lobby-stability](docs/lan-lobby-stability.md), [lan-startup-fix](docs/lan-startup-fix.md) |
| Launcher | [launcher](docs/launcher.md), [launcher-validation](docs/launcher-validation.md) |
| Başarımlar / canlı ayarlar | [achievements](docs/achievements.md), [in-game-settings](docs/in-game-settings.md) |
| Çıkış | [quit-menu](docs/quit-menu.md) |
| PS3/klavye kontrolü | [controller-prompts](docs/controller-prompts.md), [keyboard-prompts](docs/keyboard-prompts.md), [controller-mappings](docs/controller-mappings.md) |
| Adaptif tetikler | [adaptive-triggers](docs/adaptive-triggers.md) |
| FSR/ultrawide | [experimental-rendering](docs/experimental-rendering.md); açılıştaki stable sürüm ifadesi tarihsel, güncel pin `sdk-version.json` |
| Kaplama/collision | [career-textures](docs/career-textures.md), [collision-data](docs/collision-data.md) |
| Crash teşhisi | [crash-reports](docs/crash-reports.md) |

Belge ile kod çelişirse sürüm manifestleri ve gerçek çalışan çağrı yolu esas alınmalı; çelişki güncel kanıtla belgelenmelidir. Yeni testin hangi ortamda, hangi commit/paketle yapıldığını ve henüz neyi kanıtlamadığını yazmak sonraki devri kolaylaştırır.
