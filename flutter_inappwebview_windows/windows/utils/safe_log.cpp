#include "safe_log.h"

#include <windows.h>

namespace flutter_inappwebview_plugin
{
  namespace
  {
    // Lazily initialized CRITICAL_SECTION. We can't rely on a C++ static
    // for the mutex (would invoke STL machinery via std::mutex) so we
    // manage initialization manually with Interlocked* primitives.
    CRITICAL_SECTION g_cs;
    LONG g_csState = 0;  // 0 = uninit, 1 = ready

    void ensureCsInitialized() noexcept
    {
      if (InterlockedCompareExchange(&g_csState, 0, 0) == 1) return;
      // CRITICAL_SECTION init is cheap and idempotent enough that racing
      // threads briefly re-initing it is harmless; we just need at most
      // one thread to perform the InitializeCriticalSection call.
      static LONG initLatch = 0;
      if (InterlockedCompareExchange(&initLatch, 1, 0) == 0) {
        InitializeCriticalSection(&g_cs);
        InterlockedExchange(&g_csState, 1);
      } else {
        // Spin until the initializing thread flips the state.
        while (InterlockedCompareExchange(&g_csState, 0, 0) != 1) {
          Sleep(0);
        }
      }
    }

    int copyStr(char* dst, int cap, const char* s) noexcept
    {
      if (!s || cap <= 0) return 0;
      int w = 0;
      while (s[w] && w < cap) { dst[w] = s[w]; ++w; }
      return w;
    }

    int writeHex64(char* dst, int cap, unsigned long long v) noexcept
    {
      static const char hex[] = "0123456789abcdef";
      if (cap < 3) return 0;
      char tmp[17];
      int n = 0;
      if (v == 0) { tmp[n++] = '0'; }
      else { while (v != 0) { tmp[n++] = hex[v & 0xF]; v >>= 4; } }
      int w = 0;
      dst[w++] = '0';
      dst[w++] = 'x';
      while (n > 0 && w < cap) { dst[w++] = tmp[--n]; }
      return w;
    }

    int writeTimestamp(char* dst, int cap) noexcept
    {
      if (cap < 23) return 0;
      SYSTEMTIME st;
      GetLocalTime(&st);
      int w = 0;
      dst[w++] = (char)('0' + (st.wYear / 1000) % 10);
      dst[w++] = (char)('0' + (st.wYear / 100) % 10);
      dst[w++] = (char)('0' + (st.wYear / 10) % 10);
      dst[w++] = (char)('0' + st.wYear % 10);
      dst[w++] = '-';
      dst[w++] = (char)('0' + (st.wMonth / 10) % 10);
      dst[w++] = (char)('0' + st.wMonth % 10);
      dst[w++] = '-';
      dst[w++] = (char)('0' + (st.wDay / 10) % 10);
      dst[w++] = (char)('0' + st.wDay % 10);
      dst[w++] = 'T';
      dst[w++] = (char)('0' + (st.wHour / 10) % 10);
      dst[w++] = (char)('0' + st.wHour % 10);
      dst[w++] = ':';
      dst[w++] = (char)('0' + (st.wMinute / 10) % 10);
      dst[w++] = (char)('0' + st.wMinute % 10);
      dst[w++] = ':';
      dst[w++] = (char)('0' + (st.wSecond / 10) % 10);
      dst[w++] = (char)('0' + st.wSecond % 10);
      dst[w++] = '.';
      dst[w++] = (char)('0' + (st.wMilliseconds / 100) % 10);
      dst[w++] = (char)('0' + (st.wMilliseconds / 10) % 10);
      dst[w++] = (char)('0' + st.wMilliseconds % 10);
      return w;
    }

    HANDLE openLogFile() noexcept
    {
      wchar_t path[MAX_PATH + 64];
      DWORD n = GetTempPathW(MAX_PATH, path);
      if (n == 0 || n > MAX_PATH) return INVALID_HANDLE_VALUE;
      static const wchar_t kName[] = L"flutter_inappwebview_native.log";
      DWORD i = 0;
      while (kName[i] != L'\0' && n + i + 1 < MAX_PATH + 64) {
        path[n + i] = kName[i];
        ++i;
      }
      path[n + i] = L'\0';
      return CreateFileW(path, FILE_APPEND_DATA,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                         OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    void emit(HANDLE h, const char* buf, int len) noexcept
    {
      if (len <= 0) return;
      DWORD written = 0;
      WriteFile(h, buf, (DWORD)len, &written, nullptr);
    }
  } // namespace

  void safeLog(const char* topic, const char* message) noexcept
  {
    ensureCsInitialized();
    EnterCriticalSection(&g_cs);
    HANDLE h = openLogFile();
    if (h != INVALID_HANDLE_VALUE) {
      char line[768];
      int cap = (int)sizeof(line);
      int w = writeTimestamp(line, cap);
      w += copyStr(line + w, cap - w, " | ");
      w += copyStr(line + w, cap - w, topic ? topic : "(null)");
      w += copyStr(line + w, cap - w, " | ");
      w += copyStr(line + w, cap - w, message ? message : "(null)");
      w += copyStr(line + w, cap - w, "\r\n");
      emit(h, line, w);
      FlushFileBuffers(h);
      CloseHandle(h);
    }
    LeaveCriticalSection(&g_cs);
  }

  void safeLogHex(const char* topic,
                  const char* label1, unsigned long long v1,
                  const char* label2, unsigned long long v2) noexcept
  {
    ensureCsInitialized();
    EnterCriticalSection(&g_cs);
    HANDLE h = openLogFile();
    if (h != INVALID_HANDLE_VALUE) {
      char line[256];
      int cap = (int)sizeof(line);
      int w = writeTimestamp(line, cap);
      w += copyStr(line + w, cap - w, " | ");
      w += copyStr(line + w, cap - w, topic ? topic : "(null)");
      w += copyStr(line + w, cap - w, " | ");
      if (label1) {
        w += copyStr(line + w, cap - w, label1);
        w += copyStr(line + w, cap - w, "=");
        w += writeHex64(line + w, cap - w, v1);
      }
      if (label2) {
        w += copyStr(line + w, cap - w, " ");
        w += copyStr(line + w, cap - w, label2);
        w += copyStr(line + w, cap - w, "=");
        w += writeHex64(line + w, cap - w, v2);
      }
      w += copyStr(line + w, cap - w, "\r\n");
      emit(h, line, w);
      FlushFileBuffers(h);
      CloseHandle(h);
    }
    LeaveCriticalSection(&g_cs);
  }
} // namespace flutter_inappwebview_plugin
