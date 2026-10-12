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
