#ifndef FLUTTER_INAPPWEBVIEW_PLUGIN_CRASH_LOG_H_
#define FLUTTER_INAPPWEBVIEW_PLUGIN_CRASH_LOG_H_

#include <string>

namespace flutter_inappwebview_plugin
{
  // Always-on (Release + Debug) logger that appends a line to
  // %TEMP%\flutter_inappwebview_native.log. Intended for tracing the
  // sequence of native method handler invocations so a post-mortem
  // dump can be correlated against the last activity before the crash.
  //
  // Format: "<ISO timestamp> | <topic> | <message>\n"
  //
  // No-op on I/O failure — never throws, never blocks the caller.
  void crashLog(const std::string& topic, const std::string& message) noexcept;

  // Records the *intention* to handle a method, before it runs. Combined
  // with try/catch in the call site, this ensures that even native crashes
  // leave a trail pointing at the last method we touched.
  void crashLogMethodEnter(const std::string& method) noexcept;

  // Records that the method finished normally.
  void crashLogMethodExit(const std::string& method) noexcept;

  // Records a caught exception (the process did NOT die). The native string
  // exception type or generic info is included.
  void crashLogMethodException(const std::string& method,
                               const std::string& info) noexcept;
} // namespace flutter_inappwebview_plugin

#endif // FLUTTER_INAPPWEBVIEW_PLUGIN_CRASH_LOG_H_
