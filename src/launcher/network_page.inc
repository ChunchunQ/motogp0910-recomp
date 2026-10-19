  // The network page shares the main launcher's pending settings and Save/PLAY
  // transaction. Switching modes or tabs never discards hidden field values.
  void BuildNetworkPage(){
    const auto label=LabelControl(6,L"Connection mode",L"Bağlantı modu",0,0,kFormWidth,24);
    network_mode=PageControl(6,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,0,26,kFormWidth,310,kLanEnabled);
    help_labels[network_mode]=label;
    auto field=[&](const wchar_t* en,const wchar_t* tr,int x,int y,int width,int id,int limit){
      const auto caption=LabelControl(6,en,tr,x,y,width,24);
      const auto control=PageControl(6,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,x,y+28,width,30,id);
      SendMessageW(control,EM_SETLIMITTEXT,limit,0);help_labels[control]=caption;return control;
    };
    network_player=field(L"Player name",L"Oyuncu adı",0,88,kColumnWidth,kLanPlayer,15);
    network_room=field(L"Room name (host)",L"Oda adı (kurucu)",kColumnX,88,kColumnWidth,kLanRoom,31);
    network_adapter=field(L"Local IPv4 (optional)",L"Yerel IPv4 (isteğe bağlı)",0,176,kColumnWidth,kLanAdapter,15);
    network_host=field(L"Host IPv4 (optional)",L"Kurucu IPv4 (isteğe bağlı)",kColumnX,176,kColumnWidth,kLanHost,15);
    network_server=field(L"Online server (HTTPS)",L"Çevrimiçi sunucu (HTTPS)",0,176,kFormWidth,kOnlineServer,512);
    // Online: the NOBK Online site account replaces the free player name.
    account_label=LabelControl(6,L"NOBK Online account",L"NOBK Online hesabı",0,88,kColumnWidth,24);
    account_status=PageControl(6,L"STATIC",L"",SS_LEFTNOWORDWRAP|SS_CENTERIMAGE,0,116,kColumnWidth,30);
    account_user=field(L"Username",L"Kullanıcı adı",0,176,kColumnWidth,kAccountUser,15);
    const auto pass_caption=LabelControl(6,L"Password",L"Şifre",kColumnX,176,kColumnWidth,24);
    account_pass=PageControl(6,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD|WS_BORDER,kColumnX,204,kColumnWidth,30,kAccountPass);
    SendMessageW(account_pass,EM_SETLIMITTEXT,128,0);help_labels[account_pass]=pass_caption;
    account_signin=PageControl(6,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,0,264,kColumnWidth,32,kSignIn);
    account_register=PageControl(6,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,kColumnX,264,kColumnWidth,32,kRegister);
    account_signout=PageControl(6,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON,0,176,kColumnWidth,32,kSignOut);
  }
  void ShowAccountControls(bool online){
    const bool signed_in=account.has_value();
    const auto show=[&](HWND control,bool visible){
      if(!visible&&GetFocus()==control)SetFocus(network_mode);
      EnableWindow(control,!process&&visible);ShowWindow(control,current_page==6&&visible?SW_SHOW:SW_HIDE);
    };
    show(account_label,online);show(account_status,online);
    for(auto control:{account_user,account_pass}){show(control,online&&!signed_in);show(help_labels.at(control),online&&!signed_in);}
    show(account_signin,online&&!signed_in);show(account_register,online&&!signed_in);show(account_signout,online&&signed_in);
    SetWindowTextW(account_status,signed_in?(T(L"Signed in: ",L"Giriş yapıldı: ")+Wide(account->username)).c_str():T(L"Not signed in",L"Giriş yapılmadı"));
  }
  void AccountSignIn(){
    const auto user=Utf8(Text(account_user));auto password=Utf8(Text(account_pass));
    Status(L"Signing in to NOBK Online…",L"NOBK Online'a giriş yapılıyor…");UpdateWindow(window);
    try{account=SignIn(user,password,turkish);}catch(...){SecureZeroMemory(password.data(),password.size());throw;}
    SecureZeroMemory(password.data(),password.size());SetWindowTextW(account_pass,L"");
    UpdateNetworkMode();
    Status(L"Signed in as "+Wide(account->username)+L". Press PLAY.",Wide(account->username)+L" olarak giriş yapıldı. OYUNU BAŞLAT'a bas.",true);
  }
  void AccountSignOut(){
    ForgetAccount();account.reset();UpdateNetworkMode();
    Status(L"Signed out of NOBK Online.",L"NOBK Online hesabından çıkış yapıldı.");
  }
  // PLAY in Online mode: refresh the sign-in, link this PC's game profile and
  // start the game with the account name.
  void PrepareOnlineLaunch(){
    if(SendMessageW(network_mode,CB_GETCURSEL,0,0)!=2)return;
    Status(L"Connecting to NOBK Online…",L"NOBK Online'a bağlanılıyor…");UpdateWindow(window);
    std::string name;
    try{name=PrepareOnline(kCommunityServer,turkish);}
    catch(const SignInRejected&){account.reset();UpdateNetworkMode();throw;}
    account=LoadAccount();
    SetWindowTextW(network_player,Wide(name).c_str());UpdateNetworkMode();
  }
  void UpdateNetworkMode(){
    if(!network_mode)return;
    help.Pop();
    const auto mode=SendMessageW(network_mode,CB_GETCURSEL,0,0);
    for(auto control:{network_player,network_room,network_adapter,network_host,network_server}){
      // The community server is fixed (kCommunityServer); its box stays hidden.
      // Online uses the NOBK Online account name instead of a typed player name.
      const bool relevant=control==network_server?false:control==network_player?mode==1:
          (control==network_adapter||control==network_host)?mode==1:mode>0;
      if(!relevant&&GetFocus()==control)SetFocus(network_mode);
      EnableWindow(control,!process&&relevant);
      ShowWindow(control,current_page==6&&relevant?SW_SHOW:SW_HIDE);
      ShowWindow(help_labels.at(control),current_page==6&&relevant?SW_SHOW:SW_HIDE);
    }
    ShowAccountControls(mode==2);
    EnableWindow(network_mode,!process);
    if(current_page==6)RedrawWindow(page_content,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
  }
  void SetNetworkControls(const Settings& s){
    SendMessageW(network_mode,CB_SETCURSEL,s.network_mode=="online"?2:s.network_mode=="lan"?1:0,0);
    for(auto [control,value]:{std::pair{network_player,&s.lan_player_name},{network_room,&s.lan_room_name},
        {network_adapter,&s.lan_bind_ip},{network_host,&s.lan_host_ip},{network_server,&s.online_server}})
      SetWindowTextW(control,Wide(*value).c_str());
    UpdateNetworkMode();
  }
  void ReadNetworkControls(Settings& s)const{
    const auto mode=SendMessageW(network_mode,CB_GETCURSEL,0,0);
    if(mode<0||mode>2)throw std::runtime_error("Invalid connection mode");
    s.network_mode=std::array{"offline","lan","online"}[mode];s.lan_enabled=mode==1;
    s.lan_player_name=Utf8(Text(network_player));s.lan_room_name=Utf8(Text(network_room));
    s.lan_bind_ip=Utf8(Text(network_adapter));s.lan_host_ip=Utf8(Text(network_host));s.online_server=kCommunityServer;
  }
  void LocalizeNetworkHelp(){
    Tip(network_mode,L"Online connects to NOBK Online automatically (rooms, relay and leaderboards). Offline uses no multiplayer service. LAN works on local or virtual networks. Switching modes keeps your entered details.",L"Çevrimiçi, NOBK Online'a otomatik bağlanır (odalar, aktarım ve sıralamalar). Çevrimdışı, çok oyunculu hizmete bağlanmaz. LAN, yerel veya sanal ağlarda çalışır. Mod değiştirirken girilen bilgiler korunur.");
    Tip(network_player,L"Name shown to other players. Up to 15 UTF-8 bytes.",L"Diğer oyuncuların gördüğü ad. En fazla 15 UTF-8 bayt.");
    Tip(network_room,L"Used when you host a room. Up to 31 UTF-8 bytes. Both players need the same build and Player Match.",L"Odayı kurduğunda kullanılır. En fazla 31 UTF-8 bayt. Oyuncular aynı buildi ve Player Match seçeneğini kullanmalı.");
    Tip(network_adapter,L"Leave empty for automatic selection, including virtual LAN adapters. Both players must join the same local or virtual LAN.",L"Sanal LAN bağdaştırıcıları dahil otomatik seçim için boş bırak. Oyuncular aynı yerel veya sanal LAN ağına bağlanmalı.");
    Tip(network_host,L"Rooms are discovered automatically. Enter the host's LAN IPv4 only if discovery fails.",L"Odalar otomatik bulunur. Yalnızca oda bulunamazsa kurucunun LAN IPv4 adresini gir.");
    SetWindowTextW(account_signin,T(L"Sign in",L"Giriş yap"));SetWindowTextW(account_register,T(L"Create an account",L"Hesap oluştur"));
    SetWindowTextW(account_signout,T(L"Sign out",L"Çıkış yap"));
    Tip(account_user,L"Your NOBK Online account (nobkonline.com), the same one you use for FIFA 09. It is your name in rooms and on the site.",L"NOBK Online hesabın (nobkonline.com); FIFA 09'da kullandığın hesapla aynı. Odalarda ve sitede bu adla görünürsün.");
    Tip(account_pass,L"Only sent to nobkonline.com to sign in. The launcher keeps a sign-in token, never the password.",L"Sadece giriş için nobkonline.com'a gönderilir. Launcher şifreni değil, bir giriş anahtarı saklar.");
    Tip(account_signout,L"Removes the saved sign-in from this Windows user.",L"Bu Windows kullanıcısındaki kayıtlı girişi siler.");
    Tip(network_server,L"Use the same running community HTTPS server as the other players. This launcher does not host a server. Use the same build and Player Match.",L"Diğer oyuncularla aynı, çalışır durumdaki topluluk HTTPS sunucusunu kullan. Launcher sunucu açmaz. Aynı buildi ve Player Match seçeneğini kullanın.");
  }
