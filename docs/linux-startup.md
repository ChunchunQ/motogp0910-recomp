# Linux bulutunda gerçek oyun açılışını yeniden deneme

Nihai hedef Windows 10/11'dir. Bu Linux denemesi tam disk verileriyle gerçek
oyunun ana menüye ulaşmasını kontrol etmek içindir. Headless probe'un başarılı
olması veya süreçte fatal hata görülmemesi, menü doğrulaması sayılmaz.

Bulut bağımlılıkları `scripts/install-cloud.sh` ile root gerektirmeden hazırlanır.
ReXGlue sürümü **0.10.0** olarak sabittir. Xvfb sanal ekran, Mesa lavapipe Vulkan
sürücüsü sağlar; oyun görüntüsü CPU üzerinde çizilir. ALSA null sink gerçek SDL
ses backend'ini açar, fakat duyulabilir ses kalitesi sınanmaz.

Önce README'deki Linux derlemesini tamamla. `assets/game-tu/default.xex` ve
`default.xexp` kod üretiminin girdileridir; çalışma için tam disk klasörünün
orijinal dizin yapısı da gerekir. `xenondata/`, `nxeart/` ve `default.xex` aynı
oyun kökünün altında kalmalıdır. Oyun verisi depoda bulunmaz.

```bash
bash scripts/run-linux-cloud.sh '/tam/yol/MotoGP 09-10'
```

Betik boş bir X ekranı seçer ve ekran numarasıyla log klasörünü yazar. Örneğin
`Display: :0` çıktısı için ayrı terminalde:

```bash
source scripts/cloud-env.sh
import -display :0 -window root out/linux-startup/frame.png
DISPLAY=:0 xdotool search --name motogp0910 windowfocus keydown Return
# Tuşu birkaç frame boyunca tut, sonra bırak:
DISPLAY=:0 xdotool keyup Return
```

SDK'nın normal klavye/controller emülasyonu `--mnk_mode=true` ile açıktır:
**Enter=Start**, **Space=A**, **Backspace=B**, **W/S/A/D=sol stick**,
**Shift+oklar=D-pad**. Menü geçişlerinde tuşları basılı tutup sonra bırakmak,
yazılım Vulkan sürücüsünün yavaş frame'lerinde kısa basışların kaçmasını önler.
Ekran görüntüsünü inceleyerek gerçek menüyü doğrula; yalnızca siyah ekran veya
CAPCOM logosu yeterli değildir.

Betik ayrı kullanıcı verisini `out/linux-game-userdata/` altında tutar. Loglar
her çalıştırmada yeni `out/linux-startup/run-*` klasörüne gider; oyun kapanınca
yalnızca betiğin kendi Xvfb süreci durdurulur. XEX/oyun veri dosyaları değiştirilmez;
SDK çalışma sırasında sibling title-update patch'ini uygular. `default.xexp`
yoksa uygulamanın mevcut TU çıkarma yolu kullanılabilir.

GDB ile incelemede `handle SIGSEGV nostop noprint pass` kullan: SDK guest
bellek/MMIO için SIGSEGV handler'ı kurar. Debugger'ın bu sinyalde ilk duruşunu
kendiliğinden bir oyun çöküşü olarak yorumlama. Gerçek abort/unregistered-call
hataları ve ana menüye ulaşma durumu ayrı kontrol edilmelidir.
