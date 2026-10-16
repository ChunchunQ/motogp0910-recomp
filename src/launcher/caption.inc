  // A client-drawn title bar provides exactly minimize + close. Standard
  // Windows captions retain a disabled maximize slot even without MAXIMIZEBOX.
  int caption_hover=0,caption_pressed=0;
  struct CaptionArea {int width,height,border;};
  CaptionArea CaptionBounds()const{RECT r;GetClientRect(window,&r);return {r.right,S(kCaptionHeight),0};}
  int CaptionHit(POINT point)const{
    ScreenToClient(window,&point);const auto a=CaptionBounds();const int x=point.x,y=point.y;
    if(x<0||x>=a.width||y<0||y>=a.height)return 0;
    if(x>=a.width-S(46))return HTCLOSE;
    if(x>=a.width-S(92))return HTMINBUTTON;
    return HTCAPTION;
  }
  void DrawCaption(HDC dc){
    const auto a=CaptionBounds();RECT r{0,0,a.width,a.height};
    // Repaint the same full-window artwork through the caption clip. There is
    // no title strip or persistent rectangle behind minimize / close.
    const int saved=SaveDC(dc);IntersectClipRect(dc,0,0,a.width,a.height);DrawBackground(dc);RestoreDC(dc,saved);
    SetBkMode(dc,TRANSPARENT);auto old=SelectObject(dc,small_font);SetTextColor(dc,kText);
    for(const int hit:{HTMINBUTTON,HTCLOSE}){
      const int right=a.width-a.border-(hit==HTMINBUTTON?S(46):0);r={right-S(46),a.border,right,a.height};
      SetTextColor(dc,caption_hover==hit?(hit==HTCLOSE?kRed:kAccent):kText);
      DrawTextW(dc,hit==HTCLOSE?L"×":L"—",-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    }
    SelectObject(dc,old);
  }
  void PaintCaption(){
    if(!small_font||!IsWindowVisible(window)||IsIconic(window))return;
    const auto dc=GetDC(window);if(!dc)return;DrawCaption(dc);ReleaseDC(window,dc);
  }
  void SetupCaption(){if(auto menu=GetSystemMenu(window,FALSE))DeleteMenu(menu,SC_MAXIMIZE,MF_BYCOMMAND);}
  void CaptionMove(POINT screen){
    const int hit=CaptionHit(screen);const int hover=hit==HTCLOSE||hit==HTMINBUTTON?hit:0;
    if(hover!=caption_hover){caption_hover=hover;PaintCaption();}
  }
  void CaptionRelease(POINT screen){
    const int pressed=caption_pressed;caption_pressed=0;ReleaseCapture();CaptionMove(screen);
    if(pressed&&pressed==CaptionHit(screen)){if(pressed==HTCLOSE)SendMessageW(window,WM_CLOSE,0,0);else ShowWindow(window,SW_MINIMIZE);}
  }
