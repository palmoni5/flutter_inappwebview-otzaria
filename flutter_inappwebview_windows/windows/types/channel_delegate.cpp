#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>

#include "../utils/crash_log.h"
#include "../utils/safe_log.h"
#include "../utils/util.h"
#include "channel_delegate.h"

namespace flutter_inappwebview_plugin
{
  ChannelDelegate::ChannelDelegate(flutter::BinaryMessenger* messenger, const std::string& name) : messenger(messenger)
  {
    channel = std::make_shared<flutter::MethodChannel<flutter::EncodableValue>>(
      this->messenger, name,
      &flutter::StandardMethodCodec::GetInstance()
    );
    // Catch-all crash guard: EVERY MethodChannel derived from ChannelDelegate
    // routes through this lambda before reaching the (virtual) HandleMethodCall
    // on the derived class. Wrapping here covers all 14+ HandleMethodCall
    // implementations at once, including ones we don't know about.
    //
    // We log the channel name + method name BEFORE dispatch, so a subsequent
    // native crash leaves a clear breadcrumb showing the exact channel/method
    // active at the moment of death. The first call is via safeLog (Win32
    // only, cannot trip MSVCP140) so we capture the breadcrumb even on
    // installations where crashLog itself faults. The follow-up crashLog
    // call below adds the channel name for clearer correlation, but is
    // best-effort.
    // NOTE: the lambda body deliberately avoids std::string concatenation.
    // crashLog (which used `channelName + " :: " + methodName`) was the
    // observed trigger of the MSVCP140 14.36.32532.0 +0x12f58 null-read
    // crash on at least one user's installation: every method call hit
    // the same buggy MSVCP140 std::string code before HandleMethodCall
    // could run. safeLog only takes const char*; methodName.c_str() is
    // inline and does not dispatch into MSVCP140. The channel name is
    // logged once at construction below and isn't needed per-call.
    safeLog("CHANNEL_CTOR", name.c_str());
    channel->SetMethodCallHandler(
      [this](const auto& call, auto result)
      {
        const std::string& methodName = call.method_name();
        safeLog("CHANNEL_CALL", methodName.c_str());
        try {
          this->HandleMethodCall(call, std::move(result));
          safeLog("CHANNEL_DONE", methodName.c_str());
        }
        catch (const std::exception& e) {
          // `result` was moved into HandleMethodCall. If already consumed
          // (Success/Error called), nothing more to do. If not, Flutter may
          // hang waiting for a response, but the process stays alive.
          safeLog("CHANNEL_EXCEPTION", methodName.c_str());
        }
        catch (...) {
          safeLog("CHANNEL_EXCEPTION", methodName.c_str());
        }
      });
  }

  void ChannelDelegate::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
  {}

  void ChannelDelegate::UnregisterMethodCallHandler() const
  {
    if (channel) {
      channel->SetMethodCallHandler(nullptr);
    }
  }

  ChannelDelegate::~ChannelDelegate()
  {
    messenger = nullptr;
    channel.reset();
  }
}