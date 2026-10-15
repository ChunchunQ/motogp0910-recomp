  // Included inside App: native Win32 page using the same palette and fonts.
  HWND achievement_search_label{},achievement_search{},achievement_filter{},achievement_popup{},achievement_sound{},achievement_list{},achievement_note{};
  achievements::Catalog catalog;
  std::future<achievements::Catalog> catalog_future;
  std::wstring catalog_key,loading_key,catalog_error;
  std::vector<std::unique_ptr<Gdiplus::Bitmap>> achievement_icons;
  std::map<uint32_t,uint64_t> unlocks;
  std::vector<size_t> achievement_rows;
  fs::path achievement_user_root;
  motogp::achievements::Options achievement_options;
  bool unlocks_known=false,unlocks_stale=false;
  int achievement_scroll=0;
  void LocalizeAchievements(){
    if(!achievement_filter)return;
    SetWindowTextW(achievement_search_label,T(L"Search",L"Ara"));
    FillCombo(achievement_filter,{T(L"All achievements",L"Tüm başarımlar"),T(L"Unlocked",L"Kazanılanlar"),T(L"Locked",L"Kilitli")});
    SetWindowTextW(achievement_popup,T(L"Pop-up in game",L"Oyun içi bildirim"));
    SetWindowTextW(achievement_sound,T(L"Unlock sound",L"Başarım sesi"));
    SendMessageW(achievement_search,EM_SETCUEBANNER,FALSE,reinterpret_cast<LPARAM>(T(L"Search achievements...",L"Başarımlarda ara...")));
    if(achievement_list)InvalidateRect(achievement_list,nullptr,FALSE);
  }
  void BuildAchievements(){
    auto add=[&](const wchar_t* cls,DWORD style,int x,int y,int w,int h,int id){return PageControl(5,cls,L"",style,x,y+110,w,h,id);};
    achievement_popup=add(L"BUTTON",WS_TABSTOP|BS_AUTOCHECKBOX,260,231,240,28,kAchievementPopup);
    achievement_sound=add(L"BUTTON",WS_TABSTOP|BS_AUTOCHECKBOX,520,231,240,28,kAchievementSound);
    SetCheck(achievement_popup,true);SetCheck(achievement_sound,true);
    achievement_search_label=add(L"STATIC",0,260,309,60,22,0);
    achievement_search=add(L"EDIT",WS_TABSTOP|ES_AUTOHSCROLL|WS_BORDER,326,304,474,28,kAchievementSearch);SendMessageW(achievement_search,EM_SETLIMITTEXT,160,0);
    achievement_filter=add(L"COMBOBOX",WS_TABSTOP|CBS_DROPDOWNLIST,818,304,252,180,kAchievementFilter);
    achievement_list=add(L"STATIC",WS_TABSTOP|WS_VSCROLL|SS_NOTIFY,260,346,810,338,kAchievementList);
    SetWindowSubclass(achievement_list,AchievementListProc,1,reinterpret_cast<DWORD_PTR>(this));
    achievement_note=add(L"STATIC",0,260,697,810,24,0);
  }
  fs::path AchievementRoot()const{return achievements::UserRoot(exe,Get(config.table,"user_data_root",std::string{}));}
  int AchievementLanguage()const{const auto index=SendMessageW(game_language,CB_GETCURSEL,0,0);return index>=0&&index<5?std::array{1,4,3,5,6}[index]:settings.game_language;}
  std::wstring CatalogKey()const{return GameRoot().wstring()+L"|"+achievements::MetadataRoot(exe,Get(config.table,"metadata_root",std::string{})).wstring()+L"|"+std::to_wstring(AchievementLanguage());}
  void PollAchievements(){
    try {
      const auto wanted=CatalogKey();
      if(catalog_future.valid()&&catalog_future.wait_for(std::chrono::seconds(0))==std::future_status::ready){
        try {auto loaded=catalog_future.get();if(loading_key==wanted){catalog=std::move(loaded);catalog_key=loading_key;catalog_error.clear();CacheAchievementIcons();FilterAchievements();}}
        catch(const std::exception& e){if(loading_key==wanted){catalog_error=ErrorDetails(e);catalog_key=loading_key;}}
      }
      if(current_page!=5){if(!catalog_future.valid())KillTimer(window,2);return;}
      if(wanted!=catalog_key&&!catalog_future.valid()){
        catalog.entries.clear();achievement_icons.clear();achievement_rows.clear();achievement_scroll=0;catalog_error.clear();loading_key=wanted;
        const auto game=GameRoot(),metadata=achievements::MetadataRoot(exe,Get(config.table,"metadata_root",std::string{}));const auto lang=AchievementLanguage();
        catalog_future=std::async(std::launch::async,[game,metadata,lang]{return achievements::LoadCatalog(game,metadata,lang);});
      }
      const auto root=AchievementRoot();
      if(root!=achievement_user_root){achievement_user_root=root;unlocks.clear();unlocks_known=false;unlocks_stale=false;}
      const auto file=root/"achievements"/"434307F0.toml";
      try {
        // The SDK briefly removes the old save before renaming its .tmp file.
        if(!fs::exists(file)){
          if(unlocks_known&&!unlocks.empty())unlocks_stale=true;
          else {const bool changed=!unlocks_known;unlocks_known=true;unlocks_stale=false;if(changed)FilterAchievements(false);}
        }
        else {auto loaded=achievements::ReadUnlocks(file);const bool changed=loaded!=unlocks||!unlocks_known;unlocks=std::move(loaded);unlocks_known=true;unlocks_stale=false;if(changed)FilterAchievements(false);}
      }catch(const std::exception&){unlocks_stale=true;}
      try {achievement_options=motogp::achievements::ReadOptions(root/"achievement-ui.toml");SetCheck(achievement_popup,achievement_options.popup);SetCheck(achievement_sound,achievement_options.sound);EnableWindow(achievement_popup,TRUE);EnableWindow(achievement_sound,TRUE);}
      catch(const std::exception&){EnableWindow(achievement_popup,FALSE);EnableWindow(achievement_sound,FALSE);}
      SetWindowTextW(achievement_note,unlocks_stale&&!unlocks_known?T(L"Could not read your achievement save. Retrying...",L"Başarım kaydı okunamadı. Tekrar deneniyor..."):unlocks_stale?T(L"Save temporarily unavailable; showing the last known progress.",L"Kayıt geçici olarak okunamıyor; son bilinen ilerleme gösteriliyor."):T(L"Progress is read from your game save. F7 opens achievements in game.",L"İlerleme oyun kaydından okunur. Oyun içinde başarımlar için F7."));
      RECT summary{S(242),S(118),S(1088),S(224)};InvalidateRect(window,&summary,FALSE);InvalidateRect(achievement_list,nullptr,FALSE);
    }catch(const std::exception& e){catalog_error=ErrorDetails(e);InvalidateRect(achievement_list,nullptr,FALSE);}
  }
  void SaveAchievementOption(int id){
    const auto root=AchievementRoot();const auto file=root/"achievement-ui.toml";
    auto before=motogp::achievements::ReadOptions(file),after=before;
    if(id==kAchievementPopup)after.popup=!before.popup;else after.sound=!before.sound;
    if(!motogp::achievements::WriteOptions(file,before,after))throw Error(L"Could not save achievement preferences.",L"Başarım tercihleri kaydedilemedi.");
    achievement_options=after;SetCheck(achievement_popup,after.popup);SetCheck(achievement_sound,after.sound);
  }
  void CacheAchievementIcons(){
    achievement_icons.clear();
    for(const auto& entry:catalog.entries){
      std::unique_ptr<Gdiplus::Bitmap> bitmap;
      if(!entry.icon.empty()){
        IStream* stream=SHCreateMemStream(entry.icon.data(),static_cast<UINT>(entry.icon.size()));
        if(stream){auto source=std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromStream(stream));
          if(source&&source->GetLastStatus()==Gdiplus::Ok&&source->GetWidth()<=2048&&source->GetHeight()<=2048)bitmap.reset(source->Clone(0,0,source->GetWidth(),source->GetHeight(),PixelFormat32bppARGB));
          source.reset();stream->Release();}
      }
      achievement_icons.push_back(std::move(bitmap));
    }
  }
  bool AchievementEarned(uint32_t id)const{return unlocks.contains(id);}
  void FilterAchievements(bool reset=true){
    if(!achievement_search)return;
    achievement_rows.clear();const auto search=Text(achievement_search);const auto filter=SendMessageW(achievement_filter,CB_GETCURSEL,0,0);
    for(size_t i=0;i<catalog.entries.size();++i){const auto& info=catalog.entries[i].info;const bool earned=AchievementEarned(info.id);
      if((filter==1&&!earned)||(filter==2&&earned))continue;
      const auto text=Wide(info.label+" "+info.description+" "+info.unachieved_description);
      if(search.empty()||FindNLSStringEx(LOCALE_NAME_USER_DEFAULT,FIND_FROMSTART|NORM_IGNORECASE,text.c_str(),static_cast<int>(text.size()),search.c_str(),static_cast<int>(search.size()),nullptr,nullptr,nullptr,0)>=0)achievement_rows.push_back(i);
    }
    if(reset)achievement_scroll=0;InvalidateRect(achievement_list,nullptr,FALSE);
  }
  void PaintAchievementSummary(HDC dc){
    size_t earned=0;uint64_t score=0,total=0;for(const auto& entry:catalog.entries){total+=entry.info.gamerscore;if(AchievementEarned(entry.info.id)){++earned;score+=entry.info.gamerscore;}}
    const bool ready=!catalog.entries.empty()&&unlocks_known;
    SetBkMode(dc,TRANSPARENT);SelectObject(dc,bold);SetTextColor(dc,kText);
    auto text=ready?std::to_wstring(earned)+L" / "+std::to_wstring(catalog.entries.size())+T(L" unlocked",L" kazanıldı"):std::wstring(T(L"Loading your achievements...",L"Başarımlar yükleniyor..."));
    if(!unlocks_known&&unlocks_stale)text=T(L"Achievement progress unavailable",L"Başarım ilerlemesi okunamıyor");
    if(!catalog_error.empty())text=T(L"Achievement catalog unavailable",L"Başarım listesi yüklenemedi");
    RECT r{S(260),S(139),S(930),S(165)};DrawTextW(dc,text.c_str(),-1,&r,DT_SINGLELINE|DT_NOPREFIX);
    SelectObject(dc,small_font);SetTextColor(dc,kMuted);r={S(260),S(173),S(930),S(196)};
    text=ready?std::to_wstring(score)+L" / "+std::to_wstring(total)+L" G":T(L"Reading your existing game data",L"Mevcut oyun verileri okunuyor");DrawTextW(dc,text.c_str(),-1,&r,DT_SINGLELINE|DT_NOPREFIX);
    if(ready){SelectObject(dc,heading);SetTextColor(dc,kAccent);r={S(940),S(134),S(1070),S(186)};text=std::to_wstring(earned*100/catalog.entries.size())+L"%";DrawTextW(dc,text.c_str(),-1,&r,DT_RIGHT|DT_SINGLELINE);}
    RECT track{S(260),S(205),S(1070),S(211)};Rounded(dc,track,kField,kField,6);
    if(ready&&earned){track.right=track.left+static_cast<LONG>((track.right-track.left)*earned/catalog.entries.size());Rounded(dc,track,kAccent,kAccent,6);}
  }
  void PaintAchievementList(HDC target){
    RECT client;GetClientRect(achievement_list,&client);auto dc=CreateCompatibleDC(target);auto bitmap=CreateCompatibleBitmap(target,std::max<LONG>(1,client.right),std::max<LONG>(1,client.bottom));auto old=SelectObject(dc,bitmap);
    FillRect(dc,&client,card);SetBkMode(dc,TRANSPARENT);
    const auto description=[&](size_t index)->std::wstring{const auto& info=catalog.entries[index].info;return Wide(AchievementEarned(info.id)||info.unachieved_description.empty()?info.description:info.unachieved_description);};
    auto height=[&](size_t index){SelectObject(dc,font);RECT r{0,0,std::max<LONG>(S(80),client.right-S(205)),0};auto text=description(index);DrawTextW(dc,text.c_str(),-1,&r,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);return std::max<int>(S(98),r.bottom+S(58));};
    int total=0;for(auto index:achievement_rows)total+=height(index)+S(8);
    achievement_scroll=std::clamp(achievement_scroll,0,std::max<int>(0,total-client.bottom));
    SCROLLINFO scroll{sizeof(scroll),SIF_RANGE|SIF_PAGE|SIF_POS,0,std::max(0,total-1),static_cast<UINT>(client.bottom),achievement_scroll};SetScrollInfo(achievement_list,SB_VERT,&scroll,TRUE);
    int y=-achievement_scroll;
    for(auto index:achievement_rows){const int h=height(index);if(y+h>0&&y<client.bottom){
      const auto& info=catalog.entries[index].info;const bool earned=AchievementEarned(info.id);
      RECT row{0,y,client.right-S(4),y+h};Rounded(dc,row,kField,earned?RGB(101,78,41):kBorder,10);
      RECT icon{S(14),y+S(21),S(64),y+S(71)};
      if(index<achievement_icons.size()&&achievement_icons[index]){Gdiplus::Graphics g(dc);Gdiplus::ColorMatrix matrix={1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,earned?1.f:.38f,0,0,0,0,0,1};Gdiplus::ImageAttributes attr;attr.SetColorMatrix(&matrix);auto& image=*achievement_icons[index];g.DrawImage(&image,Gdiplus::Rect(icon.left,icon.top,icon.right-icon.left,icon.bottom-icon.top),0,0,image.GetWidth(),image.GetHeight(),Gdiplus::UnitPixel,&attr);}
      else {Rounded(dc,icon,kCard,kBorder,8);SelectObject(dc,heading);SetTextColor(dc,earned?kAccent:kMuted);DrawTextW(dc,L"★",-1,&icon,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
      SelectObject(dc,bold);SetTextColor(dc,kText);RECT r{S(82),y+S(13),client.right-S(117),y+S(37)};auto label=Wide(info.label);DrawTextW(dc,label.c_str(),-1,&r,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
      SelectObject(dc,font);SetTextColor(dc,kMuted);r={S(82),y+S(41),client.right-S(117),y+h-S(9)};auto text=description(index);DrawTextW(dc,text.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);
      SelectObject(dc,bold);SetTextColor(dc,earned?kAccent:kMuted);r={client.right-S(108),y+S(20),client.right-S(18),y+S(44)};text=std::to_wstring(info.gamerscore)+L" G";DrawTextW(dc,text.c_str(),-1,&r,DT_RIGHT|DT_SINGLELINE);
      SelectObject(dc,small_font);r.top=y+S(51);r.bottom=y+S(76);DrawTextW(dc,T(!unlocks_known?L"UNKNOWN":earned?L"UNLOCKED":L"LOCKED",!unlocks_known?L"BİLİNMİYOR":earned?L"KAZANILDI":L"KİLİTLİ"),-1,&r,DT_RIGHT|DT_SINGLELINE);
    }y+=h+S(8);}
    if(achievement_rows.empty()){SelectObject(dc,font);SetTextColor(dc,kMuted);RECT r{S(18),S(25),client.right-S(24),client.bottom-S(18)};
      const auto message=!catalog_error.empty()?catalog_error:catalog.entries.empty()?std::wstring(T(L"Loading achievements from your game folder...",L"Başarımlar oyun klasöründen yükleniyor...")):std::wstring(T(L"No achievements match this filter.",L"Bu filtreye uygun başarım bulunamadı."));DrawTextW(dc,message.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);}
    BitBlt(target,0,0,client.right,client.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
  }
  static LRESULT CALLBACK AchievementListProc(HWND control,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(control,&ps);app.PaintAchievementList(dc);EndPaint(control,&ps);return 0;}
    if(message==WM_GETDLGCODE)return DLGC_WANTARROWS;
    if(message==WM_LBUTTONDOWN){SetFocus(control);return 0;}
    if(message==WM_MOUSEWHEEL||message==WM_VSCROLL||message==WM_KEYDOWN){
      RECT r;GetClientRect(control,&r);auto next=app.achievement_scroll;
      if(message==WM_MOUSEWHEEL)next-=GET_WHEEL_DELTA_WPARAM(wparam)*app.S(42)/WHEEL_DELTA;
      else if(message==WM_KEYDOWN){if(wparam==VK_DOWN)next+=app.S(42);else if(wparam==VK_UP)next-=app.S(42);else if(wparam==VK_NEXT)next+=r.bottom;else if(wparam==VK_PRIOR)next-=r.bottom;else if(wparam==VK_HOME)next=0;else if(wparam==VK_END)next=INT_MAX;else return DefSubclassProc(control,message,wparam,lparam);}
      else {switch(LOWORD(wparam)){case SB_LINEUP:next-=app.S(42);break;case SB_LINEDOWN:next+=app.S(42);break;case SB_PAGEUP:next-=r.bottom;break;case SB_PAGEDOWN:next+=r.bottom;break;case SB_TOP:next=0;break;case SB_BOTTOM:next=INT_MAX;break;case SB_THUMBTRACK:case SB_THUMBPOSITION:{SCROLLINFO si{sizeof(si),SIF_TRACKPOS};GetScrollInfo(control,SB_VERT,&si);next=si.nTrackPos;break;}}}
      app.achievement_scroll=std::max(0,next);InvalidateRect(control,nullptr,FALSE);return 0;
    }
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,AchievementListProc,id);
    return DefSubclassProc(control,message,wparam,lparam);
  }
