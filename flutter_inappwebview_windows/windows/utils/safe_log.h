#ifndef FLUTTER_INAPPWEBVIEW_PLUGIN_SAFE_LOG_H_
#define FLUTTER_INAPPWEBVIEW_PLUGIN_SAFE_LOG_H_

namespace flutter_inappwebview_plugin
{
  // Win32-only structured logger that writes a single line to
  // %TEMP%\flutter_inappwebview_native.log using exclusively CreateFileW /
  // WriteFile / CloseHandle and Win32 time + arithmetic APIs.
  //
  // Crucially, this code touches NO STL types — no std::string, no
  // std::ostringstream, no std::mutex, no CRT file I/O — so it cannot
  // trigger the MSVCP140.dll +0x12f58 access violation observed on some
  // Windows installations that have crashed every previous logging
  // attempt before a single line could reach disk. Inputs are plain C
  // strings and integers. Safe to call from any thread, including
  // SetUnhandledExceptionFilter callbacks. Never throws.
  void safeLog(const char* topic, const char* message) noexcept;

  // Variant that appends two hex-formatted unsigned values labelled by
  // their respective `labelN`. Used by the SEH filter to record exception
  // code + faulting address without pulling in printf machinery.
  void safeLogHex(const char* topic,
                  const char* label1, unsigned long long v1,
                  const char* label2, unsigned long long v2) noexcept;
} // namespace flutter_inappwebview_plugin

#endif // FLUTTER_INAPPWEBVIEW_PLUGIN_SAFE_LOG_H_
