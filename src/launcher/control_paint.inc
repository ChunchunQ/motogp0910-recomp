  // Retain native BUTTON/TRACKBAR input, focus and accessibility, but own the
  // complete client paint. Native theme erasure and ancestor composition must
  // not expose an intermediate white/empty rectangle when a page moves.
  void DrawControl(HDC dc,HWND control,const DRAWITEMSTRUCT* state=nullptr){
    const int saved=SaveDC(dc);
    wchar_t cls[32]{};GetClassNameW(control,cls,32);
    if(std::wstring_view(cls)==TRACKBAR_CLASSW){
      NMCUSTOMDRAW draw{};draw.hdr.hwndFrom=control;draw.dwDrawStage=CDDS_PREPAINT;draw.hdc=dc;
      DrawSlider(draw);
    }else if(lstrcmpiW(cls,L"COMBOBOX")==0){
      DrawComboControl(dc,control);
    }else{
      DRAWITEMSTRUCT draw{};draw.CtlType=ODT_BUTTON;draw.CtlID=GetDlgCtrlID(control);
      if(state)draw=*state;
      draw.itemAction=ODA_DRAWENTIRE;draw.hwndItem=control;draw.hDC=dc;GetClientRect(control,&draw.rcItem);
      if(!state){
        if(SendMessageW(control,BM_GETSTATE,0,0)&BST_PUSHED)draw.itemState|=ODS_SELECTED;
        if(GetFocus()==control)draw.itemState|=ODS_FOCUS;
        if(!IsWindowEnabled(control))draw.itemState|=ODS_DISABLED;
      }
      DrawButton(draw);
    }
    RestoreDC(dc,saved);
  }
  void PaintControl(HDC target,HWND control,const DRAWITEMSTRUCT* state=nullptr){
    RECT r;GetClientRect(control,&r);
    PaintBuffered(target,r,[&](HDC dc,RECT){DrawControl(dc,control,state);});
  }
  static LRESULT CALLBACK PaintedControlProc(HWND control,UINT message,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){
    auto& app=*reinterpret_cast<App*>(data);
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT){PAINTSTRUCT ps;const auto dc=BeginPaint(control,&ps);app.PaintControl(dc,control);EndPaint(control,&ps);return 0;}
    if(message==WM_PRINTCLIENT){app.PaintControl(reinterpret_cast<HDC>(wp),control);return 0;}
    if(message==WM_NCDESTROY)RemoveWindowSubclass(control,PaintedControlProc,id);
    return DefSubclassProc(control,message,wp,lp);
  }
