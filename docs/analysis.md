# MotoGP 09/10 analiz kaydı

Bu kaydın adresleri yalnızca README'deki hash'lerle tanımlanan ikililer içindir. SDK: ReXGlue v0.10.0. Oyun kaynak/veri dosyaları bu belgede yayımlanmaz.

| Sürüm | XEX entry point | Eksik keşif hedefi | Doğrulanan sınır |
|---|---|---|---|
| Stok 0.0.0.1 | `0x823A6688` | `0x825F15C8` | `[0x825F15C8, 0x825F15D0)` |
| TU 0.0.1.1 | `0x823A5E10` | `0x82709EE0` | `[0x82709EE0, 0x82709FD8)` |

Stok analizde `0x825F15D4` hedefi bulunamadı. Section dump üzerinden disassembly, hedefte `addi r3,r3,8` ve ardından bilinen `0x825EFF28` fonksiyonuna tail branch olduğunu gösterdi. İki komutluk this-adjustment thunk açık 8-byte sınırla tanımlanınca stok kod üretimi sıfır unresolved-call hatasıyla tamamlandı.

TU uygulandıktan sonra eski adres kuralı taşınmadı. Yeni analizde `0x8270A0A4` adresinden `0x82709EE0` hedefine branch çözülemedi. Disassembly burada `fabs f13,f1` ile başlayan leaf FP rutinini, `0x82709FB4` ve `0x82709FD4` return yollarını ve `0x82709FD8` adresindeki sonraki rutini gösterdi. `0x8270A0A0` wrapper'ı `r4=1` seçip bu rutine gider. Açık end sınırıyla TU kod üretimi tamamlandı. Gerçek XEX verisini kullanan native entegrasyon probu, r4=0 için asin ve r4=1 için acos sonuçlarını beş girdide kontrol etti; 10/10 geçti.

Title update LIVE/STFS paketinde yalnızca `default.xexp` bulundu. SDK sibling-patch yolu ile yükler: aynı game root altında `default.xex` ve `default.xexp`. Uygulama kaynak imza hash'i, image-key ve patch-block hash kontrollerinden geçti. Kaynak XEX değiştirilmedi.

Doğrulanan sınırlar fonksiyonları stub'lamaz ve davranışlarını değiştirmez; yalnızca otomatik keşfin atladığı başlangıç/bitişi bildirir. `--force` kullanılmadı. Tüm oyunun işlevsel olarak doğru çevrildiği henüz doğrulanmadı.

Windows x64 uygulama ve headless probe, resmi Windows SDK/MSVC CRT ile Clang 20 MSVC ABI'sine çapraz derlendi. Aynı probe'un Windows PE çıktısı Wine altında gerçek TU sürümünü ve native mapping'i yükledi; 10/10 kontrol geçti. Ayrı Windows STFS aracı da patch'i doğru SHA-256 ile çıkardı.

Dağıtım ZIP'i ayrı bir klasöre çıkarıldı; yalnızca orijinal XEX ve TU eklendi, patch önceden bulunmuyordu. Uygulama farklı bir çalışma dizininden, argümansız başlatıldı. Executable'ın yanındaki oyun kökünü buldu ve `default.xexp` dosyasını beklenen `0419205ba10dc65eaa7b0bb62abc64cada46119ece5ed749e4ba20c06a6bc079` hash'iyle oluşturdu. Ardından GUI başlatma Direct3D 12 initialization aşamasında exit 3 ile durdu; bu host'ta ekran/GPU erişimi yok. Bu sonuç otomatik patch hazırlığını doğrular, grafik veya oynanışı doğrulamaz.

## İlk gerçek Windows logu: `0x8270DF08`

Kullanıcının 2026-10-12 tarihli `motogp0910_001.log` dosyasında GTX 1650 SUPER üzerinde Direct3D 12 ve SDL input başarıyla hazırlandı; XEX 0.0.1.1 patch'i uygulandı. İlk fatal hata, thread 17812'nin `0x8270DF08` adresini dolaylı çağırmasıydı. Eski native tablo bu adresi kaydetmiyordu.

TU `.text` section'ının orijinal komutları:

| Adres | Komut | Word |
|---|---|---|
| `8270DF08` | `cmpwi cr6,r3,0` | `2F030000` |
| `8270DF0C` | `bnelr cr6` | `4C9A0020` |
| `8270DF10` | `b 8270EA60` | `48000B50` |

