#pragma once

namespace renderlab {

enum class LightType {
    Directional,
    Point,
    Spot,
};

enum class ShadowTechnique {
    None,
    Hard,
    Pcf,
};

} // namespace renderlab
