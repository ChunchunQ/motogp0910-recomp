# Tek Windows PC ile LAN bağlantı denemesi

İki ayrı oyun süreci aynı PC'de birbirine bağlanabilir. Böylece oda arama,
katılma ve yarışa geçişi sınayabilirsin. Bu deneme, gerçek iki cihaz arasındaki
Radmin broadcast/routing veya güvenlik duvarı geçişini doğrulamaz.

1. Mevcut oyun pencerelerini kapat. Güncel ZIP'in tamamını EXE klasörüne çıkar;
   Kaynak depodaki `scripts/test-lan-one-pc.ps1` dosyasını da bu klasöre kopyala (temiz iki-PC ZIP paketinde bulunmaz). `game/` klasörünü koru.
2. EXE klasöründe PowerShell aç ve çalıştır:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\test-lan-one-pc.ps1
```

3. Açılan ilk oyun penceresinde **Online > Player Match > oda oluştur**.
4. `Alt+Tab` ile ikinciye geç; **Player Match > maç ara** akışından odaya katıl.
5. İki oyuncu listelenirse oda bağlantısı gerçekleşmiştir. İkisinde de **Ready**
   seç, yarışa geçmeyi dene. Yarış açılırsa diğer oyuncunun görünmesi ve hareket
   güncellemelerini de kontrol et; sadece aynı lobby'de görünmek yarışı doğrulamaz.
6. Takılırsa `logs/lan-host.log` ve `logs/lan-client.log` dosyalarını birlikte
   gönder. Çökme varsa ilgili `logs/crash-.../` klasörü de yardımcı olur.

Radmin bu yerel deneme için gerekli değil. İki süreç loopback (`127.0.0.1`)
üzerinden gerçek oda keşfi/join ve paket yolunu kullanır. Ayrı `lan-test/host`
ve `lan-test/client` kullanıcı verisi, cache ve metadata klasörleri kullanılır;
mevcut normal oyun kayıtları ve launcher ayarları değiştirilmez. Yeni test
profilleri için ilk açılış ekranlarını geçmen gerekebilir. Bir gamepad/klavye
ile odaklanan pencereyi kontrol etmek için `Alt+Tab` kullan.

İki oyun aynı GPU/RAM kaynaklarını paylaşır. Script pencereleri 960×540 yapar,
iç çözünürlüğü 1× tutar ve native MSAA'yı kapatır. İki örnekle yaşanan kaynak
baskısı ayrıca değerlendirilmelidir. Oyun içi bu denemeyi bulutta üretmedik;
gerçek GPU, lobby katılma ve yarış sonucu kullanıcının testiyle netleşecek.

Repo kaynağından çalıştırırken scriptin yanındaki klasör yerine EXE konumunu ver:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-lan-one-pc.ps1 -ProgramDirectory 'D:\your-build-folder'
```
