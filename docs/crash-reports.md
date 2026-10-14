# Windows crash reports

Crash capture is included with the LAN launcher startup fix. The original
empty-IP launcher/SDK argument bug is documented in [lan-startup-fix.md](lan-startup-fix.md).

## Kullanım

- ZIP'in **tamamını** çıkar. `motogp0910-crash-monitor.exe` diğer EXE ve DLL'lerin
  yanında kalmalı. Mevcut oyun dosyalarını, ayarlarını ve kayıtlarını koru.
- Launcher **General > Extended crash diagnostics / Gelişmiş çökme tanılaması**
  varsayılan olarak açık. Oyunu launcher'ın **PLAY** düğmesiyle başlat.
- Çökerse **Open logs** düğmesine bas. `crash-...` klasörünün içinde `report.txt`,
  başarılıysa `crash.dmp`, son oyun logu ve mevcut ayar dosyaları bulunur.
  Bu klasörü ZIP yapıp gönder. **Error report** düğmesi de en son çökme klasörünü
  kendi rapor klasörüne `native-crash/` olarak ekler.
- İzleyici ayarını kapatırsan ek dış izleme devre dışı kalır. Doğrudan EXE'de
  temel hata yakalama devam eder; zorunlu kapanışlarda kapsamı daha sınırlıdır.
  İzleme debugger olaylarını kullanır ve ek yük oluşturabilir. Launcher'ı
  kapatmak oyunu kapatmaz; izleyici bağımsız çalışmaya devam eder.

Normal konum: `EXE-klasörü/logs/crash-.../`. Klasöre yazılamazsa yedek konum
`%LOCALAPPDATA%/MotoGP0910/crashes/`; son yedek Windows geçici klasörüdür.
İsimlerde UTC tarih/saat, süreç ve thread kimliği bulunur.

## Captured data and coverage

The text report records exception code/address/parameters, native thread
registers, module paths/base addresses, a native stack with module offsets,
SDK version and the image SHA-256. Where readable MSVC x64 standard-exception
RTTI is available, the exception type and message are included. A matching
`motogp0910.map` is shipped for resolving native game function addresses;
source line names are not guaranteed without appropriate local PDB symbols.

The external monitor launches the original game under the Windows debug API.
It lets the application's exception handlers receive first-chance exceptions;
only the initial debugger breakpoint is consumed. On an **unhandled second-chance
exception**, the game is stopped long enough to write the report and a standard
minidump, then normal fatal termination continues. Thus fast-fail
`0xC0000409` can be captured even when in-process exception filters are bypassed.
A nonzero exit without an exception produces an exit-status report; it does
not invent an exception address or a diagnosis. Normal zero exits and handled
exceptions produce no crash report.

Direct game EXE startup installs a native unhandled-exception filter and a C++
terminate handler before runtime/presentation setup. They offer best-effort
reports for ordinary native/C++ failures. Forced termination, early loader
failure, stack/heap corruption or an unwritable disk can bypass or defeat
in-process reporting. A failure that the GPU/game handles and exits normally
may appear only in the game log. Memory dumps include selected memory fragments,
configuration and local paths; this is diagnostic data, not a guarantee that
every crash can be completely diagnosed from one report.

## Validation

The production reporter/monitor were exercised in isolated Windows x64 fixture
processes under Wine. **9 scenarios passed.** Actual fatal instructions/exceptions
were raised, and
successful dump files were checked for the **MDMP header, valid stream ranges
and the actual native exception stream**, not merely filename existence.

Scenarios cover normal exit, an internally handled exception, access violation,
fast-fail with parameter `7`, unhandled C++, abnormal exit without an exception,
and direct EXE access violation/C++. The real launcher helper also starts a
monitored delayed child and exits, verifying that the child survives its parent.
Reports retain the faulting module, stack/register data, C++ message when
available, log and settings. Tests use small fixtures, never the user's game.

```powershell
python scripts/verify-crash-reports.py --build-dir out/build/win-amd64-release
```

Build optional `crash_report_probe` first. For cloud Wine validation, add
`--wine /path/to/wine64`. Native Windows GPU/Radmin racing remains a user test;
these diagnostic checks do not validate multiplayer synchronization.
