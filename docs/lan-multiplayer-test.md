# Windows LAN multiplayer — experimental test build

14 October 2026. SDK: **0.10.0.24-dev.gbd833a2**. Game: TU **0.0.1.1**.
Protocol: **LAN02**; both PCs must use this exact package. The older LAN
foundation package is incompatible.

This build connects the original title's session and UDP/VDP socket imports to
our native LAN transport. It is ready for a first **user gameplay test**, not a
claim that synchronized racing has been verified. The initial LAN integration
delivery skipped new runtime/gameplay tests at the user's request. The subsequent [launcher startup fix](lan-startup-fix.md) passed
17 targeted real-SDK Windows/Wine checks. [Crash diagnostics](crash-reports.md)
passed 9 native process/fault scenarios. Windows EXEs compiled successfully;
packaging verifies matching runtime DLL hashes and ZIP integrity. This does not
validate two-PC racing or the rest of the new packet paths.

## Kullanım: iki Windows bilgisayarı

1. İki bilgisayara da aynı ZIP'in **tamamını** çıkar. Mevcut `game/` klasörünü,
   ayarlarını ve kayıtlarını koru. Eski EXE ile yeni DLL/EXE'leri karıştırma.
2. Radmin VPN'de aynı ağa katıl. İki PC de Radmin'de çevrimiçi görünmeli.
   Aynı ev ağı kullanılıyorsa Radmin zorunlu değil.
3. Her iki bilgisayarda launcher **General > Enable LAN (Experimental)**
   seçeneğini aç. Farklı oyuncu adları yaz. Basit ASCII ad kullanabilirsin;
   oyuncu adı en fazla 15, oda adı 31 UTF-8 bayt. `Player` varsayılanına
   süreç başına farklı bir dört haneli ek gelir.
4. **Local IPv4** ve **Host IPv4** alanlarını önce boş bırak.
   Otomatik seçim, etkin Radmin adaptörünü tercih eder. Başka adaptör
   seçilirse her bilgisayarda Local IPv4'e **kendi** Radmin IP'sini yaz.
   Ayarlar bir sonraki oyun açılışında uygulanır.
5. Windows güvenlik duvarı izin isterse `motogp0910.exe` için kullandığın
   ağda erişime izin ver. Keşif ve oyun paketleri **UDP 30810–30817**
   aralığını kullanır; internet modeminde port yönlendirme gerekmez.
6. Oyunu başlat. Oyun içindeki online/Xbox Live menüsünden **Player Match**
   (derecesiz maç) akışını kullan. Bir PC oda/maç oluştursun, diğeri oyun
   içindeki maç arama listesini açsın. Oda otomatik bulunursa katıl.
7. Oda bulunamazsa katılacak PC'de oyunu kapat; launcher General >
   **Host IPv4** alanına kurucunun **Radmin IPv4** adresini yaz, kaydet ve
   oyunu tekrar aç. Yeniden maç ara. Kurucuda Host IPv4 boş kalabilir.
   Aynı yerel ağda Radmin yerine kurucunun yerel IPv4 adresini kullan.
8. İlk denemede iki oyuncuyla başla. Odaya giriş, yarış başlangıcı,
   diğer oyuncunun hareketi ve yarış sonunu gözle. Bir oyuncu çıkınca tekrar
   katılmayı da deneyebilirsin. LAN'ı kapatmak için launcher'daki tiki kaldır.

Gerçek Xbox Live'a giriş, hesap/parola ve merkezi/dedicated sunucu gerekmez.
**Ranked**, Xbox Live arkadaşlar/davetler, leaderboards ve host migration
bu sürümün kapsamında değil. Ranked oturum oluşturma açıkça reddedilir.

## Sorun olursa gönderilecekler

Her iki PC'de oluşan en son oyun logunu (`logs/`), hangi PC'nin kurucu
olduğunu ve sorunun aşamasını gönder: menüye giriş / oda arama / katılma /
yarış başlatma / yarış sırasında / tekrar katılma. Otomatik aramanın mı,
yoksa Host IPv4 yedeğinin mi kullanıldığını da belirt.

`LAN_LOBBY` seçilen adaptörü/endpoint'i, `LAN_SESSION` oturum mesajlarını,
`LAN_PACKET` ilk oyun paketini gösterir. Bir socket gönderme denemesi logda
bulunması tek başına karşı tarafın aldığını veya yarışın senkron olduğunu
kanıtlamaz. Loglar yerel/Radmin IP'leri içerebilir.

## Implemented in this delivery

- Native subnet/global discovery, direct-IP fallback, actual session IDs/keys,
  heartbeat admission/leave/expiry and a bounded host-distributed peer roster.
- Scoped network identity and online admission for local player 0. The stored
  offline profile XUID is unchanged; save/profile calls keep their original
  identity. Network callers get an independent per-process identity.
- App-local XNet address/connection/QoS calls and virtual UDP/VDP guest sockets.
  Guest ports stay separate from the host transport ports. Original datagram
  bytes are copied through the transport; peer-to-peer forwarding uses the
  host's roster. This is not a public internet or authenticated service.
- Scatter/gather WSA send/receive ABI, pending overlapped receives, real kernel
  event completion, result/error reporting, cancellation and socket cleanup.
  Pending receives retain copied buffer descriptors, not temporary stack
  `bytes/flags` output pointers. The receiver stops before guest memory teardown.
- Bounded copied contexts and scalar/string/binary property serialization,
  filtering and correct search row slot/property/context pointers. Public and
  private lobby capacities are represented. QoS metadata comes from the host;
  Internet bandwidth and latency are not measured.
- General page: LAN toggle, player/room names, automatic local adapter selection
  with explicit IPv4 override, optional host IPv4 fallback. EN/TR localization.
- Separate game cover and launcher logo icons, existing graphics/controller,
  collision, career livery readback and stability changes remain in the package.

## Remaining verification and limits

Rendered Player Match admission, racing synchronization and real Radmin
broadcast/routing remain **unverified**. Wine in this cloud reports no physical
IP Helper adapters; earlier network checks used loopback. Actual Windows GPU
and two-PC play need the user's machines. This is the first experimental
integration, so additional title-specific hooks may be needed after that test.

Initial use is two players. Multi-client mesh and private slots have code
paths but are not validated in this delivery. Nonzero WSA completion routines,
TCP-based multiplayer and blocking scalar receive are not implemented by this
bridge; the inspected title VDP path uses events, overlapped receives and
nonblocking sockets. No leaderboards, ranked arbitration, voice verification
or host migration is claimed. Disabling LAN forwards original imports to the
pinned runtime. Existing offline fixes were validated in earlier builds; the
startup fix passed 17 targeted checks, but no broad new gameplay/regression run was performed.