Sonraki ayrı rutin `8270DF14` adresinde `blr` ile başlar. CRT init rutini `8270EAD0`, `lis r11,0x8271; addi r11,r11,-8440` ile callback adresini üretip bildirim kaydına yazar. `823B1840` kayıtları dolaşıp callback'i `bctrl` ile çağırır. İmmediate parçaları ayrı kaldığı ve callback'te standart prologue bulunmadığı için otomatik keşif adresi kaçırdı. Manifestte `[8270DF08,8270DF14)` sınırı tanımlanarak üç gerçek komut üretilir; sıfır argümanda mevcut CRT TLS cleanup rutini `8270EA60` çağrılır, diğer argümanlarda geri döner. `4C9A0020` komutunda BO=4 ve BI=26 (CR6 EQ biti) olduğundan koşul EQ=false, yani `bnelr`'dir.

Regresyon probu aynı adresin kayıtlı olmasını ve beş argümanda (`0`, `1`, `7FFFFFFF`, `80000000`, `FFFFFFFF`) dönüş/register/stack davranışını kontrol eder. Sıfır argümandaki cleanup yolu için test ortamında CRT TLS slotu `82941AF0` geçici olarak `FFFFFFFF` yapılır; cleanup'ın TLS-disabled yolu yürütülür ve özgün slot geri yüklenir. Aktif TLS/guest heap temizliği veya tüm oyun başlangıcı bu testin kapsamında değildir. Eski tabloyla test `Startup callback 0x8270DF08 is not registered` hatasını verdi. Yeni fonksiyon tablosu SDK tarafından üretilir; oyunun davranışı ve SDK kaynakları değiştirilmez.

Yeni kod üretimi sıfır unresolved-call hatasıyla tamamlandı; ilk üretimde 6 dosya değişti, 223 dosyanın içeriği aynı kaldı. Hedefli Windows entegrasyon testinde yeni callback TU'su, kayıt tabloları ve probe derlenip diğer değişmeyen oyun objeleriyle linklendi. Wine altında **10 asin/acos + 5 callback kontrolü geçti**, exit 0. Windows PowerShell betikleri sözdizimi kontrolünden geçti. Tam uygulamanın yeni Windows yerel derlemesi ve gerçek oyun başlangıcı kullanıcı bilgisayarında yapılacak; bu test o sonucu iddia etmez.


## İkinci Windows logu ve tam veri ile Linux açılışı

İkinci logda patch uygulandıktan sonra `0x82826D90` dolaylı çağrısı eksikti. TU `.data` bölümündeki `82874614..8287B5B4` CRT initializer tablosunun 7144 girdisi birlikte kontrol edildi; 24 kayıt initializer'ı eksikti. Bunlar sabit node/name/payload/size adreslerini yükleyip `82384B98` kayıt yardımcısına tail branch yapar. Manifest sınırları terminal branch'ten alınır; örneğin `82826E30` rutini `82826E54` adresinde biter, aradaki padding dahil edilmez. Probe 24 rutini gerçekten yürütür, beş word node içeriğini ve kayıt listesi başını kontrol eder; tüm değiştirdiği test alanlarını geri yükler.

Tam disk verisiyle Linux Vulkan denemesinde sonraki eksik çağrı `82500660` oldu. Bu prologue içermeyen leaf, iki bağlı listenin packed 16-bit anahtar maksimumlarını karşılaştırır. Eşitlik dönüşü `825006D0`, diğer dönüş `825006E4`, sonraki pdata fonksiyonu `825006E8` adresindedir. Altı guest-memory vakasında boş listeler, küçük/büyük/eşit sonuçlar ve çok düğümlü listeler kontrol edildi; 6/6 geçti. Bu rutin stub ile değiştirilmez.

Bu düzeltmeyle gerçek CAPCOM açılış görüntüsü üretildi. Sonraki eksik çağrı `825F9BE0`, Bink kilit bırakma callback'iydi: embedded lock vtable'ının +20 slotuna altı komutla tail-call yapar; `825F9BF4` son bctr, `825F9BF8` sonraki pdata fonksiyonudur. Aynı video/stream arayüzlerinin `.rdata` tablolarında 21 ek this-adjustment thunk doğrulandı. Her biri tam olarak `addi r3,r3,offset; b registered_target` biçiminde sekiz byte'tır; hedeflerin tamamı mevcut üretilmiş tabloda kayıtlıdır.

