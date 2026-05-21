#include "include/flutter_inappwebview_windows/flutter_inappwebview_windows_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "flutter_inappwebview_windows_plugin.h"
#include "utils/crash_log.h"

// NOTE: an earlier revision installed a global static `DllLoadMarker` that
// called crashLog() from DLL_PROCESS_ATTACH. Combined with C++ exceptions
// now being enabled (we removed _HAS_EXCEPTIONS=0), a std::string allocation
// inside that static initializer could throw under loader lock, propagate
// out, and abort DLL initialization — manifesting as STATUS_DLL_INIT_FAILED
// (0xc0000142). The C_API_REGISTER line below runs in the same role —
// confirming the DLL is loaded and Flutter has invoked it — but executes
// AFTER the loader is done, so it's safe.
void FlutterInappwebviewWindowsPluginCApiRegisterWithRegistrar(
  FlutterDesktopPluginRegistrarRef registrar)
{
  flutter_inappwebview_plugin::crashLog("C_API_REGISTER",
    "FlutterInappwebviewWindowsPluginCApiRegisterWithRegistrar called");
  flutter_inappwebview_plugin::FlutterInappwebviewWindowsPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarManager::GetInstance()
    ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
