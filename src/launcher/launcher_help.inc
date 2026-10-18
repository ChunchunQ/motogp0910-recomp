  // Concise, localized help appears only on hover. The field label also carries
  // the tip so options disabled by another setting can still explain themselves.
  void Tip(HWND control,const wchar_t* en,const wchar_t* tr){
    const auto text=T(en,tr);help.Set(control,text);
    if(auto label=help_labels.find(control);label!=help_labels.end())help.Set(label->second,text);
  }
  void LocalizeUpscalerHelp(){
    const auto selected=SendMessageW(effect,CB_GETCURSEL,0,0);
    if(selected==3||selected==4){
      Tip(effect,L"FSR 2 / 3 uses neutral motion and depth inputs. It may fall back to FSR 1. No frame generation.",L"FSR 2 / 3, nötr hareket ve derinlik verileri kullanır. FSR 1'e dönebilir. Kare üretimi yoktur.");
    }else Tip(effect,L"FSR upscales the rendered image. CAS sharpens it without upscaling. Bilinear keeps sharpness adjustment off.",L"FSR, çizilen görüntüyü ölçekler. CAS, ölçeklemeden keskinleştirir. Bilinear seçiliyken keskinlik ayarlanamaz.");
    Tip(fsr_quality,L"Available with FSR 2 / 3. Controls the internal scaling quality; Auto uses the runtime default.",L"FSR 2 / 3 ile kullanılabilir. İç ölçekleme kalitesini belirler; Otomatik, çalışma zamanının varsayılanını kullanır.");
    Tip(sharpness,L"Sharpness for the selected FSR or CAS mode. Choose an upscaler or sharpener to enable this slider.",L"Seçili FSR veya CAS modunun keskinliği. Kaydırıcıyı açmak için bir ölçekleyici veya keskinleştirici seç.");
  }
  void LocalizeHelp(){
    help.Pop();
    Tip(path,L"Select your original game folder containing default.xex, the title update and xenondata.",L"default.xex, title update ve xenondata içeren özgün oyun klasörünü seç.");
    Tip(browse,L"Choose the folder containing your original game files.",L"Özgün oyun dosyalarının bulunduğu klasörü seç.");
    Tip(verify_files,L"Check original game files against the supported hashes. No files are changed or downloaded. Click again to cancel.",L"Özgün oyun dosyalarını desteklenen özetlerle karşılaştırır. Dosyalar değiştirilmez veya indirilmez. İptal için tekrar tıkla.");
    Tip(game_language,L"Changes game text and subtitles. Original voices are preserved.",L"Oyun metinlerini ve altyazıları değiştirir. Özgün seslendirmeler korunur.");
    Tip(prompt_style,L"Automatic follows your input device. Choose PlayStation, Xbox or keyboard to force the button symbols.",L"Otomatik, giriş aygıtını izler. Tuş simgelerini sabitlemek için PlayStation, Xbox veya klavye seç.");
    Tip(crash_diagnostics,L"Collects an additional local crash report if the game fails. Reports are saved in Logs.",L"Oyun çökerse ek bir yerel çökme raporu toplar. Raporlar Loglar klasörüne kaydedilir.");
    Tip(render_resolution,L"Internal drawing resolution. Higher settings sharpen the image and increase GPU load.",L"İç çizim çözünürlüğü. Yüksek ayarlar görüntüyü netleştirir ve GPU yükünü artırır.");
    Tip(size,L"Common and monitor-reported window sizes that fit the selected display. Borderless uses your desktop resolution.",L"Seçili ekrana sığan yaygın ve monitörün bildirdiği pencere boyutları. Kenarlıksız mod masaüstü çözünürlüğünü kullanır.");
    Tip(refresh_rate,L"Exclusive fullscreen only. Borderless and windowed modes use the desktop refresh rate.",L"Yalnızca özel tam ekranda kullanılır. Kenarlıksız ve pencereli modlar masaüstünün yenileme hızını kullanır.");
    Tip(fps_limit,L"Adds a maximum frame rate. This does not remove the game's own limit.",L"En yüksek kare hızına sınır ekler. Oyunun kendi sınırını kaldırmaz.");
    Tip(aspect,L"Auto keeps the original 16:9 view. Wider ratios are experimental.",L"Otomatik, özgün 16:9 görüntüyü korur. Daha geniş oranlar deneyseldir.");
    for(auto control:engine_choices)Tip(control,L"Game default preserves the original effect. Off disables this effect.",L"Oyunun ayarı, özgün efekti korur. Kapalı, bu efekti devre dışı bırakır.");
    Tip(debug_overlay,L"Enables F3 to show performance, display and runtime diagnostics. Off by default; applies on the next launch.",L"F3 ile performans, görüntü ve çalışma bilgilerini açar. Varsayılan kapalıdır; sonraki açılışta uygulanır.");
    Tip(shadow_distance,L"Game default preserves each track's shadow distance. A higher distance can increase GPU load.",L"Oyunun ayarı, her pistin özgün gölge mesafesini korur. Yüksek mesafe GPU yükünü artırabilir.");
    Tip(backend,L"SDL supports native PlayStation and other controllers. DualSense adaptive triggers require SDL.",L"SDL, doğal PlayStation ve diğer kontrolcüleri destekler. DualSense adaptif tetikler için SDL gerekir.");
    Tip(keyboard,L"Enable keyboard input in addition to your controller. Edit individual keys below.",L"Kontrolcünün yanında klavye girişini etkinleştirir. Tuşları aşağıdan düzenleyebilirsin.");
    Tip(adaptive_triggers,L"Experimental fixed trigger resistance for DualSense / Edge using SDL. Other controllers keep their normal triggers.",L"SDL kullanan DualSense / Edge için deneysel sabit tetik direnci. Diğer kontrolcülerin tetikleri normal çalışır.");
    Tip(reset_bindings,L"Restore only the keyboard bindings. Other launcher settings are preserved.",L"Yalnızca klavye tuşlarını varsayılana döndürür. Diğer launcher ayarları korunur.");
    for(size_t i=0;i<binding_buttons.size();++i){
      Tip(binding_buttons[i],L"Click, then press a key or key combination. Esc cancels. Right-click to clear this binding.",L"Tıkla, ardından bir tuşa veya tuş birleşimine bas. Esc iptal eder. Atamayı silmek için sağ tıkla.");
    }
    Tip(save,L"Save launcher settings for the next game launch. During play, F1 opens settings that can change immediately.",L"Launcher ayarlarını sonraki oyun açılışı için kaydeder. Oyun sırasında F1, anında değişen ayarları açar.");
    Tip(defaults,L"Restore launcher settings to their defaults. Game saves and achievements are kept.",L"Launcher ayarlarını varsayılana döndürür. Oyun kayıtları ve başarımlar korunur.");
    Tip(logs,L"Open the local log and crash-report folder.",L"Yerel log ve çökme raporları klasörünü açar.");
    Tip(report,L"Create a local diagnostic report with settings and recent logs. Nothing is uploaded.",L"Ayarlar ve son loglarla yerel bir tanılama raporu oluşturur. Hiçbir şey yüklenmez.");
    Tip(play,L"Save the current launcher settings and start MotoGP 09/10.",L"Geçerli launcher ayarlarını kaydedip MotoGP 09/10'u başlatır.");
    LocalizeNetworkHelp();
    LocalizeUpscalerHelp();
  }
