#include "include/flutter_inappwebview_windows/flutter_inappwebview_windows_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "flutter_inappwebview_windows_plugin.h"
#include "utils/safe_log.h"

void FlutterInappwebviewWindowsPluginCApiRegisterWithRegistrar(
  FlutterDesktopPluginRegistrarRef registrar)
{
  // safeLog is Win32-only — see utils/safe_log.h. Even if the broader
  // crashLog path is currently jammed by the MSVCP140 +0x12f58 bug, this
  // line will reach disk and confirm Flutter actually loaded our DLL.
  flutter_inappwebview_plugin::safeLog(
    "C_API_REGISTER", "FlutterInappwebviewWindowsPluginCApiRegisterWithRegistrar");
  flutter_inappwebview_plugin::FlutterInappwebviewWindowsPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarManager::GetInstance()
    ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
