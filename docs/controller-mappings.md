# Gamepad eşleme veritabanı

ReXGlue v0.10.0 SDL giriş backend'i `hid_mappings_file` ayarını kullanır.
Varsayılan değer `gamecontrollerdb.txt` olduğundan gönderilen dosya aynı adla
EXE yanında paketlenir. `game/` içine konmaz. SDK dosyayı
`SDL_AddGamepadMappingsFromFile` ile yükler; başarılı yükleme logda
`SDL GameControllerDB: loaded ... mappings.` satırıyla gösterilir.

Bu dosya, kolun GUID kimliği için düğme/eksen eşlemeleri sağlar. SDL'nin zaten
tanıdığı bir kol dosya olmadan da çalışabilir. Kullanıcıya ait belirli bir
kolun bağlantısını ve bütün düğmelerini bu bulut makinesinde doğrulamak mümkün
değildir; dosyanın yüklenmesi bütün kolların doğru çalışacağı anlamına gelmez.
`input_backend=xinput` kullanılırsa SDL eşleme dosyası kullanılmaz.

SDK v0.10.0 bu varsayılan dosya adını çalışma dizinine göre arar. Normalde
program klasöründen açılır. Kısayoldan başlatıyorsan **Start in / Başlangıç**
alanını EXE klasörüne ayarla. Özel `hid_mappings_file` ayarın varsa bu tercih
korunur; gerekirse dosyanın tam yolunu verebilirsin. Launcher aşamasında oyun
işleminin çalışma dizini program klasörü olarak seçilmelidir.

Gönderilen dosya değiştirilmeden `third_party/SDL_GameControllerDB/` altında
saklandı. 2241 girişin 860'ı Windows içindir. SHA-256:
`e4e5cb4b96e36b439531c5fdfd648b56e69d579844cb970e680a40d9b314b786`.
Upstream kaynak ve zlib lisansı birlikte korunur. Paketleme betiği her Windows
paketine dosyayı ve `licenses/SDL_GameControllerDB-LICENSE.txt` lisansını ekler.

Linux SDK'nın gerçek SDL API'siyle içe aktarım başarıyla tamamlandı (472 yeni
eşleme). Bu sayı platform filtresinden ve mevcut/tekrarlanan eşlemelerden
etkilenir; dosyadaki toplam satır sayısıyla aynı olması gerekmez.

Windows SDK ile verilen aynı SDL static library kullanılarak oluşturulan Windows
PE probu Wine altında paket klasöründeki dosyayı yükledi: platform Windows,
569 yeni eşleme, exit 0. SDK registry varsayılan dosya adı ve bir örnek Windows
GUID eşlemesinin ad/düğme içeriği doğrulandı. Bu, fiziksel kol testi değildir.
Paket CRC ve üye SHA-256 kontrollerinden geçti; eklenen veritabanı yüklenen
dosyayla byte olarak aynıdır. Oyun EXE/DLL kodu bu teslimatta değişmedi.
