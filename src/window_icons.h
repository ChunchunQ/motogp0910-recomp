#pragma once
#include <initializer_list>
#ifdef _WIN32
#include <Windows.h>
namespace motogp {
inline constexpr int kIconResource = 101;
inline void ApplyWindowIcon(void* native_window) {
  const auto window = static_cast<HWND>(native_window);
  if (!window) return;
  const auto instance = GetModuleHandleW(nullptr);
  for (const auto kind : {ICON_BIG, ICON_SMALL}) {
    const auto icon = LoadImageW(instance, MAKEINTRESOURCEW(kIconResource), IMAGE_ICON,
        GetSystemMetrics(kind == ICON_BIG ? SM_CXICON : SM_CXSMICON),
        GetSystemMetrics(kind == ICON_BIG ? SM_CYICON : SM_CYSMICON), LR_SHARED);
    if (icon) SendMessageW(window, WM_SETICON, kind, reinterpret_cast<LPARAM>(icon));
  }
}
}
#else
namespace motogp { inline void ApplyWindowIcon(void*) {} }
#endif
