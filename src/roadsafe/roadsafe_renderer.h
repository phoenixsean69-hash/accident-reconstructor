#pragma once

// ROADSAFE_NATIVE_GLTF_RENDERER_V1

#include <cstddef>
#include <filesystem>
#include <string>

#include <glad/glad.h>

#include "roadsafe_core.h"

namespace roadsafe
{

struct RenderSettings
{
    int width=1;
    int height=1;

    float fovDegrees=60.0f;

    // 0 Perspective, 1 Top, 2 Front, 3 Right.
    int viewPreset=0;

    // 0 Lit, 1 Wireframe, 2 Analysis.
    int renderMode=0;

    int selectedEntityId=0;
};

class RoadSafeRenderer
{
public:
    RoadSafeRenderer();
    ~RoadSafeRenderer();

    RoadSafeRenderer(
        const RoadSafeRenderer&)=delete;

    RoadSafeRenderer& operator=(
        const RoadSafeRenderer&)=delete;

    bool render(
        const RoadSafeCase& caseData,
        const RenderSettings& settings,
        const std::filesystem::path& assetRoot);

    void shutdown();
    void clearAssetCache();

    GLuint colorTexture() const;

    std::size_t loadedAssetCount() const;
    std::size_t renderedEntityCount() const;

    const std::string& status() const;

private:
    struct Impl;
    Impl* impl_=nullptr;
};

} // namespace roadsafe