Linux grafik ortamı Xvfb 1280×720 ve Mesa 25.0.7 lavapipe Vulkan sürücüsüdür; fiziksel GPU gerekmez. Resmi SDK SDL ses sürücüleri pipewire/pulseaudio/alsa içerir, dummy içermez. ALSA null sink ile gerçek SDL ses backend'i ve 6 kanal/48000 Hz endpoint kurulmuştur; duyulabilir ses kalitesi test edilmez. Klavye kontrolü `--mnk_mode=true` ile SDK'nın normal input sisteminden gelir (Enter=Start, Space=A). GDB kullanılırsa SDK'nın guest bellek/MMIO SIGSEGV handler'ına sinyal iletilmelidir; ilk SIGSEGV'de debugger'ın durması tek başına oyun çöküşü değildir.


Bink düzeltmelerinin ardından açılış videosu ve MotoGP başlık görüntüsü elde edildi (gerçek Xvfb kareleri `docs/screenshots/` altında). Menü hazırlığında `826543E8` arayüz getter'ı eksikti. Rutin null output pointer için 37 döndürür; geçerli output pointer'da nesnenin +28 alanını yazıp 0 döndürür. İki return yolu dahil `[826543E8,82654408)` sınırı doğrulandı. Önceki keşif, ikinci yolu `826543F8` olarak ayrı kaydetmişti. Aynı `.rdata` arayüz tablolarındaki 13 eksik leaf/wrapper birlikte disassemble edildi; manifest gerçek son dönüşü/tail branch'i içerir, padding içermez. Diğer koşullu rutinlerde de keşfin ayırdığı suffix'ler gerçek fonksiyon kapsamına dahil edildi (`8258CA20..8258CA40`, `825FE308..825FE33C`).

Bu analiz her code-looking sayısal değeri fonksiyon kabul etmez: `.rdata` exception-handler metadata'sındaki iç etiketler normal vtable çağrısı olarak eklenmedi. `.data` içindeki `82848684` görünümü ise artan packed 16-bit veri tablosundadır ve `.text` hedefi padding'dir; false positive olarak dışarıda bırakıldı. SDK ve oyun algoritmaları değiştirilmedi; yalnızca 62 doğrulanmış fonksiyon sınırı bildirildi.


## Gerçek Linux ana menüsü: doğrulama sonucu

62 sınır içeren güncel manifest ve tam disk verileriyle Linux Release uygulaması ana menüye ulaştı. İlk profil/bilgilendirme ve zorluk ekranları geçildi; ardından modal olmadan Championship, Career, Arcade, Time Trial, Multiplayer ve Options başlıkları çizildi. Gerçek menü karesi `docs/screenshots/linux-main-menu.png` dosyasındadır. Menü en az iki dakika gözlendi; güncel çalıştırmanın logunda fatal/unregistered-call bulunmadı. Yaklaşık dokuz dakikalık açılış/gözlem denemesi sonrasında süreç bilinçli SIGTERM ile durduruldu; bu nedenle çıkış kodu bir oyun başarı testi olarak sunulmaz.

Linux native regresyon probu 47/47 davranış kontrolünü geçti (10 matematik, 5 callback, 24 initializer, 6 karşılaştırıcı, 2 getter); 7144 CRT initializer hedefinin tamamı kayıtlıdır. SDK dosya sistemi logunda `XFileXctdCompressionInformation` gibi desteklenmeyen sorgular hâlâ görülebilir, ancak bu çalıştırmada menüye ulaşmayı engellemedi. Görsel doğruluk, tam menü gezinmesi, yarış, duyulabilir ses kalitesi ve güncel Windows GPU davranışı ayrı doğrulama gerektirir. Bu kabul sonucu yalnızca kullanıcının istediği Linux'ta çökmeden ana menüye ulaşma düzeyini kapsar.


Güncel kaynakların Windows x64 uygulaması ve headless probe'u tam hedef derlemesiyle yeniden üretildi; uygulama/probe PE AMD64 olarak doğrulandı. Windows probe gerçek 0.0.1.1 XEX'i Wine altında yükledi, Linux ile aynı 47 davranış kontrolünü ve 7144 initializer kaydını geçti (exit 0). Güncel EXE SHA-256: `b6d8c3462515b37ff01f0a6f28396ec24c05a106ea3304bf39d919246b6ffd51`. Hazır paket EXE, eşleşen resmi SDK/CRT DLL'leri, klavye başlatıcısı, notlar ve checksum'ları içerir; oyun/TU/patch içermez. CRC, paket üyelerinin tüm SHA-256 değerleri ve paketteki EXE'nin derleme çıktısıyla aynı olması kontrol edildi. Bu, gerçek Windows GPU menü/yarış doğrulamasının yerini almaz.


