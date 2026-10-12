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
