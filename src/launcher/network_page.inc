  // The network page shares the main launcher's pending settings and Save/PLAY
  // transaction. Switching modes or tabs never discards hidden field values.
  void BuildNetworkPage(){
    const auto label=LabelControl(6,L"Connection mode",L"Bağlantı modu",0,0,kFormWidth,24);
    network_mode=PageControl(6,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,0,26,kFormWidth,220,kLanEnabled);
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
  }
  void UpdateNetworkMode(){
    if(!network_mode)return;
    help.Pop();
    const auto mode=SendMessageW(network_mode,CB_GETCURSEL,0,0);
    for(auto control:{network_player,network_room,network_adapter,network_host,network_server}){
      const bool relevant=control==network_server?mode==2:
          (control==network_adapter||control==network_host)?mode==1:mode>0;
      if(!relevant&&GetFocus()==control)SetFocus(network_mode);
      EnableWindow(control,!process&&relevant);
      ShowWindow(control,current_page==6&&relevant?SW_SHOW:SW_HIDE);
      ShowWindow(help_labels.at(control),current_page==6&&relevant?SW_SHOW:SW_HIDE);
    }
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
    s.lan_bind_ip=Utf8(Text(network_adapter));s.lan_host_ip=Utf8(Text(network_host));s.online_server=Utf8(Text(network_server));
  }
  void LocalizeNetworkHelp(){
    Tip(network_mode,L"Offline uses no multiplayer service. LAN works on local or virtual networks. Online uses a community server. Switching modes keeps your entered details.",L"Çevrimdışı, çok oyunculu hizmete bağlanmaz. LAN, yerel veya sanal ağlarda çalışır. Çevrimiçi, topluluk sunucusunu kullanır. Mod değiştirirken girilen bilgiler korunur.");
    Tip(network_player,L"Name shown to other players. Up to 15 UTF-8 bytes.",L"Diğer oyuncuların gördüğü ad. En fazla 15 UTF-8 bayt.");
    Tip(network_room,L"Used when you host a room. Up to 31 UTF-8 bytes. Both players need the same build and Player Match.",L"Odayı kurduğunda kullanılır. En fazla 31 UTF-8 bayt. Oyuncular aynı buildi ve Player Match seçeneğini kullanmalı.");
    Tip(network_adapter,L"Leave empty for automatic selection, including virtual LAN adapters. Both players must join the same local or virtual LAN.",L"Sanal LAN bağdaştırıcıları dahil otomatik seçim için boş bırak. Oyuncular aynı yerel veya sanal LAN ağına bağlanmalı.");
    Tip(network_host,L"Rooms are discovered automatically. Enter the host's LAN IPv4 only if discovery fails.",L"Odalar otomatik bulunur. Yalnızca oda bulunamazsa kurucunun LAN IPv4 adresini gir.");
    Tip(network_server,L"Use the same running community HTTPS server as the other players. This launcher does not host a server. Use the same build and Player Match.",L"Diğer oyuncularla aynı, çalışır durumdaki topluluk HTTPS sunucusunu kullan. Launcher sunucu açmaz. Aynı buildi ve Player Match seçeneğini kullanın.");
  }
