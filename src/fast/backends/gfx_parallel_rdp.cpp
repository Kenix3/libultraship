#include "fast/backends/gfx_parallel_rdp.h"

#ifdef ENABLE_OPENGL

#include <utility>

namespace Fast {

const char* GfxRenderingAPIParallelRDP::GetName() {
    return "Vulkan (parallelrdp)";
}

bool GfxRenderingAPIParallelRDP::SupportsRawRdpCommands() const {
    return true;
}

void GfxRenderingAPIParallelRDP::SubmitRawRdpCommand(uint32_t w0, uint32_t w1) {
    if (mRawRdpCommandCallback) {
        mRawRdpCommandCallback(w0, w1);
    }
}

void GfxRenderingAPIParallelRDP::SetRawRdpCommandCallback(RawRdpCommandCallback callback) {
    mRawRdpCommandCallback = std::move(callback);
}

} // namespace Fast

#endif
