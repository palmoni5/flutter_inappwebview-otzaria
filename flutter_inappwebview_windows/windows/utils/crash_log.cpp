#include "crash_log.h"

#include <windows.h>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace flutter_inappwebview_plugin
{
  namespace
  {
    std::mutex& logMutex()
    {
      static std::mutex m;
      return m;
    }

    std::string logFilePath() noexcept
    {
      // %TEMP%\flutter_inappwebview_native.log
      // Resolved once and cached; if TEMP isn't set (very unusual), fall back
      // to the current directory.
      static std::string cached;
      static bool resolved = false;
      if (resolved) return cached;
      try {
        wchar_t buf[MAX_PATH + 1] = { 0 };
        DWORD n = GetTempPathW(MAX_PATH, buf);
        if (n == 0 || n > MAX_PATH) {
          cached = "flutter_inappwebview_native.log";
        } else {
          // narrow conversion via WideCharToMultiByte
          int needed = WideCharToMultiByte(CP_UTF8, 0, buf, -1, nullptr, 0,
                                            nullptr, nullptr);
          if (needed > 1) {
            std::string narrow(needed - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, buf, -1, narrow.data(), needed,
                                nullptr, nullptr);
            cached = narrow + "flutter_inappwebview_native.log";
          } else {
            cached = "flutter_inappwebview_native.log";
          }
        }
      } catch (...) {
        cached = "flutter_inappwebview_native.log";
      }
      resolved = true;
      return cached;
    }

    std::string isoTimestamp() noexcept
    {
      try {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream os;
        os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
           << '.' << std::setw(3) << std::setfill('0') << ms.count();
        return os.str();
      } catch (...) {
        return "?";
      }
    }
  } // namespace

  void crashLog(const std::string& topic, const std::string& message) noexcept
  {
    try {
      std::lock_guard<std::mutex> lock(logMutex());
      const std::string path = logFilePath();
      // O_APPEND open via fopen("ab") — flushed and closed on every write so
      // a subsequent native crash still leaves the data on disk.
      FILE* f = nullptr;
#ifdef _WIN32
      fopen_s(&f, path.c_str(), "ab");
#else
      f = std::fopen(path.c_str(), "ab");
#endif
      if (!f) return;
      const std::string line =
          isoTimestamp() + " | " + topic + " | " + message + "\n";
      std::fwrite(line.data(), 1, line.size(), f);
      std::fflush(f);
      std::fclose(f);
    } catch (...) {
      // never propagate
    }
  }

  void crashLogMethodEnter(const std::string& method) noexcept
  {
    crashLog("METHOD_ENTER", method);
  }

  void crashLogMethodExit(const std::string& method) noexcept
  {
    crashLog("METHOD_EXIT", method);
  }

  void crashLogMethodException(const std::string& method,
                               const std::string& info) noexcept
  {
    crashLog("METHOD_EXCEPTION", method + " :: " + info);
  }
} // namespace flutter_inappwebview_plugin
