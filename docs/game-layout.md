# Oyun verilerini game klasörüne yerleştirme

Bu teslimat yalnızca dosya düzenini standartlaştırır. Windows launcher ayrı
bir sonraki iştir. ReXGlue sürümü **v0.10.0**, oyun sürümü **0.0.1.1**.

## Hedef düzen

```text
D:\x\motogp0910\
├── motogp0910.exe
├── rexruntime.dll
├── rexgpu-xenos.dll
├── diğer paket DLL'leri
├── readme.txt
└── game\
    ├── default.xex
    ├── default.xexp
    ├── TU_11K61VG_0000004000000.0000000000081
    ├── xenondata\
    ├── nxeart\
    └── diğer orijinal disk dosyaları/dizinleri
```

## Mevcut kurulumdan geçiş

1. Oyunu kapat. [Yeni Windows paketini](https://github.com/inflamesgo/motogp0910-recomp/blob/main/downloads/MotoGP0910-Windows-x64-direct-launch.zip)
   GitHub'da **Download raw file** ile indir ve program klasörüne çıkar.
2. Kendi oyununun **tüm orijinal disk dosyalarını**, `default.xex`, mevcut
   `default.xexp` ve TU paketiyle birlikte `game` altklasörüne taşı. `xenondata`
   ve `nxeart` içindeki dizin yapısını koru. EXE/DLL dosyaları üstte kalır.
3. Üst klasördeki `motogp0910.exe`ye çift tıkla. Klavye varsayılan olarak açıktır;
   `.cmd` gerekmez. Program doğru oyun dizinini kendisi bulur.

ZIP yalnızca program/DLL'leri ve `game` içindeki yerleştirme notunu içerir.
Oyun verileri, XEX, title update veya patch ZIP'te bulunmaz. Dosyaları otomatik
taşıyan bir işlem yapılmaz; mevcut oyun dosyaları kullanıcı tarafından taşınır.

## Seçim kuralları ve kayıtlar

- Açıkça verilen `--game_data_root` her zaman önceliklidir. Yeni düzen için bu
  parametre veya `run-windows.ps1 -GameRoot`, `D:\x\motogp0910\game` yolunu göstermeli.
- Parametre verilmediğinde önce EXE yanındaki `game/default.xex` aranır.
- Orada XEX yoksa eski düz yerleşim (EXE yanında `default.xex`) desteklenir.
- Her iki yerde XEX varsa yeni EXE `game` klasörünü seçer.
- EXE farklı bir çalışma dizininden başlatıldığında da kendi klasörünü esas alır.
- Kayıt/cache varsayılanı aynı SDK kullanıcı klasöründe kalır. Kayıtları `game`
  içine taşımak gerekmez. Özel `--user_data_root` kullanıyorsan aynı yolu koru.
  Programı başka bir klasöre taşıyorsan kendi `motogp0910.toml` ayar dosyanı da
  yeni EXE'nin yanına al; kendine ait ayar/kayıt dosyalarını silme.

Title update yüklemesi aynı şekilde devam eder: patch yoksa uygulama `game`
içindeki TU'dan `default.xexp` çıkarır; stok XEX'i ve mevcut patch'i değiştirmez.
Uygulama yalnızca 0.0.1.1 sürümünü kabul eder.
