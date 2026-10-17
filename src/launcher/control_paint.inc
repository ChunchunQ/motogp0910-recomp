  // Retain native BUTTON/TRACKBAR input, focus and accessibility, but own the
  // complete client paint. Native theme erasure and ancestor composition must
  // not expose an intermediate white/empty rectangle when a page moves.
  void DrawControl(HDC dc,HWND control){
    const int saved=SaveDC(dc);
    wchar_t cls[32]{};GetClassNameW(control,cls,32);
    if(std::wstring_view(cls)==TRACKBAR_CLASSW){
      NMCUSTOMDRAW draw{};draw.hdr.hwndFrom=control;draw.dwDrawStage=CDDS_PREPAINT;draw.hdc=dc;
      DrawSlider(draw);
    }else{
      DRAWITEMSTRUCT draw{};draw.CtlType=ODT_BUTTON;draw.CtlID=GetDlgCtrlID(control);
      draw.itemAction=ODA_DRAWENTIRE;draw.hwndItem=control;draw.hDC=dc;GetClientRect(control,&draw.rcItem);
      if(SendMessageW(control,BM_GETSTATE,0,0)&BST_PUSHED)draw.itemState|=ODS_SELECTED;
      if(GetFocus()==control)draw.itemState|=ODS_FOCUS;
      if(!IsWindowEnabled(control))draw.itemState|=ODS_DISABLED;
      DrawButton(draw);
    }
    RestoreDC(dc,saved);
  }
  void PaintControl(HDC target,HWND control){
    RECT r;GetClientRect(control,&r);if(r.right<=0||r.bottom<=0)return;
    const auto dc=CreateCompatibleDC(target);
    const auto bitmap=dc?CreateCompatibleBitmap(target,r.right,r.bottom):nullptr;
    if(!bitmap){if(dc)DeleteDC(dc);DrawControl(target,control);return;}
    const auto old=SelectObject(dc,bitmap);DrawControl(dc,control);
    BitBlt(target,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
  }
  static LRESULT CALLBACK PaintedControlProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT){PAINTSTRUCT ps;const auto dc=BeginPaint(control,&ps);app.PaintControl(dc,control);EndPaint(control,&ps);return 0;}
    if(message==WM_PRINTCLIENT){app.PaintControl(reinterpret_cast<HDC>(wp),control);return 0;}
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,PaintedControlProc,id);
    return DefSubclassProc(control,message,wp,lp);
  }
