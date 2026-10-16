# Windows LAN multiplayer — experimental test build

14 October 2026. SDK: **0.10.0.24-dev.gbd833a2**. Game: TU **0.0.1.1**.
Protocol: **LAN02**; both PCs must use this exact package. The older LAN
foundation package is incompatible.

This build connects the original title's session and UDP/VDP socket imports to
our native LAN transport. The user has now reported a successful two-player
Radmin race; early lobby retries remain a known issue addressed in the
[lobby stability fix](lan-lobby-stability.md). The initial LAN integration
delivery skipped new runtime/gameplay tests at the user's request. The subsequent [launcher startup fix](lan-startup-fix.md) passed
17 targeted real-SDK Windows/Wine checks. [Crash diagnostics](crash-reports.md)
passed 9 native process/fault scenarios. Windows EXEs compiled successfully;
packaging verifies matching runtime DLL hashes and ZIP integrity. This does not
replace native two-PC gameplay checks for the revised executable.

## Two-player Windows observations — 14 October 2026

The user found and joined a room using two local instances, then connected two
PCs through Radmin and reported stable racing. Logs show real bidirectional
guest packet exchange, two-member sessions and successful session starts.
Early attempts include a discovery-cache timeout and invalid leave parameters;
the latest patch fixes these and adds targeted Windows regression coverage.
Native Radmin testing of the revised patch is pending. Larger grids, poor
network conditions, reconnects and extended gameplay are not yet validated.

## Native Windows host observation — 14 October 2026

The user reached the rendered Player Match lobby without crashing. Their log
confirms automatic selection of an active Famatech Radmin VPN adapter, native
UDP bind on the discovery range, successful XGI create (`B0010`) and local join
(`B0012`), each with result `0`, and one member. This validates this user's local
host startup and lobby path. No second peer or game packet exchange appears in
the supplied log. Remote discovery, client join and synchronized racing still
need validation. Personal IPs, room IDs, original log and screenshots are not
committed here.

For one Windows PC, use the [two-instance test instructions](lan-one-pc-test.md).

## Kullanım: iki Windows bilgisayarı

1. İki bilgisayara da aynı ZIP'in **tamamını** çıkar. Mevcut `game/` klasörünü,
   ayarlarını ve kayıtlarını koru. Eski EXE ile yeni DLL/EXE'leri karıştırma.
2. Radmin VPN'de aynı ağa katıl. İki PC de Radmin'de çevrimiçi görünmeli.
   Aynı ev ağı kullanılıyorsa Radmin zorunlu değil.
3. Her iki bilgisayarda launcher **General > Connection mode > LAN / Radmin**
   seçeneğini seç. Farklı oyuncu adları yaz. Basit ASCII ad kullanabilirsin;
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
- General page: Offline / LAN / Online selection, player/room names, automatic local adapter selection
  with explicit IPv4 override, optional host IPv4 fallback. EN/TR localization.
- Separate game cover and launcher logo icons, existing graphics/controller,
  collision, career livery readback and stability changes remain in the package.

## Remaining verification and limits

The user has verified local discovery/join and reported successful two-player
Radmin racing with the preceding executable. Lobby retry regressions are now
tested under Wine; native testing of the updated fix is pending. Wine in this
cloud reports no physical IP Helper adapters, so its network checks use
loopback and cannot substitute for Radmin routing/gameplay checks.

Initial use is two players. Multi-client mesh and private slots have code
paths but are not validated in this delivery. Nonzero WSA completion routines,
TCP-based multiplayer and blocking scalar receive are not implemented by this
bridge; the inspected title VDP path uses events, overlapped receives and
nonblocking sockets. No leaderboards, ranked arbitration, voice verification
or host migration is claimed. Disabling LAN forwards original imports to the
pinned runtime. Existing offline fixes were validated in earlier builds; the
startup fix passed 17 targeted checks, but no broad new gameplay/regression run was performed.