## Yarışa geçişteki Windows çöküşü: `0x82271650`

Kullanıcının 13 Ekim Windows logunda Direct3D 12, input ve 0.0.1.1 patch başlatması tamamlandı. Kullanıcı ana menü, motorcu/pist seçimi ve Losail Time Trial hazırlık ekranını doğruladı; Start seçilince logun son fatal satırı `Call to invalid or unregistered function at guest address 0x82271650` oldu.

TU `.text` disassembly, `[82271650,8227168C)` aralığında 15 komutluk prologue içermeyen bir wrapper gösterdi. Üç parametre nesnesinin +12 alanındaki float pointer'larını (`82917A44`, `82917A68`, `82917A8C`) yükler, gelen r7'yi r10'a taşır, r6=8 seçer ve `82271688` adresindeki terminal branch ile mevcut `82270FC8` yardımcısına gider. `8227168C` sıfır padding'dir; `82271690` ayrı kısa wrapper, `82271698` sonraki pdata fonksiyonudur. `827A2D38` initializer'ı `lis r8,0x8227; addi r11,r8,0x1650` ile bu callback'i nesnenin 8068, 10244 ve 12420 offsetlerine yazar. Adresin parçalı oluşturulması ve standart prologue olmaması keşfin bunu atlamasını açıklar; `.rdata` statik pointer tablosunda bulunmaz.

Manifest sınırı eklendi; gerçek 15 komut ve native kayıt SDK tarafından üretildi. Fonksiyon, ortak yardımcı, oyun parametreleri ve GPU ayarları değiştirilmedi. Windows uygulaması ve PE probu artımlı derlendi. Regresyon, dormant vehicle guest-memory fixture'ında gerçek indirect callback ve ortak yardımcısını yürütür. Üç farklı işaretli yön değerinde yardımcının clamp sonucu, r6 seçimi, float parametre yükleri, stack/LR/nonvolatile dönüşü ve dormant output'un korunması kontrol edilir. Üç geçici parametre pointer'ı ve ayrılan bellek test sonrasında geri yüklenir. Bu, tam yarış simülasyonu değildir.

Windows PE probu Wine altında 50/50 davranış kontrolünü ve 7144 initializer hedefini doğruladı (exit 0). Güncel Windows race-start-fix paketi orijinal XEX/TU/patch/oyun verisini içermez. CRC, üyelerin SHA-256 değerleri ve EXE'nin derleme çıktısıyla aynı olduğu kontrol edildi. Kullanıcının isteğine uygun olarak gerçek yarış yeniden oynatılmadı; yeni Windows paketinin yarış testi kullanıcı cihazına bırakıldı.


## İlk ayrı teslimat: game/ veri düzeni

Kullanıcı işleri tek tek görmek istediği için bu teslimatta yalnızca dosya düzeni değişti; launcher, genel kalite kontrol aşaması ve LAN geliştirmesine başlanmadı. `OnConfigurePaths` açık `game_data_root` değerini korur, varsayılan seçimde önce EXE yanındaki `game/default.xex`, ardından eski düz yerleşimi arar. Paket, veri yerleştirme notuyla `game/` klasörü içerir; orijinal oyun dosyalarını taşımaz veya dağıtmaz. SDK varsayılan kayıt/cache konumu ve mevcut yarış başlangıcı düzeltmesi korunur.

Güncel Windows EXE gerçek Wine çalıştırmalarıyla farklı çalışma dizininden denendi. Türkçe karakter/boşluk içeren program klasöründe her iki XEX adayı varken TU yalnızca game/ içindeydi: doğru patch game/default.xexp altında oluşturuldu, ana dizine yazılmadı. Patch SHA-256 beklenen değerle eşleşti. İkinci çalıştırmada mevcut patch byte'ları ve mtime değişmedi. Ayrı denemeler eski düz yerleşim fallback'ini ve açık oyun yolunun game/ adayına göre önceliğini doğruladı; stok XEX hash'leri değişmedi.

