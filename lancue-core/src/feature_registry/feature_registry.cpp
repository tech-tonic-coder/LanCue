#include "feature_registry.h"

#include "corelib/logging/logger.h"

namespace lancue {

void FeatureRegistry::registerFeature(std::unique_ptr<IFeature> feature) {
    m_features.push_back(std::move(feature));
}

void FeatureRegistry::startAll() {
    for (auto& feature : m_features) {
        if (!feature->start()) {
            logError(QStringLiteral("FeatureRegistry: a feature failed to start; continuing with the rest."));
        }
    }
}

void FeatureRegistry::stopAll() {
    for (auto& feature : m_features) {
        feature->stop();
    }
}

} // namespace lancue
