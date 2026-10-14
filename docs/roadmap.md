# MotoGP 09/10 geliştirme notları

## Güncel arayüz işleri — 14 Ekim 2026

Kullanıcının güncel sırası aşağıdaki gibidir. Her iş ayrı teslim edilir;
önceki iş gösterilmeden sonraki iş başlatılmaz.

1. **Klavye tuş simgeleri:** koyu tuş kapakları, turuncu MotoGP detayları ve
   daha okunaklı yazı. Kullanıcı bu görünümü seçti; [uygulama ve Windows testleri](keyboard-prompts.md) tamamlandı. Kullanıcının görsel testi bekleniyor.
2. **Oyun içinden canlı ayarlar:** F4 menüsüne benzer veya mevcut menü içinden
   launcher ayarlarına erişim. Canlı değişebilen ayarlar ile yeniden başlatma
   gerektirenler doğru ayrılacak; uygulama henüz başlamadı.
3. **Achievement arayüzü:** F7 listesi ve sağ alttaki bildirimler; sade,
   MotoGP renk temasına uygun tasarım. Henüz başlamadı.
4. **Launcher Controls:** daha basit düzen. Bu aşamaya gelince kullanıcıya
   tasarım ve kapsam detayları sorulacak; henüz başlamadı.
5. **Launcher genel tasarımı:** en son yapılacak. Detaylar kullanıcıdan
   alınacak; henüz başlamadı.

FPS/fizik incelemesi kullanıcı tarafından iptal edildi; o incelemenin
tanılama/test kodları geri alındı. LAN/lobi düzeltmeleri korundu. Kullanıcı
iki oyunculu Radmin yarışının çalıştığını bildirdi; son lobi giriş düzeltmesi
ayrı build olarak teslim edildi.

13 Ekim 2026 kullanıcı talepleri. Ana platform **Windows 10/11**, SDK
**ReXGlue Nightly 0.10.0.24-dev.gbd833a2**. Kullanıcı yarış başlangıcı düzeltmesinin ardından oyunun
düzgün çalıştığını bildirdi. Bu, tüm modların/pistlerin bağımsız kalite kontrolünün
tamamlandığı anlamına gelmez.

İşler kullanıcıya tek tek teslim edilir; önceki iş gösterilmeden sonrakine geçilmez. `game/` dosya düzeni bu teslimatta tamamlandı; [kullanım ve test notları](game-layout.md). İngilizce/Türkçe launcher ilk sürümü ayrı aşamada uygulandı; [teslimat ve testler](launcher.md). LAN keşif/oturum altyapısından sonra [deneysel XNet/WSA ve launcher entegrasyonu](lan-multiplayer-test.md) eklendi; yeni testler kullanıcının isteğiyle atlandı, gerçek Radmin/yarış doğrulaması bekliyor.

| İstek | Durum / uygulanabilirlik | Öncelik |
|---|---|---|
| EXE yanında `game/` veri klasörü | Tamamlandı: game önceliği, paket klasörü ve Windows yol/TU kontrolleri. | 1 |
| Windows launcher | Genişletildi: İngilizce/Türkçe, grafik/GPU/MSAA/FSR 1/CAS/FXAA, gerçek ses düzeyi, 25 düzenlenebilir klavye eşlemesi; test ve paket mevcut. | 1 |
| Cihaza göre oyun içi tuş simgeleri | Native çizim ve otomatik giriş seçimi uygulandı. [Kapsam ve testler](controller-prompts.md). | Yerel Windows görsel testi |
| Stabilite ve kalite kontrol | Runtime bellek/thread/yol düzeltmeleri ve Windows CPU/UI regresyonu tamamlandı. [Kapsam](stability-quality-control.md). Tam pist/mod GPU taraması bekliyor. | Her aşama |
| Xbox Live multiplayer akışının LAN üzerinden çalışması | [İlk keşif/oturum altyapısı](lan-lobby-preview.md) ve iki Windows süreciyle test hazır. XNet/WSA oyun paket entegrasyonu ve launcher LAN ayarları eklendi; [yeni build](lan-multiplayer-test.md) gerçek iki-PC yarış doğrulaması bekliyor. | 3 |

## Doğrudan EXE açılışı