Bu GUI yol denemeleri patch hazırlığından sonra bu host'un Direct3D 12 sunumu olmadığı için beklenen exit 3 ile durdu; yeni Windows grafik/yarış çalıştırması olarak sunulmaz. Ayrı Windows PE probu game/ içindeki gerçek XEX/patch'i yükledi ve 50 davranış kontrolünü geçti (exit 0). Paket CRC, üyelerin SHA-256 değerleri, game/ notu ve paketteki EXE'nin güncel derleme çıktısıyla eşleşmesi doğrulandı.


## Launcher öncesi ayrı teslimat: doğrudan EXE açılışı

Kullanıcı game/ düzeninin sorunsuz çalıştığını bildirdi ve CMD yerine EXE açılışı istedi. OnPreSetup giriş sistemi kurulmadan önce, yalnızca kaynağı SDK varsayılanı olan mnk_mode değerini true yapar. Açık config/env/CLI/runtime tercihleri ve mevcut backend/tuş eşlemeleri korunur. SDK ve guest oyun kodu değiştirilmedi. Windows EXE artımlı derlendi. Gerçek SDK ile Windows PE giriş probunda beş öncelik senaryosu ve her senaryoda runtime false tercihinin korunması geçti (5/5, exit 0). Grafik veya fiziksel gamepad yeniden denenmedi. Paket CMD içermez; yeni yönerge dosyaları readme.txt ve game/readme.txt adlarıyla İngilizce adlandırıldı. CRC, üye hashleri ve EXE/derleme eşleşmesi kontrol edildi. Launcher geliştirmesine başlanmadı.


## Kullanıcının controller veritabanını paketleme

ReXGlue v0.10.0 SDL sürücüsünde varsayılan hid_mappings_file=gamecontrollerdb.txt ve gerçek SDL_AddGamepadMappingsFromFile yükleme yolu doğrulandı. Kullanıcı dosyası değiştirilmeden third_party altında saklandı; upstream kaynak/zlib lisansı korundu. Paketleme betiği bundan sonraki Windows ZIPlerine dosyayı EXE yanında, lisansı licenses altında ekler. Linux gerçek SDK SDL API yüklemesi 472 yeni eşleme aldı. Aynı resmi Windows SDK SDL static library ile Windows PE probu Wine altında platform=Windows, 569 yeni eşleme ve örnek GUID/düğme içeriğini doğruladı (exit 0). Fiziksel kol denenmedi. XInput backend bu dosyayı kullanmaz; SDK varsayılan göreli yolu çalışma dizininden arar. EXE/DLL kodu değiştirilmedi. 14 dosyalı paket CRC/üye hashleri ve yüklenen dosyayla byte eşleşmesi kontrol edildi. Launcher henüz uygulanmadı.


## Ayrı teslimat: İngilizce/Türkçe Windows launcher

Varsayılan dil İngilizce; Language / Dil menüsünden Türkçe seçilir ve ayrı motogp0910-launcher.toml dosyasında saklanır. Native Win32 launcher mevcut motogp0910.toml değerlerini toml++ ile okur; yönetilmeyen değerleri korur, ilk dosyanın byte yedeğini alır, dış değişikliği denetler ve atomik kaydeder. Seçilen game root ve yönetilen SDK seçenekleri CreateProcessW ile iletilir; çalışma dizini EXE klasörüdür. Özel kayıt/cache yolları korunur. SDK/game kodu değiştirilmedi; oyun EXE'si eski direct-launch paketindekiyle aynıdır. Controller veritabanı ve upstream lisansları paketlenir.

Windows PE launcher probunda Wine altında 46 kontrol geçti (exit 0), gerçek SDK flag değerleri ve Unicode/boşluk içeren yolda çocuk süreç argümanları/CWD doğrulandı. Gerçek GUI'de iki dil, ayar kaydı/yeniden yükleme, varsayılanlar, klasör seçimi ve üç logun 2 MiB kuyruklarıyla rapor üretimi denendi. İlk klasör seçici hatası yerel Wine kurulumunda eksik winspool.drv ve tamamlanmamış COM/profil kayıtlarından kaynaklandı; driver erişimi ve wineboot -u ile giderildi. Tam dump'taki nxeart boş olduğundan ön kontrol bunu hata saymaz; xenondata dolu olmalı. Gerçek kullanıcı kolu/Windows GPU oynanışı yeniden denenmedi. Genel QA/LAN aşamasına başlanmadı.
