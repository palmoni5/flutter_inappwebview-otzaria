#include "include/flutter_inappwebview_windows/flutter_inappwebview_windows_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "flutter_inappwebview_windows_plugin.h"
#include "utils/crash_log.h"

namespace
{
  // Global constructor that fires when the DLL is loaded into the process,
  // before any other plugin code runs. This is our earliest possible
  // logging point — if this line is missing from the native log after a
  // crash, it means either (a) the build deployed by the host application
  // does not contain this DLL, or (b) a static initializer earlier in the
  // C runtime has already terminated the process. Either way it is a
  // diagnostic ground truth that the crashLog mechanism itself is wired
  // up and reachable in this binary.
  struct DllLoadMarker {
    DllLoadMarker() noexcept
    {
      flutter_inappwebview_plugin::crashLog(
        "DLL_LOADED",
        "flutter_inappwebview_windows_plugin.dll attached to process");
    }
  };
  static DllLoadMarker g_dllLoadMarker;
}

void FlutterInappwebviewWindowsPluginCApiRegisterWithRegistrar(
  FlutterDesktopPluginRegistrarRef registrar)
{
  flutter_inappwebview_plugin::crashLog("C_API_REGISTER",
    "FlutterInappwebviewWindowsPluginCApiRegisterWithRegistrar called");
  flutter_inappwebview_plugin::FlutterInappwebviewWindowsPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarManager::GetInstance()
    ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
