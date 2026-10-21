  // A client-drawn title bar provides minimize + close. The optional update
  // button is a separate native child control. Standard
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
    Gdiplus::Graphics graphics(dc);graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    for(const int hit:{HTMINBUTTON,HTCLOSE}){
      const int right=a.width-a.border-(hit==HTMINBUTTON?S(46):0);r={right-S(46),a.border,right,a.height};
      if(caption_hover==hit){
        const bool pressed=caption_pressed==hit;
        Gdiplus::SolidBrush tint(hit==HTCLOSE?Gdiplus::Color(pressed?210:170,232,45,39):Gdiplus::Color(pressed?65:35,255,255,255));
        graphics.FillRectangle(&tint,Gdiplus::Rect(r.left,r.top,r.right-r.left,r.bottom-r.top));
      }
      // Equal 10-logical-pixel vector symbols avoid a tiny multiplication glyph
      // or different font fallback on Windows. Both share the same stroke.
      const Gdiplus::REAL cx=(r.left+r.right)*.5f,cy=(r.top+r.bottom)*.5f,half=static_cast<float>(5*scale);
      Gdiplus::Pen pen(Gdiplus::Color(255,242,244,247),static_cast<float>(scale));
      if(hit==HTCLOSE){graphics.DrawLine(&pen,cx-half,cy-half,cx+half,cy+half);graphics.DrawLine(&pen,cx-half,cy+half,cx+half,cy-half);}
      else graphics.DrawLine(&pen,cx-half,cy-.5f,cx+half,cy-.5f);
    }
  }
  void PaintCaption(){
    if(!small_font||!IsWindowVisible(window)||IsIconic(window))return;
    const auto dc=GetDC(window);if(!dc)return;
    const auto a=CaptionBounds();const auto memory=CreateCompatibleDC(dc);const auto bitmap=CreateCompatibleBitmap(dc,a.width,a.height);const auto old=SelectObject(memory,bitmap);
    DrawCaption(memory);BitBlt(dc,0,0,a.width,a.height,memory,0,0,SRCCOPY);
    SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,dc);
  }
  void SetupCaption(){if(auto menu=GetSystemMenu(window,FALSE)){DeleteMenu(menu,SC_MAXIMIZE,MF_BYCOMMAND);AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,kNoticesCommand,T(L"Third-party notices…",L"Üçüncü taraf bildirimleri…"));}}
  void CaptionMove(POINT screen){
    const int hit=CaptionHit(screen);const int hover=hit==HTCLOSE||hit==HTMINBUTTON?hit:0;
    if(hover!=caption_hover){caption_hover=hover;PaintCaption();}
  }
  void CaptionRelease(POINT screen){
    const int pressed=caption_pressed;caption_pressed=0;ReleaseCapture();CaptionMove(screen);
    if(pressed&&pressed==CaptionHit(screen)){if(pressed==HTCLOSE)SendMessageW(window,WM_CLOSE,0,0);else ShowWindow(window,SW_MINIMIZE);}
  }
