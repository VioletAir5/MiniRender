#pragma once

#include <cstdint>

namespace renderlab {

enum class CompareOperation {
    Always,
    NotEqual,
};

enum class StencilOperation {
    Keep,
    Replace,
};

struct StencilState {
    bool enabled{false};
    CompareOperation comparison{CompareOperation::Always};
    StencilOperation stencilFail{StencilOperation::Keep};
    StencilOperation depthFail{StencilOperation::Keep};
    StencilOperation pass{StencilOperation::Keep};
    std::uint32_t reference{0};
    std::uint32_t readMask{0xFF};
    std::uint32_t writeMask{0xFF};
};

} // namespace renderlab
