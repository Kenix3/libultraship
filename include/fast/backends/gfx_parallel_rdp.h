#pragma once

#ifdef ENABLE_OPENGL

#include <functional>
#include "gfx_opengl.h"

namespace Fast {

class GfxRenderingAPIParallelRDP final : public GfxRenderingAPIOGL {
  public:
    using RawRdpCommandCallback = std::function<void(uint32_t, uint32_t)>;

    const char* GetName() override;
    bool SupportsRawRdpCommands() const override;
    void SubmitRawRdpCommand(uint32_t w0, uint32_t w1) override;

    void SetRawRdpCommandCallback(RawRdpCommandCallback callback);

  private:
    RawRdpCommandCallback mRawRdpCommandCallback;
};

} // namespace Fast

#endif