Launcher öncesindeki ayrı teslimat: klavye emülasyonu yalnızca kullanıcı bir
`mnk_mode` tercihi belirtmemişse EXE tarafından açılır. TOML, ortam değişkeni
ve komut satırındaki açık tercihler korunur. Yeni paket CMD içermez;
yönergeler `readme.txt` ve `game/readme.txt` adlarıyla dağıtılır. Bundan sonraki
üretilen dosya adları İngilizce olacak.

## Launcher

İlk sürüm: [kullanım ve doğrulama](launcher.md). Dosya adı `motogp0910-launcher.exe`;
varsayılan dil İngilizce, General > Launcher language menüsünden Türkçe seçilir. Controller
veritabanı EXE yanında paketlenir. Launcher teslim edilmeden sonraki genel kalite
kontrol veya LAN aşamasına geçilmez.


Hedef: Windows üzerinde ayarları kaydeden, oyun dosyalarını denetleyen ve oyunu
başlatan ayrı bir uygulama. Ayarlar için SDK'nın mevcut CLI/TOML yapılandırmasını
kullanmak mümkün; örneğin çözünürlük, pencere/tam ekran, VSync, render ölçeği,
SDL/XInput seçimi, klavye eşlemesi ve sesi kapatma seçenekleri mevcut. Her seçenek
ilgili backend üzerinde doğrulanmalıdır. Yeni grafik/FPS seçenekleri ayrıca
oyun hızı ve fizik üzerindeki etkileriyle sınanmalıdır.

İlk sürüm hedefleri:

- Oyun klasörünü bulma/seçme; XEX, veri dizinleri ve gerekli title update kontrolü.
- Ayarları saklama, tekrar açıldığında geri yükleme ve varsayılanlara dönüş.
- Oyunu başlatma ve mevcut kullanıcı kayıt/cache konumunu koruma.
- Log klasörüne erişim ve hata raporu hazırlama.
- Varsayılan İngilizce ve seçilebilir Türkçe arayüz, EXE ile birlikte dağıtım; son kullanıcıda derleme gerektirmeme.

Kabul ölçütü: boşluk/Türkçe karakter içeren yollardan oyun açılır, ayarlar kalıcı
olur, mevcut kayıtlar korunur, eksik veri durumunda anlaşılır hata gösterilir.

## Oyun dosya düzeni

Mevcut `Motogp0910App::OnConfigurePaths`, açık bir `game_data_root` verilmemişse
önce EXE yanındaki `game/` içinde, ardından eski düz yerleşimde `default.xex` arar.
Bu nedenle mevcut race-start-fix EXE ile de aşağıdaki düzen kullanılabilir:

```text
MotoGP0910/
├── motogp0910-launcher.exe
├── motogp0910.exe
├── gamecontrollerdb.txt
├── rexruntime.dll
├── rexgpu-xenos.dll
├── diğer paket DLL'leri
├── readme.txt
└── game/
    ├── default.xex
    ├── default.xexp
    ├── TU_11K61VG_0000004000000.0000000000081
    ├── xenondata/
    ├── nxeart/
    └── diğer orijinal disk dosyaları/dizinleri
```

Tüm orijinal oyun dosyaları ve patch birlikte `game/` altında bulunmalı;
EXE/DLL'ler üst dizinde kalır. Her iki yerde XEX varsa yeni EXE `game/` klasörünü seçer. `--game_data_root` veya `run-windows.ps1 -GameRoot`
kullanıldığında yol yeni `game/` dizinine işaret etmelidir. Kayıt ve cache
klasörleri oyun verisinden ayrı korunmalıdır; taşımaya gerek yoktur.

Paket ve [taşıma rehberi](game-layout.md) bu düzeni standartlaştırır. Launcher ayrı aşamada teslim edilir; [ilk sürüm](launcher.md) hazırdır. Kabul ölçütü:
farklı çalışma dizininden çift tıklamayla açılış, title-update yükleme ve kayıt
koruma doğrulanır; eski düzende açılış da desteklenir.

## LAN multiplayer

14 Ekim 2026: ilk hedef **Windows + Radmin VPN, aynı sanal ağda otomatik oda
keşfi ve host/join; yedek seçenek olarak IP ile bağlanma**. Ayrı merkezi sunucu zorunluluğu yok. [Kaynak incelemesi ve uygulama
rotası](lan-research.md) tamamlandı; DBZ Burst Limit'in Radmin/LAN yolu ve diğer
ReXGlue ağ katmanları incelendi. [Keşif/guest oturum köprüsü](lan-lobby-preview.md)
ilk aşaması uygulandı. Oynanabilir LAN yarışı ve oyun içi lobby UI testi henüz tamamlanmadı.

