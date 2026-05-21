#include "flutter_inappwebview_windows_plugin.h"

#include <flutter/plugin_registrar_windows.h>
#include <windows.h>

#include "cookie_manager.h"
#include "headless_in_app_webview/headless_in_app_webview_manager.h"
#include "in_app_browser/in_app_browser_manager.h"
#include "in_app_webview/in_app_webview_manager.h"
#include "platform_util.h"
#include "utils/safe_log.h"
#include "webview_environment/webview_environment_manager.h"


#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#pragma comment(lib, "WindowsApp.lib")

namespace flutter_inappwebview_plugin
{
  namespace
  {
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousSehFilter = nullptr;
    LONG g_sehFilterInstalled = 0;

    // Top-level SEH filter — last line of defense before the process dies.
    // Uses safeLog only (Win32-only, no STL/MSVCP140) because the very
    // crash we're trying to capture is the MSVCP140 +0x12f58 fault that
    // makes anything STL-based unreachable. We chain to any pre-existing
    // filter (e.g. Chromium / Sentry installed earlier) so we don't
    // disrupt other crash-reporting machinery.
    LONG WINAPI inappwebviewSehFilter(EXCEPTION_POINTERS* ep) noexcept
    {
      unsigned long long code = 0;
      unsigned long long addr = 0;
      if (ep && ep->ExceptionRecord) {
        code = (unsigned long long)ep->ExceptionRecord->ExceptionCode;
        addr = (unsigned long long)(ULONG_PTR)ep->ExceptionRecord->ExceptionAddress;
      }
      safeLogHex("UNHANDLED_SEH", "code", code, "addr", addr);
      if (g_previousSehFilter) return g_previousSehFilter(ep);
      return EXCEPTION_CONTINUE_SEARCH;
    }

    void installSehFilterOnce() noexcept
    {
      if (InterlockedCompareExchange(&g_sehFilterInstalled, 1, 0) != 0) return;
      g_previousSehFilter = SetUnhandledExceptionFilter(&inappwebviewSehFilter);
      safeLog("PLUGIN_INIT", "SetUnhandledExceptionFilter installed");
    }
  }

  // static
  void FlutterInappwebviewWindowsPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar)
  {
    safeLog("PLUGIN_INIT", "RegisterWithRegistrar begin");
    installSehFilterOnce();
    auto plugin = std::make_unique<FlutterInappwebviewWindowsPlugin>(registrar);
    registrar->AddPlugin(std::move(plugin));
    safeLog("PLUGIN_INIT", "RegisterWithRegistrar done");
  }

  FlutterInappwebviewWindowsPlugin::FlutterInappwebviewWindowsPlugin(flutter::PluginRegistrarWindows* registrar)
    : registrar(registrar)
  {
    webViewEnvironmentManager = std::make_unique<WebViewEnvironmentManager>(this);
    inAppWebViewManager = std::make_unique<InAppWebViewManager>(this);
    inAppBrowserManager = std::make_unique<InAppBrowserManager>(this);
    headlessInAppWebViewManager = std::make_unique<HeadlessInAppWebViewManager>(this);
    cookieManager = std::make_unique<CookieManager>(this);
    platformUtil = std::make_unique<PlatformUtil>(this);

    window_proc_id = registrar->RegisterTopLevelWindowProcDelegate(
      [this](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
      {
        return HandleWindowProc(hWnd, message, wParam, lParam);
      });
  }

  FlutterInappwebviewWindowsPlugin::~FlutterInappwebviewWindowsPlugin()
  {
    if (registrar) {
      registrar->UnregisterTopLevelWindowProcDelegate(window_proc_id);
    }
    webViewEnvironmentManager = nullptr;
    inAppWebViewManager = nullptr;
    inAppBrowserManager = nullptr;
    headlessInAppWebViewManager = nullptr;
    cookieManager = nullptr;
    platformUtil = nullptr;
  }


  std::optional<LRESULT> FlutterInappwebviewWindowsPlugin::HandleWindowProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
  {
    std::optional<LRESULT> result = std::nullopt;

    if (platformUtil) {
      result = platformUtil->HandleWindowProc(hWnd, message, wParam, lParam);
    }

    return result;
  }
}