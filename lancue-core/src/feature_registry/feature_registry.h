#pragma once

#include <memory>
#include <vector>

#include "feature.h"

namespace lancue {

// Owns every IFeature instance lancue-core runs. main.cpp registers
// features here and calls startAll()/stopAll() once each — it never
// touches an individual feature after that (§4.4).
class FeatureRegistry {
public:
    // Takes ownership. Not yet started — startAll() does that for every
    // registered feature at once, in registration order.
    void registerFeature(std::unique_ptr<IFeature> feature);

    // Calls start() on every registered feature, in registration order.
    // A feature whose start() returns false is logged and left in the
    // registry (so stopAll() still calls its stop(), which must be safe
    // to call on a feature that never successfully started) rather than
    // aborting the whole daemon over one failed feature.
    void startAll();

    void stopAll();

private:
    std::vector<std::unique_ptr<IFeature>> m_features;
};

} // namespace lancue