Kullanıcının tercihi: **aynı yerel ağda, ayrı bir merkezi/dedicated sunucu
zorunluluğu olmadan** oynama. Bir oyuncunun oda açması ve diğerlerinin ona
bağlanması hedeflenir. Uzak internet üzerinden bağlantı ayrı bir aşamadır;
LAN çalışması tek başına NAT veya internet eşleştirme desteğini kanıtlamaz.

İlk incelemede oyunun üretilmiş kodunda socket, XNet bağlantı/adres dönüşümü ve
XAM oturum çağrıları bulundu. Pinned Nightly SDK'da temel socket altyapısı mevcut,
ancak örneğin `XNetConnect` ve bazı key/QoS işlevleri stub; XNet adres dönüşümleri
başarısız dönüyor ve oturum oluşturma/arama/join akışlarının bazıları gerçek
oturum verisi üretmeden başarı döndürüyor. LAN için yalnızca launcher'a bir
seçenek eklemek yeterli değildir.

Araştırma sırası:

1. Oyundaki multiplayer ekranlarının Live oturumu ve varsa System Link akışını
   incele; yerleşik System Link desteğini henüz doğrulanmış kabul etme.
2. Gerekli profil, oyuncu kimliği, oturum, adres ve paket çağrılarını belirle.
3. Otomatik oda listesiyle iki oyun örneğinin lobby bağlantısını dene; keşif
   çalışmadığında aynı bağlantı akışını doğrudan host IP'siyle doğrula.
4. Yarış başlatma, oyuncu durumu senkronizasyonu, bağlantı kopması ve yeniden
   bağlanmayı iki ayrı Windows cihazında sınayarak uygulanabilirliği doğrula.

Bu hedef orijinal Xbox Live hizmetine bağlanmak değil, oyunun multiplayer
akışını yerel ağ backend'iyle çalıştırmaktır. Orijinal Live matchmaking,
leaderboard ve arkadaş/davet özellikleri bu ilk hedefe dahil değildir.
Dedicated sunucu ancak araştırma bunu gerekli gösterirse ayrı bir tasarım
seçeneği olarak değerlendirilir; şu anda kullanıcı tercihi değildir.

## Stabilite ve kalite kontrol

İşler tekrar üretilebilen hatalar ve etkilerine göre takip edilir:

- Önce çökme, takılma, kayıt bozulması ve eksik fonksiyon/kernel davranışları.
- Ardından tüm yarış modları, motor sınıfları, pistler, AI, yeniden başlatma,
  menüye dönme ve uzun oyun oturumları.
- Kayıt/profil: oluşturma, yeniden yükleme, ayarların ve kariyer ilerlemesinin
  korunması; paket güncellemesi ve `game/` düzenine geçiş.
- Grafik: hatalı/eksik dokular, ışık, gölge, HUD, shader/texture-fetch uyarıları
  ve farklı Windows GPU'larında çizim.
- Ses: kanal dengesi, müzik/motor sesleri ve senkronizasyon.
- Giriş: SDL/XInput gamepad, klavye eşlemesi, cihaz takıp çıkarma, odak kaybı.
- Performans: frame süreleri, yükleme, shader cache ve bellek kullanımı.

Her düzeltmenin kapsamı, tekrar üretme adımları, test sonucu ve kalan sorunları
kaydedilir. Gerçek Windows yarış denemeleri kullanıcı cihazından gelen log ve
gözlemlerle desteklenir; headless probe tek başına oynanış doğrulaması sayılmaz.

## Title update durumu

**Aktif: kullanıcının verdiği TU, stok 0.0.0.1 oyununu 0.0.1.1'e güncelliyor.**
Windows logunda `XEX patch applied successfully: base version: 0.0.0.1,
new version: 0.0.1.1` satırı bulunuyor. Uygulama bu sürümü kontrol edip diğer
sürümlerde başlatmayı reddediyor. Patch çalışma sırasında uygulanır; orijinal
stok XEX dosyası değiştirilmez. Bu bilgi, verilen TU'nun uygulandığını gösterir;
en son yayımlanmış TU olduğu ayrıca araştırılmadı.
