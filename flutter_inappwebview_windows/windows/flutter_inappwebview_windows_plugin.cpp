#include "flutter_inappwebview_windows_plugin.h"

#include <flutter/plugin_registrar_windows.h>
#include <windows.h>
#include <sstream>

#include "cookie_manager.h"
#include "headless_in_app_webview/headless_in_app_webview_manager.h"
#include "in_app_browser/in_app_browser_manager.h"
#include "in_app_webview/in_app_webview_manager.h"
#include "platform_util.h"
#include "utils/crash_log.h"
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
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousFilter = nullptr;

    // Top-level SEH exception filter. Fires for any unhandled structured
    // exception in the process — access violations, stack overflow, ABI
    // crashes from MSVCP140 raising past the plugin boundary, std::terminate
    // failures, etc. The Chromium browser process and main UI thread share a
    // single SetUnhandledExceptionFilter slot, so we chain to any previously
    // installed filter. This is our last line of defense after the more
    // targeted try/catch wraps in HandleMethodCall: if a thread inside
    // EmbeddedBrowserWebView.dll raises before any catch handler is reached,
    // we still leave a breadcrumb in the native log before the process dies.
    LONG WINAPI inappwebviewUnhandledFilter(EXCEPTION_POINTERS* ep) noexcept
    {
      try {
        std::ostringstream os;
        os << "code=0x" << std::hex
           << (ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionCode : 0u)
           << " addr=0x"
           << (ep && ep->ExceptionRecord
                 ? reinterpret_cast<uintptr_t>(ep->ExceptionRecord->ExceptionAddress)
                 : 0u);
        crashLog("UNHANDLED_SEH", os.str());
      } catch (...) {
        // Never let the filter itself crash.
      }
      if (g_previousFilter) {
        return g_previousFilter(ep);
      }
      return EXCEPTION_CONTINUE_SEARCH;
    }

    void installUnhandledFilterOnce() noexcept
    {
      static bool installed = false;
      if (installed) return;
      installed = true;
      try {
        g_previousFilter =
            SetUnhandledExceptionFilter(&inappwebviewUnhandledFilter);
        crashLog("PLUGIN_INIT", "SetUnhandledExceptionFilter installed");
      } catch (...) {
        // ignore
      }
    }
  }

  // static
  void FlutterInappwebviewWindowsPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar)
  {
    crashLog("PLUGIN_INIT", "RegisterWithRegistrar: begin");
    installUnhandledFilterOnce();
    // No try/catch here: with _HAS_EXCEPTIONS=0 the STL doesn't throw, and
    // rethrowing across the extern "C" plugin boundary into Flutter caused
    // silent process exits at startup. Native crashes from this point on
    // are captured by the SetUnhandledExceptionFilter installed above.
    auto plugin = std::make_unique<FlutterInappwebviewWindowsPlugin>(registrar);
    registrar->AddPlugin(std::move(plugin));
    crashLog("PLUGIN_INIT", "RegisterWithRegistrar: done");
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