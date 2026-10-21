  // Concise, localized help appears only on hover. The field label also carries
  // the tip so options disabled by another setting can still explain themselves.
  void Tip(HWND control,const wchar_t* en,const wchar_t* tr){
    const auto text=T(en,tr);help.Set(control,text);
    if(auto label=help_labels.find(control);label!=help_labels.end())help.Set(label->second,text);
  }
  void LocalizeUpscalerHelp(){
    const auto selected=SendMessageW(effect,CB_GETCURSEL,0,0);
    if(selected==3||selected==4){
      Tip(effect,L"FSR 2 / 3 uses neutral motion and depth inputs. It may fall back to FSR 1. No frame generation.",L"FSR 2 / 3, hareket ve derinlik için sabit girdiler kullanır. Gerekirse FSR 1'e geçer. Kare üretimi yapmaz.");
    }else Tip(effect,L"FSR upscales the rendered image. CAS sharpens it without upscaling. Bilinear keeps sharpness adjustment off.",L"FSR görüntüyü büyütür. CAS görüntüyü büyütmeden keskinleştirir. Bilineer seçiliyken keskinlik ayarlanamaz.");
    Tip(fsr_quality,L"Available with FSR 2 / 3. Controls the internal scaling quality; Auto uses the runtime default.",L"FSR 2 / 3 ile kullanılabilir. Ölçekleme kalitesini belirler; Otomatik, varsayılan kalite ayarını kullanır.");
    Tip(sharpness,L"Sharpness for the selected FSR or CAS mode. Choose an upscaler or sharpener to enable this slider.",L"Seçili FSR veya CAS modunun görüntü keskinliğini ayarlar. Bu ayarı kullanmak için bir ölçekleme veya keskinleştirme yöntemi seç.");
  }
  void LocalizeHelp(){
    help.Pop();
    Tip(path,L"Select your original game folder containing default.xex, the title update and xenondata.",L"default.xex, oyun güncellemesi ve xenondata içeren özgün oyun klasörünü seç.");
    Tip(browse,L"Choose the folder containing your original game files.",L"Özgün oyun dosyalarının bulunduğu klasörü seç.");
    RefreshGameFilesAction();
    Tip(game_language,L"Changes game text and subtitles. Original voices are preserved.",L"Oyun metinlerini ve altyazıları değiştirir. Özgün seslendirmeler korunur.");
    Tip(prompt_style,L"Automatic follows your input device. Choose PlayStation, Xbox or keyboard to force the button symbols.",L"Otomatik, kullandığın giriş aygıtına göre tuş simgelerini değiştirir. Simge türünü sabitlemek için PlayStation, Xbox veya Klavye seç.");
    Tip(crash_diagnostics,L"Collects an additional local crash report if the game fails. Reports are saved in Logs.",L"Oyun çökerse ek bir yerel çökme raporu oluşturur. Raporlar logs klasörüne kaydedilir.");
    Tip(render_resolution,L"1.5x uses a 720p source and FSR 1, 2 or 3. Windowed/fullscreen targets 1080p; Borderless uses desktop resolution. FSR 2 / 3 has limited temporal inputs.",L"1.5x, 720p kaynak ve FSR 1, 2 veya 3 kullanır. Pencereli/tam ekran 1080p'yi hedefler; Kenarlıksız masaüstü çözünürlüğünü kullanır. FSR 2 / 3'ün zamansal girdileri sınırlıdır.");
    Tip(size,L"Common and monitor-reported window sizes that fit the selected display. Borderless uses your desktop resolution.",L"Seçili ekrana sığan yaygın ve monitörün bildirdiği pencere boyutları. Kenarlıksız mod masaüstü çözünürlüğünü kullanır.");
    Tip(display_monitor,L"Uses your primary screen by default. Choose another monitor for the next game launch; resolution and refresh-rate choices follow it.",L"Varsayılan olarak birincil ekranını kullanır. Sonraki oyun açılışı için başka monitör seçebilirsin; çözünürlük ve yenileme hızı seçenekleri ona göre güncellenir.");
    Tip(refresh_rate,L"Exclusive fullscreen only. Borderless and windowed modes use the desktop refresh rate.",L"Yalnızca tam ekran modunda ayarlanabilir. Kenarlıksız ve pencereli modlar masaüstünün yenileme hızını kullanır.");
    Tip(fps_limit,L"Adds a maximum frame rate. This does not remove the game's own limit.",L"Kare hızına bir üst sınır koyar. Oyunun kendi kare hızı sınırını kaldırmaz.");
    Tip(aspect,L"Auto adapts to wider windows and keeps 16:9 on narrower screens. Forced 4:3 and ultrawide ratios are experimental.",L"Otomatik, geniş pencerelere uyum sağlar; dar ekranlarda 16:9'u korur. Sabit 4:3 ve ultra geniş oranlar deneyseldir.");
    Tip(msaa,L"Uses native 2× MSAA when supported by your GPU.",L"Ekran kartın destekliyorsa oyunun yerleşik 2× MSAA kenar yumuşatmasını kullanır.");
    Tip(letterbox,L"Preserves the aspect ratio, adding black bars when needed.",L"Görüntü oranını korur; gerektiğinde siyah kenarlar ekler.");
    Tip(dither,L"Adds subtle dithering to reduce banding in color gradients.",L"Renk geçişlerine hafif bir gürültü ekleyerek bantlaşmayı azaltır.");
    Tip(shader_cache,L"Reuses saved shaders to reduce stutter on later runs.",L"Kaydedilen shader'ları tekrar kullanarak sonraki açılışlarda takılmaları azaltır.");
    Tip(engine_choices[0],L"Adds motion blur as speed increases. Off removes this blur.",L"Hız arttıkça hareket bulanıklığı ekler. Kapatıldığında bu bulanıklık uygulanmaz.");
    Tip(engine_choices[1],L"Blurs areas outside the camera's focus. Off keeps them sharp.",L"Kameranın odak noktası dışındaki alanları bulanıklaştırır. Kapatıldığında bu alanlar net kalır.");
    Tip(engine_choices[2],L"Draws real-time rider and bike shadows.",L"Sürücü ve motosikletin gerçek zamanlı gölgelerini çizer.");
    Tip(engine_choices[3],L"Controls speed-related darkening at the screen edges; percentages scale the game strength.",L"Hıza bağlı olarak ekran kenarlarının ne kadar kararacağını ayarlar. Yüzdeler, oyunun varsayılan efekt gücüne göre uygulanır.");
    Tip(engine_choices[4],L"Controls glow around bright areas; percentages scale the game's bloom strength.",L"Parlak alanların etrafındaki ışık yayılmasını ayarlar. Yüzdeler, oyunun varsayılan efekt gücüne göre uygulanır.");
    Tip(engine_choices[5],L"Filters shadow edges for a softer appearance.",L"Gölge kenarlarını daha yumuşak görünmeleri için filtreler.");
    Tip(engine_choices[6],L"Sets the soft-shadow range, within the overall shadow render distance.",L"Genel gölge çizim mesafesi içindeki yumuşak gölge mesafesini ayarlar.");
    Tip(engine_choices[7],L"Controls the density of particles such as smoke, sparks and rain.",L"Duman, kıvılcım ve yağmur gibi parçacıkların yoğunluğunu ayarlar.");
    Tip(engine_choices[8],L"Scales instance visibility distance within the engine's limits. Higher values can increase GPU load.",L"Nesnelerin görünme mesafesini motorun sınırları içinde değiştirir. Yüksek değerler GPU yükünü artırabilir.");
    Tip(debug_overlay,L"Enables F3 diagnostics and detailed Debug logs on the next launch. Off uses normal Info logs unless you explicitly choose another log level.",L"Sonraki açılışta F3 tanılama ekranını ve ayrıntılı hata ayıklama günlüklerini açar. Kapalıyken, başka bir günlük düzeyi seçmediysen normal bilgi günlükleri kullanılır.");
    Tip(shadow_distance,L"Sets how far rider and bike shadows are drawn. Higher values can increase GPU load.",L"Sürücü ve motosiklet gölgelerinin çizim mesafesini ayarlar. Yüksek değerler GPU yükünü artırabilir.");
    Tip(backend,L"SDL supports native PlayStation and other controllers. DualSense adaptive triggers require SDL.",L"SDL, PlayStation ve diğer kontrolcüleri doğrudan destekler. DualSense adaptif tetikleri için SDL gerekir.");
    Tip(keyboard,L"Enable keyboard input in addition to your controller. Edit individual keys below.",L"Kontrolcünün yanında klavye girişini etkinleştirir. Tuşları aşağıdan düzenleyebilirsin.");
    Tip(adaptive_triggers,L"Experimental fixed trigger resistance for DualSense / Edge using SDL. Other controllers keep their normal triggers.",L"SDL kullanan DualSense / Edge için deneysel sabit tetik direnci. Diğer kontrolcülerin tetikleri normal çalışır.");
    Tip(reset_bindings,L"Restore only the keyboard bindings. Other launcher settings are preserved.",L"Yalnızca klavye tuş atamalarını varsayılanlara döndürür. Launcher'ın diğer ayarları korunur.");
    for(size_t i=0;i<binding_buttons.size();++i){
      Tip(binding_buttons[i],L"Click, then press a key or key combination. Esc cancels. Right-click to clear this binding.",L"Tıkla, ardından bir tuşa veya tuş birleşimine bas. Esc iptal eder. Atamayı silmek için sağ tıkla.");
    }
    Tip(save,L"Save launcher settings for the next game launch. During play, F1 opens settings that can change immediately.",L"Launcher ayarlarını sonraki oyun açılışı için kaydeder. Oyun sırasında F1 ile bazı ayarları oyunu kapatmadan değiştirebilirsin.");
    Tip(defaults,L"Restore launcher settings to their defaults. Game saves and achievements are kept.",L"Launcher ayarlarını varsayılanlara döndürür. Oyun kayıtları ve başarımlar korunur.");
    Tip(logs,L"Open the local log and crash-report folder.",L"Yerel günlüklerin ve çökme raporlarının bulunduğu klasörü açar.");
    Tip(report,L"Create a local diagnostic report with settings and recent logs. Nothing is uploaded.",L"Ayarları ve son günlükleri içeren yerel bir tanılama raporu oluşturur. İnternete hiçbir şey gönderilmez.");
    Tip(play,L"Save the current launcher settings and start MotoGP 09/10.",L"Launcher'ın geçerli ayarlarını kaydedip MotoGP 09/10'u başlatır.");
    LocalizeNetworkHelp();
    LocalizeUpscalerHelp();
  }
