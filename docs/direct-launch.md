# Doğrudan EXE ile açılış

Yeni Windows paketi: `MotoGP0910-Windows-x64-direct-launch.zip`.
ZIP'i mevcut program klasörüne çıkarıp EXE/DLL'leri güncelle. Oyun dosyaları
`game/` altında kalır. `motogp0910.exe`ye çift tıkla; klavye kontrolü için
CMD veya komut satırı parametresi gerekmez. Yeni paketteki yönerge dosyaları
`readme.txt` ve `game/readme.txt` adlarıyla gelir. Önceki paketten kalan CMD
ve Türkçe yönerge dosyalarını kullanman gerekmez; istersen silebilirsin.

Enter=Start, Space=A, Backspace=B; W/S/A/D=sol stick, Shift+oklar=D-pad.
SDK'nın gamepad backend'i ve mevcut tuş eşlemeleri aynı şekilde kullanılır.
Bu teslimat yalnızca doğrudan başlatma davranışını düzenler; launcher ayrı iştir.

## Kullanıcı tercihini koruma

Uygulama, SDK yapılandırması yüklendikten sonra ve giriş sistemi kurulmadan
önce `mnk_mode` kaynağını kontrol eder. Değer hâlâ SDK varsayılanıysa klavye
emülasyonunu açar. Açık TOML, ortam değişkeni, CLI veya runtime tercihini
değiştirmez. EXE yanındaki mevcut `motogp0910.toml` içinde `mnk_mode = false`
varsa, klavyeyi açmak için bu değeri `true` yap veya ilgili satırı kaldır.
Gamepad kullanan ve klavyeyi kapatmak isteyen kullanıcı `false` bırakabilir.

## Doğrulama

ReXGlue v0.10.0 ile Windows x64 uygulaması derlendi. İsteğe bağlı
`input_defaults_probe` hedefi gerçek SDK registry/config/env/CLI kodunu ve
uygulamanın aynı giriş başlangıç fonksiyonunu kullanır; yeni bir flag tanımı
veya taklit parser içermez. Windows PE çıktısı Wine altında şu kontrolleri geçti:

| Senaryo | Beklenen sonuç |
|---|---|
| Parametresiz, özel ayarsız açılış | Klavye açık |
| TOML `mnk_mode=false`, özel backend ve tuş eşlemesi | Klavye kapalı, backend/eşleme korunur |
| Ortam değişkeni false, TOML true | Klavye kapalı |
| CLI false, ortam değişkeni/TOML true | Klavye kapalı |
| CLI true, ortam değişkeni/TOML false | Klavye açık |

Her senaryo ayrıca sonradan verilen runtime false tercihinin korunmasını
kontrol etti. **5/5 senaryo, exit 0.** Bu kontroller grafik ve fiziksel gamepad
etkileşimi çalıştırmaz. Bu bulut makinesinde Windows Direct3D 12 oynanışı
yeniden test edilmedi; kullanıcı önceki paketin düzgün çalıştığını bildirdi.

Paket CRC'si, her üyenin SHA-256 değeri, EXE'nin derleme çıktısıyla eşleşmesi,
CMD içermemesi ve İngilizce dosya adları ayrıca kontrol edilir. Orijinal oyun
dosyaları/TU/patch dağıtım paketine dahil edilmez.
