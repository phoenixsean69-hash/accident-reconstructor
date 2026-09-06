#pragma once

// ROADSAFE_ASSET_LIBRARY_V1

#include <filesystem>
#include <string>
#include <vector>

#include "roadsafe_core.h"
#include "roadsafe_renderer.h"

namespace roadsafe
{

struct AssetLibraryEntry
{
    std::string assetId;
    std::string displayName;
    std::string category;
    std::string relativePath;

    std::string sourceUrl;
    std::string author;
    std::string licenseName;
    std::string attribution;
    std::string materialProfile;

    float metersPerUnit=1.0f;
    bool arReady=true;

    std::uintmax_t fileSizeBytes=0;
};

struct AssetLibraryState
{
    bool open=false;
    bool requestFocus=false;
    bool initialized=false;

    int selectedIndex=-1;
    int categoryIndex=0;

    char search[128]{};

    std::vector<AssetLibraryEntry> entries;

    std::string status=
        "Asset Library ready";
};

void refreshAssetLibrary(
    AssetLibraryState& state,
    const std::filesystem::path& assetRoot);

void drawAssetLibrary(
    AssetLibraryState& state,
    RoadSafeCase& caseData,
    int selectedEntityId,
    RoadSafeRenderer& renderer,
    const std::filesystem::path& assetRoot);

} // namespace roadsafe