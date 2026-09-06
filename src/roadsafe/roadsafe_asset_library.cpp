// ROADSAFE_ASSET_LIBRARY_V1
// ROADSAFE_ASSET_LIBRARY_V2

#include "roadsafe_asset_library.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <system_error>
#include <vector>

namespace roadsafe
{
namespace
{

RoadSafeRenderer gAssetPreviewRenderer;

static std::string trim(
    const std::string& value)
{
    std::size_t first=0;

    while (first<value.size() &&
           std::isspace(
               static_cast<unsigned char>(
                   value[first]
               )))
    {
        ++first;
    }

    std::size_t last=value.size();

    while (last>first &&
           std::isspace(
               static_cast<unsigned char>(
                   value[last-1]
               )))
    {
        --last;
    }

    return value.substr(
        first,
        last-first
    );
}

static std::string lower(
    std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c)
        {
            return
                static_cast<char>(
                    std::tolower(c)
                );
        }
    );

    return value;
}

static std::string normalizedWords(
    const std::string& value)
{
    std::string result;
    result.reserve(
        value.size()+2
    );

    result.push_back(' ');

    bool previousWasSpace=true;

    for (unsigned char c : value)
    {
        if (std::isalnum(c))
        {
            result.push_back(
                static_cast<char>(
                    std::tolower(c)
                )
            );

            previousWasSpace=false;
        }
        else if (!previousWasSpace)
        {
            result.push_back(' ');
            previousWasSpace=true;
        }
    }

    if (result.empty() ||
        result.back()!=' ')
    {
        result.push_back(' ');
    }

    return result;
}

static bool containsPhrase(
    const std::string& normalizedText,
    const char* phrase)
{
    if (!phrase || !phrase[0])
        return false;

    const std::string needle=
        normalizedWords(
            phrase
        );

    return
        normalizedText.find(
            needle
        )!=
        std::string::npos;
}

static bool containsAnyPhrase(
    const std::string& normalizedText,
    std::initializer_list<const char*> phrases)
{
    for (const char* phrase : phrases)
    {
        if (containsPhrase(
                normalizedText,
                phrase))
        {
            return true;
        }
    }

    return false;
}

static bool parseBool(
    const std::string& value,
    bool fallback)
{
    const std::string normalized=
        lower(trim(value));

    if (normalized=="true" ||
        normalized=="1" ||
        normalized=="yes" ||
        normalized=="on")
    {
        return true;
    }

    if (normalized=="false" ||
        normalized=="0" ||
        normalized=="no" ||
        normalized=="off")
    {
        return false;
    }

    return fallback;
}

static float parseFloat(
    const std::string& value,
    float fallback)
{
    try
    {
        return std::stof(
            trim(value)
        );
    }
    catch (...)
    {
        return fallback;
    }
}

static std::string titleFromStem(
    std::string value)
{
    for (char& c : value)
    {
        if (c=='_' ||
            c=='-')
        {
            c=' ';
        }
    }

    bool uppercaseNext=true;

    for (char& c : value)
    {
        if (c==' ')
        {
            uppercaseNext=true;
            continue;
        }

        if (uppercaseNext)
        {
            c=
                static_cast<char>(
                    std::toupper(
                        static_cast<
                            unsigned char
                        >(c)
                    )
                );

            uppercaseNext=false;
        }
    }

    return value;
}

static std::string normalizedRelativePath(
    const std::filesystem::path& path)
{
    return
        path.generic_string();
}

static std::string canonicalCategory(
    const std::string& value)
{
    const std::string normalized=
        normalizedWords(
            value
        );

    if (containsAnyPhrase(
            normalized,
            {
                "vehicle",
                "vehicles"
            }))
    {
        return "Vehicles";
    }

    if (containsAnyPhrase(
            normalized,
            {
                "people",
                "person",
                "persons",
                "human",
                "humans"
            }))
    {
        return "People";
    }

    if (containsAnyPhrase(
            normalized,
            {
                "road infrastructure",
                "infrastructure",
                "roads"
            }))
    {
        return "Road Infrastructure";
    }

    if (containsAnyPhrase(
            normalized,
            {
                "environment",
                "nature",
                "terrain"
            }))
    {
        return "Environment";
    }

    if (containsAnyPhrase(
            normalized,
            {
                "building",
                "buildings",
                "architecture"
            }))
    {
        return "Buildings";
    }

    if (containsPhrase(
            normalized,
            "evidence"))
    {
        return "Evidence";
    }

    if (containsPhrase(
            normalized,
            "street furniture"))
    {
        return "Street Furniture";
    }

    if (containsPhrase(
            normalized,
            "forensic marker"))
    {
        return "Forensic Markers";
    }

    if (containsAnyPhrase(
            normalized,
            {
                "props",
                "misc",
                "miscellaneous"
            }))
    {
        return "Props / Misc";
    }

    if (containsPhrase(
            normalized,
            "imported"))
    {
        return "Imported";
    }

    return "";
}

static std::string classifyStrongText(
    const std::string& normalizedText)
{
    if (containsAnyPhrase(
            normalizedText,
            {
                "human",
                "person",
                "people",
                "pedestrian",
                "character",
                "male",
                "female",
                "woman",
                "man",
                "child",
                "worker",
                "survivor",
                "cyclist"
            }))
    {
        return "People";
    }

    if (containsAnyPhrase(
            normalizedText,
            {
                "sedan",
                "suv",
                "hatchback",
                "car",
                "vehicle",
                "truck",
                "firetruck",
                "fire truck",
                "haulage",
                "lorry",
                "tanker",
                "tractor",
                "trailer",
                "pickup",
                "bus",
                "coach",
                "minibus",
                "kombi",
                "van",
                "ambulance",
                "taxi",
                "motorcycle",
                "motorbike",
                "bicycle",
                "bike",
                "scooter",
                "kart"
            }))
    {
        return "Vehicles";
    }

    if (containsAnyPhrase(
            normalizedText,
            {
                "evidence marker",
                "scene marker",
                "survey marker",
                "forensic marker",
                "scale marker"
            }))
    {
        return "Forensic Markers";
    }

    if (containsAnyPhrase(
            normalizedText,
            {
                "debris",
                "skid",
                "tire",
                "tyre",
                "wheel rim",
                "bumper",
                "shard",
                "broken glass",
                "gouge"
            }))
    {
        return "Evidence";
    }

    if (containsAnyPhrase(
            normalizedText,
            {
                "road",
                "street",
                "asphalt",
                "tar road",
                "dirt road",
                "gravel road",
                "intersection",
                "crosswalk",
                "pedestrian crossing",
                "curb",
                "kerb",
                "sidewalk",
                "bridge",
                "highway",
                "guardrail",
                "guard rail",
                "barrier",
                "bollard",
                "traffic",
                "lane",
                "road sign",
                "traffic sign",
                "traffic cone"
            }))
    {
        return "Road Infrastructure";
    }

    if (containsAnyPhrase(
            normalizedText,
            {
                "tree",
                "bush",
                "plant",
                "grass",
                "forest",
                "nature",
                "rock",
                "stone",
                "terrain",
                "ground",
                "soil",
                "sand",
                "mud",
                "mountain",
                "hill",
                "hedge",
                "flower",
                "water",
                "river"
            }))
    {
        return "Environment";
    }

    if (containsAnyPhrase(
            normalizedText,
            {
                "building",
                "house",
                "apartment",
                "office",
                "shop",
                "store",
                "warehouse",
                "factory",
                "garage",
                "hangar",
                "school",
                "hospital",
                "church",
                "tower"
            }))
    {
        return "Buildings";
    }

    if (containsAnyPhrase(
            normalizedText,
            {
                "bench",
                "street lamp",
                "lamp post",
                "lamppost",
                "hydrant",
                "mailbox",
                "trash bin",
                "garbage bin",
                "street light",
                "streetlight",
                "parking meter"
            }))
    {
        return "Street Furniture";
    }

    if (containsAnyPhrase(
            normalizedText,
            {
                "barrel",
                "crate",
                "box",
                "pallet",
                "container",
                "table",
                "chair",
                "fence",
                "tool",
                "prop",
                "furniture"
            }))
    {
        return "Props / Misc";
    }

    return "";
}

static std::string inferAssetCategory(
    const AssetLibraryEntry& entry)
{
    const std::string modelText=
        normalizedWords(
            entry.displayName
        );

    const std::string pathText=
        normalizedWords(
            entry.relativePath
        );

    // Model names are the strongest signal. This prevents a tree inside
    // a city pack from being classified as a building, for example.
    const std::string modelCategory=
        classifyStrongText(
            modelText
        );

    if (!modelCategory.empty())
        return modelCategory;

    const std::string pathCategory=
        classifyStrongText(
            pathText
        );

    if (!pathCategory.empty())
        return pathCategory;

    // Pack-level hints are intentionally weaker than model-name hints.
    if (containsAnyPhrase(
            pathText,
            {
                "nature kit",
                "nature",
                "forest",
                "terrain"
            }))
    {
        return "Environment";
    }

    if (containsAnyPhrase(
            pathText,
            {
                "road kit",
                "city roads",
                "traffic"
            }))
    {
        return "Road Infrastructure";
    }

    if (containsAnyPhrase(
            pathText,
            {
                "car kit",
                "vehicle",
                "racing kit"
            }))
    {
        return "Vehicles";
    }

    if (containsAnyPhrase(
            pathText,
            {
                "character",
                "human",
                "people"
            }))
    {
        return "People";
    }

    if (containsAnyPhrase(
            pathText,
            {
                "city kit",
                "suburban",
                "industrial",
                "commercial",
                "building"
            }))
    {
        return "Buildings";
    }

    const std::string sidecarCategory=
        canonicalCategory(
            entry.category
        );

    if (!sidecarCategory.empty())
        return sidecarCategory;

    return "Props / Misc";
}

static void buildSearchText(
    AssetLibraryEntry& entry)
{
    entry.searchText=
        lower(
            entry.displayName+
            " "+
            entry.category+
            " "+
            entry.licenseName+
            " "+
            entry.author+
            " "+
            entry.materialProfile+
            " "+
            entry.relativePath+
            " "+
            entry.format+
            " "+
            entry.sourceUrl
        );
}

static bool searchTokenMatches(
    const AssetLibraryEntry& entry,
    const std::string& token)
{
    const std::size_t separator=
        token.find(':');

    if (separator!=
        std::string::npos)
    {
        const std::string field=
            token.substr(
                0,
                separator
            );

        const std::string value=
            token.substr(
                separator+1
            );

        if (value.empty())
            return true;

        if (field=="category" ||
            field=="cat")
        {
            return
                lower(entry.category).
                    find(value)!=
                std::string::npos;
        }

        if (field=="license" ||
            field=="licence")
        {
            return
                lower(entry.licenseName).
                    find(value)!=
                std::string::npos;
        }

        if (field=="author")
        {
            return
                lower(entry.author).
                    find(value)!=
                std::string::npos;
        }

        if (field=="format")
        {
            return
                lower(entry.format).
                    find(value)!=
                std::string::npos;
        }

        if (field=="source")
        {
            return
                lower(entry.sourceUrl).
                    find(value)!=
                std::string::npos;
        }
    }

    return
        entry.searchText.find(token)!=
        std::string::npos;
}

static bool entryMatchesSearch(
    const AssetLibraryEntry& entry,
    const char* search)
{
    if (!search ||
        !search[0])
    {
        return true;
    }

    std::istringstream stream(
        lower(search)
    );

    std::string token;

    while (stream>>token)
    {
        if (!searchTokenMatches(
                entry,
                token))
        {
            return false;
        }
    }

    return true;
}

static constexpr std::array<
    const char*,
    11
> kCategories={
    "All",
    "Vehicles",
    "People",
    "Road Infrastructure",
    "Environment",
    "Buildings",
    "Evidence",
    "Street Furniture",
    "Forensic Markers",
    "Props / Misc",
    "Imported"
};

static constexpr std::array<
    const char*,
    5
> kLicenseFilters={
    "All licences",
    "CC0",
    "CC BY",
    "Other licensed",
    "Unspecified"
};

static constexpr std::array<
    const char*,
    3
> kFormatFilters={
    "All formats",
    "GLB",
    "glTF"
};

static constexpr std::array<
    const char*,
    4
> kSortModes={
    "Name A-Z",
    "Category",
    "Size: largest",
    "Size: smallest"
};

static bool entryMatchesCategory(
    const AssetLibraryEntry& entry,
    int categoryIndex)
{
    if (categoryIndex<=0)
        return true;

    categoryIndex=
        std::max(
            0,
            std::min(
                static_cast<int>(
                    kCategories.size()
                )-1,
                categoryIndex
            )
        );

    return
        entry.category==
        kCategories[
            static_cast<std::size_t>(
                categoryIndex
            )
        ];
}

static bool entryMatchesLicense(
    const AssetLibraryEntry& entry,
    int licenseIndex)
{
    if (licenseIndex<=0)
        return true;

    const std::string license=
        lower(
            entry.licenseName
        );

    const bool unspecified=
        trim(license).empty() ||
        license=="unspecified";

    const bool cc0=
        license.find("cc0")!=
            std::string::npos ||
        license.find(
            "creative commons zero"
        )!=
            std::string::npos;

    const bool ccBy=
        !cc0 &&
        (
            license.find("cc by")!=
                std::string::npos ||
            license.find(
                "creative commons attribution"
            )!=
                std::string::npos
        );

    switch (licenseIndex)
    {
        case 1:
            return cc0;

        case 2:
            return ccBy;

        case 3:
            return
                !unspecified &&
                !cc0 &&
                !ccBy;

        case 4:
            return unspecified;

        default:
            return true;
    }
}

static bool entryMatchesFormat(
    const AssetLibraryEntry& entry,
    int formatIndex)
{
    if (formatIndex<=0)
        return true;

    const std::string format=
        lower(
            entry.format
        );

    if (formatIndex==1)
        return format=="glb";

    if (formatIndex==2)
        return format=="gltf";

    return true;
}

static const char* kindName(
    SceneEntityKind kind)
{
    switch (kind)
    {
        case SceneEntityKind::Environment:
            return "Environment";

        case SceneEntityKind::Vehicle:
            return "Vehicle";

        case SceneEntityKind::Evidence:
            return "Evidence";

        case SceneEntityKind::Measurement:
            return "Measurement";
    }

    return "Unknown";
}

static bool categoryRecommendedFor(
    const AssetLibraryEntry& entry,
    SceneEntityKind kind)
{
    const std::string& category=
        entry.category;

    switch (kind)
    {
        case SceneEntityKind::Vehicle:
            return
                category=="Vehicles";

        case SceneEntityKind::Evidence:
            return
                category=="Evidence" ||
                category=="Forensic Markers" ||
                category=="Props / Misc";

        case SceneEntityKind::Environment:
            return
                category=="Road Infrastructure" ||
                category=="Environment" ||
                category=="Buildings" ||
                category=="Street Furniture" ||
                category=="People" ||
                category=="Props / Misc" ||
                category=="Imported";

        case SceneEntityKind::Measurement:
            return false;
    }

    return false;
}

static std::string formatBytes(
    std::uintmax_t bytes)
{
    const double value=
        static_cast<double>(
            bytes
        );

    char buffer[64]{};

    if (bytes>=
        1024ull*1024ull*1024ull)
    {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "%.2f GB",
            value/
            (1024.0*1024.0*1024.0)
        );
    }
    else if (bytes>=
             1024ull*1024ull)
    {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "%.1f MB",
            value/
            (1024.0*1024.0)
        );
    }
    else if (bytes>=1024ull)
    {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "%.1f KB",
            value/1024.0
        );
    }
    else
    {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "%llu B",
            static_cast<
                unsigned long long
            >(bytes)
        );
    }

    return buffer;
}

static void loadSidecar(
    const std::filesystem::path& sidecar,
    AssetLibraryEntry& entry)
{
    std::ifstream input(sidecar);

    if (!input)
        return;

    std::string line;

    while (std::getline(
        input,
        line))
    {
        line=trim(line);

        if (line.empty() ||
            line[0]=='#')
        {
            continue;
        }

        const std::size_t separator=
            line.find('=');

        if (separator==
            std::string::npos)
        {
            continue;
        }

        const std::string key=
            lower(
                trim(
                    line.substr(
                        0,
                        separator
                    )
                )
            );

        const std::string value=
            trim(
                line.substr(
                    separator+1
                )
            );

        if (key=="asset_id")
            entry.assetId=value;
        else if (key=="name")
            entry.displayName=value;
        else if (key=="category")
            entry.category=value;
        else if (key=="source_url")
            entry.sourceUrl=value;
        else if (key=="author")
            entry.author=value;
        else if (key=="license")
            entry.licenseName=value;
        else if (key=="attribution")
            entry.attribution=value;
        else if (key=="material_profile")
            entry.materialProfile=value;
        else if (key=="meters_per_unit")
            entry.metersPerUnit=
                parseFloat(
                    value,
                    entry.metersPerUnit
                );
        else if (key=="ar_ready")
            entry.arReady=
                parseBool(
                    value,
                    entry.arReady
                );
    }
}

static std::filesystem::path libraryRoot(
    const std::filesystem::path& assetRoot)
{
    return
        assetRoot/
        "library";
}

static bool entryPassesFilters(
    const AssetLibraryEntry& entry,
    const AssetLibraryState& state,
    const SceneEntityRecord* selectedEntity)
{
    if (!entryMatchesSearch(
            entry,
            state.search))
    {
        return false;
    }

    if (!entryMatchesCategory(
            entry,
            state.categoryIndex))
    {
        return false;
    }

    if (!entryMatchesLicense(
            entry,
            state.licenseIndex))
    {
        return false;
    }

    if (!entryMatchesFormat(
            entry,
            state.formatIndex))
    {
        return false;
    }

    if (state.compatibleOnly)
    {
        if (!selectedEntity)
            return false;

        if (!categoryRecommendedFor(
                entry,
                selectedEntity->kind))
        {
            return false;
        }
    }

    return true;
}

static void sortVisibleIndices(
    std::vector<int>& indices,
    const std::vector<AssetLibraryEntry>& entries,
    int sortIndex)
{
    std::sort(
        indices.begin(),
        indices.end(),
        [&](int left,
            int right)
        {
            const auto& a=
                entries[
                    static_cast<
                        std::size_t
                    >(left)
                ];

            const auto& b=
                entries[
                    static_cast<
                        std::size_t
                    >(right)
                ];

            switch (sortIndex)
            {
                case 1:
                    if (a.category!=b.category)
                        return
                            a.category<
                            b.category;

                    return
                        a.displayName<
                        b.displayName;

                case 2:
                    if (a.fileSizeBytes!=
                        b.fileSizeBytes)
                    {
                        return
                            a.fileSizeBytes>
                            b.fileSizeBytes;
                    }

                    return
                        a.displayName<
                        b.displayName;

                case 3:
                    if (a.fileSizeBytes!=
                        b.fileSizeBytes)
                    {
                        return
                            a.fileSizeBytes<
                            b.fileSizeBytes;
                    }

                    return
                        a.displayName<
                        b.displayName;

                default:
                    return
                        a.displayName<
                        b.displayName;
            }
        }
    );
}

static std::size_t categoryCount(
    const std::vector<AssetLibraryEntry>& entries,
    const char* category)
{
    if (!category ||
        std::string(category)=="All")
    {
        return entries.size();
    }

    return
        static_cast<std::size_t>(
            std::count_if(
                entries.begin(),
                entries.end(),
                [&](const AssetLibraryEntry& entry)
                {
                    return
                        entry.category==
                        category;
                }
            )
        );
}

static void drawCategoryCombo(
    AssetLibraryState& state)
{
    state.categoryIndex=
        std::max(
            0,
            std::min(
                static_cast<int>(
                    kCategories.size()
                )-1,
                state.categoryIndex
            )
        );

    const char* preview=
        kCategories[
            static_cast<std::size_t>(
                state.categoryIndex
            )
        ];

    if (ImGui::BeginCombo(
            "##AssetCategory",
            preview))
    {
        for (std::size_t i=0;
             i<kCategories.size();
             ++i)
        {
            const bool selected=
                state.categoryIndex==
                static_cast<int>(i);

            const std::size_t count=
                categoryCount(
                    state.entries,
                    kCategories[i]
                );

            char label[96]{};

            std::snprintf(
                label,
                sizeof(label),
                "%s (%zu)",
                kCategories[i],
                count
            );

            if (ImGui::Selectable(
                    label,
                    selected))
            {
                state.categoryIndex=
                    static_cast<int>(i);
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
}

static void drawPreview(
    AssetLibraryState& state,
    const AssetLibraryEntry& entry,
    const std::filesystem::path& assetRoot)
{
    ImGui::TextDisabled(
        "MODEL PREVIEW"
    );

    const float previewWidth=
        std::max(
            180.0f,
            ImGui::GetContentRegionAvail().x
        );

    const float previewHeight=
        std::max(
            170.0f,
            std::min(
                260.0f,
                previewWidth*0.62f
            )
        );

    ImGui::BeginChild(
        "##AssetModelPreview",
        ImVec2(
            previewWidth,
            previewHeight
        ),
        true,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    if (!state.autoPreview)
    {
        ImGui::TextDisabled(
            "Preview paused."
        );

        ImGui::EndChild();
        return;
    }

    if (state.previewAssetId!=
        entry.assetId)
    {
        gAssetPreviewRenderer.
            clearAssetCache();

        state.previewAssetId=
            entry.assetId;
    }

    RoadSafeCase previewCase;

    SceneEntityRecord previewEntity;
    previewEntity.id=
        "ASSET-PREVIEW";
    previewEntity.legacyId=1;
    previewEntity.kind=
        SceneEntityKind::Environment;
    previewEntity.recordIndex=-1;
    previewEntity.name=
        entry.displayName;
    previewEntity.asset.assetId=
        entry.assetId;
    previewEntity.asset.sourcePath=
        entry.relativePath;
    previewEntity.asset.materialProfile=
        entry.materialProfile;
    previewEntity.asset.metersPerUnit=
        entry.metersPerUnit;
    previewEntity.asset.pbrEnabled=true;
    previewEntity.asset.arReady=
        entry.arReady;
    previewEntity.active=true;
    previewEntity.visible=true;
    previewEntity.locked=false;

    previewCase.sceneEntities.push_back(
        previewEntity
    );

    RenderSettings settings;
    settings.width=
        std::max(
            1,
            static_cast<int>(
                previewWidth-2.0f
            )
        );
    settings.height=
        std::max(
            1,
            static_cast<int>(
                previewHeight-2.0f
            )
        );
    settings.fovDegrees=48.0f;
    settings.viewPreset=0;
    settings.renderMode=0;
    settings.selectedEntityId=0;

    const bool rendered=
        gAssetPreviewRenderer.render(
            previewCase,
            settings,
            assetRoot
        );

    const GLuint texture=
        rendered
            ? gAssetPreviewRenderer.
                colorTexture()
            : 0;

    if (texture!=0)
    {
        ImGui::Image(
            (ImTextureID)(intptr_t)texture,
            ImVec2(
                previewWidth-2.0f,
                previewHeight-2.0f
            ),
            ImVec2(0.0f,1.0f),
            ImVec2(1.0f,0.0f)
        );
    }
    else
    {
        ImGui::TextWrapped(
            "Preview unavailable: %s",
            gAssetPreviewRenderer.
                status().c_str()
        );
    }

    ImGui::EndChild();
}

} // namespace

void refreshAssetLibrary(
    AssetLibraryState& state,
    const std::filesystem::path& assetRoot)
{
    state.entries.clear();
    state.selectedIndex=-1;
    state.previewAssetId.clear();

    gAssetPreviewRenderer.
        clearAssetCache();

    const auto root=
        libraryRoot(
            assetRoot
        );

    std::error_code error;

    if (!std::filesystem::exists(
            root,
            error))
    {
        std::filesystem::
            create_directories(
                root,
                error
            );
    }

    if (error)
    {
        state.status=
            "Could not access assets/library: "+
            error.message();

        state.initialized=true;
        return;
    }

    std::filesystem::
        recursive_directory_iterator iterator(
            root,
            std::filesystem::
                directory_options::
                skip_permission_denied,
            error
        );

    const std::filesystem::
        recursive_directory_iterator end;

    for (;
         !error &&
         iterator!=end;
         iterator.increment(error))
    {
        if (!iterator->
                is_regular_file(error))
        {
            continue;
        }

        const auto filePath=
            iterator->path();

        const std::string extension=
            lower(
                filePath.
                    extension().
                    string()
            );

        if (extension!=".glb" &&
            extension!=".gltf")
        {
            continue;
        }

        AssetLibraryEntry entry;

        std::filesystem::path relative=
            std::filesystem::relative(
                filePath,
                assetRoot,
                error
            );

        if (error)
        {
            error.clear();

            relative=
                filePath.filename();
        }

        entry.relativePath=
            normalizedRelativePath(
                relative
            );

        entry.displayName=
            titleFromStem(
                filePath.
                    stem().
                    string()
            );

        entry.category=
            "";

        entry.assetId=
            "local:"+
            entry.relativePath;

        entry.materialProfile=
            "Metallic-Roughness PBR";

        entry.format=
            extension==".glb"
                ? "GLB"
                : "glTF";

        entry.fileSizeBytes=
            std::filesystem::
                file_size(
                    filePath,
                    error
                );

        if (error)
        {
            entry.fileSizeBytes=0;
            error.clear();
        }

        auto sidecar=filePath;

        sidecar.replace_extension(
            ".roadsafeasset"
        );

        loadSidecar(
            sidecar,
            entry
        );

        // Do not blindly trust bulk-generated sidecar categories.
        // Model/path inference fixes tree/building/road/etc. assets that
        // were previously shown as Vehicles.
        entry.category=
            inferAssetCategory(
                entry
            );

        buildSearchText(
            entry
        );

        state.entries.push_back(
            std::move(entry)
        );
    }

    std::sort(
        state.entries.begin(),
        state.entries.end(),
        [](const AssetLibraryEntry& a,
           const AssetLibraryEntry& b)
        {
            return
                a.displayName<
                b.displayName;
        }
    );

    state.status=
        std::to_string(
            state.entries.size()
        )+
        " installed model asset(s)";

    state.initialized=true;
}

void drawAssetLibrary(
    AssetLibraryState& state,
    RoadSafeCase& caseData,
    int selectedEntityId,
    RoadSafeRenderer& renderer,
    const std::filesystem::path& assetRoot)
{
    if (!state.open)
        return;

    if (!state.initialized)
    {
        refreshAssetLibrary(
            state,
            assetRoot
        );
    }

    if (state.requestFocus)
    {
        ImGui::SetNextWindowFocus();
        state.requestFocus=false;
    }

    ImGui::SetNextWindowSize(
        ImVec2(
            1180.0f,
            720.0f
        ),
        ImGuiCond_FirstUseEver
    );

    if (!ImGui::Begin(
            "Asset Library",
            &state.open,
            ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    SceneEntityRecord* selectedEntity=
        caseData.findSceneEntity(
            selectedEntityId
        );

    ImGui::TextDisabled(
        "ROADSAFE ASSET LIBRARY"
    );

    ImGui::SameLine();

    ImGui::Text(
        "%d installed",
        static_cast<int>(
            state.entries.size()
        )
    );

    ImGui::SameLine();

    ImGui::TextDisabled(
        "| native GLB/glTF preview + provenance-aware assignment"
    );

    ImGui::Separator();

    if (ImGui::Button(
            "REFRESH",
            ImVec2(
                88.0f,
                30.0f
            )))
    {
        refreshAssetLibrary(
            state,
            assetRoot
        );

        renderer.clearAssetCache();
    }

    ImGui::SameLine();

    ImGui::SetNextItemWidth(
        std::max(
            220.0f,
            ImGui::GetContentRegionAvail().x-
                98.0f
        )
    );

    ImGui::InputTextWithHint(
        "##AssetLibrarySearch",
        "Search all fields...  e.g. tree  category:environment  license:cc0  author:kenney  format:glb",
        state.search,
        sizeof(state.search)
    );

    ImGui::Spacing();

    ImGui::SetNextItemWidth(
        190.0f
    );
    drawCategoryCombo(
        state
    );

    ImGui::SameLine();

    ImGui::SetNextItemWidth(
        145.0f
    );
    ImGui::Combo(
        "##AssetLicenseFilter",
        &state.licenseIndex,
        kLicenseFilters.data(),
        static_cast<int>(
            kLicenseFilters.size()
        )
    );

    ImGui::SameLine();

    ImGui::SetNextItemWidth(
        125.0f
    );
    ImGui::Combo(
        "##AssetFormatFilter",
        &state.formatIndex,
        kFormatFilters.data(),
        static_cast<int>(
            kFormatFilters.size()
        )
    );

    ImGui::SameLine();

    ImGui::SetNextItemWidth(
        145.0f
    );
    ImGui::Combo(
        "##AssetSortMode",
        &state.sortIndex,
        kSortModes.data(),
        static_cast<int>(
            kSortModes.size()
        )
    );

    ImGui::SameLine();

    ImGui::BeginDisabled(
        selectedEntity==nullptr
    );

    ImGui::Checkbox(
        "Compatible only",
        &state.compatibleOnly
    );

    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button(
            "CLEAR FILTERS",
            ImVec2(
                116.0f,
                28.0f
            )))
    {
        state.search[0]=0;
        state.categoryIndex=0;
        state.licenseIndex=0;
        state.formatIndex=0;
        state.sortIndex=0;
        state.compatibleOnly=false;
    }

    std::vector<int> visibleIndices;
    visibleIndices.reserve(
        state.entries.size()
    );

    for (std::size_t index=0;
         index<state.entries.size();
         ++index)
    {
        if (entryPassesFilters(
                state.entries[index],
                state,
                selectedEntity))
        {
            visibleIndices.push_back(
                static_cast<int>(
                    index
                )
            );
        }
    }

    sortVisibleIndices(
        visibleIndices,
        state.entries,
        state.sortIndex
    );

    const bool selectedVisible=
        state.selectedIndex>=0 &&
        std::find(
            visibleIndices.begin(),
            visibleIndices.end(),
            state.selectedIndex
        )!=
        visibleIndices.end();

    if (!selectedVisible)
    {
        state.selectedIndex=
            visibleIndices.empty()
                ? -1
                : visibleIndices.front();

        state.previewAssetId.clear();
    }

    ImGui::Spacing();

    ImGui::TextDisabled(
        "%zu shown / %zu installed",
        visibleIndices.size(),
        state.entries.size()
    );

    if (state.compatibleOnly &&
        selectedEntity)
    {
        ImGui::SameLine();

        ImGui::TextDisabled(
            "| compatible with %s (%s)",
            selectedEntity->
                name.c_str(),
            kindName(
                selectedEntity->kind
            )
        );
    }

    ImGui::SameLine();

    ImGui::TextDisabled(
        "| %s",
        state.status.c_str()
    );

    ImGui::Spacing();

    const float totalWidth=
        ImGui::GetContentRegionAvail().x;

    const float detailsWidth=
        std::min(
            430.0f,
            std::max(
                340.0f,
                totalWidth*0.38f
            )
        );

    const float browserWidth=
        std::max(
            360.0f,
            totalWidth-
            detailsWidth-
            8.0f
        );

    ImGui::BeginChild(
        "##AssetLibraryBrowser",
        ImVec2(
            browserWidth,
            0.0f
        ),
        true
    );

    if (visibleIndices.empty())
    {
        ImGui::TextDisabled(
            "No assets match the current filters."
        );

        ImGui::Spacing();

        ImGui::TextWrapped(
            "Try clearing filters or searching fewer terms."
        );
    }
    else if (ImGui::BeginTable(
        "##AssetLibraryTable",
        5,
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp,
        ImVec2(
            0.0f,
            0.0f
        )))
    {
        ImGui::TableSetupScrollFreeze(
            0,
            1
        );

        ImGui::TableSetupColumn(
            "NAME",
            ImGuiTableColumnFlags_WidthStretch,
            2.1f
        );

        ImGui::TableSetupColumn(
            "CATEGORY",
            ImGuiTableColumnFlags_WidthStretch,
            1.35f
        );

        ImGui::TableSetupColumn(
            "LICENCE",
            ImGuiTableColumnFlags_WidthStretch,
            1.1f
        );

        ImGui::TableSetupColumn(
            "FORMAT",
            ImGuiTableColumnFlags_WidthFixed,
            64.0f
        );

        ImGui::TableSetupColumn(
            "SIZE",
            ImGuiTableColumnFlags_WidthFixed,
            82.0f
        );

        ImGui::TableHeadersRow();

        for (int index :
             visibleIndices)
        {
            const auto& entry=
                state.entries[
                    static_cast<
                        std::size_t
                    >(index)
                ];

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(
                0
            );

            const bool selected=
                state.selectedIndex==
                index;

            ImGui::PushID(
                index
            );

            if (ImGui::Selectable(
                    entry.displayName.c_str(),
                    selected,
                    ImGuiSelectableFlags_SpanAllColumns))
            {
                if (state.selectedIndex!=
                    index)
                {
                    state.selectedIndex=
                        index;

                    state.previewAssetId.
                        clear();
                }
            }

            ImGui::PopID();

            ImGui::TableSetColumnIndex(
                1
            );

            ImGui::TextUnformatted(
                entry.category.c_str()
            );

            ImGui::TableSetColumnIndex(
                2
            );

            ImGui::TextUnformatted(
                entry.licenseName.empty()
                    ? "Unspecified"
                    : entry.licenseName.c_str()
            );

            ImGui::TableSetColumnIndex(
                3
            );

            ImGui::TextUnformatted(
                entry.format.c_str()
            );

            ImGui::TableSetColumnIndex(
                4
            );

            const std::string sizeText=
                formatBytes(
                    entry.fileSizeBytes
                );

            ImGui::TextUnformatted(
                sizeText.c_str()
            );
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild(
        "##AssetLibraryDetails",
        ImVec2(
            0.0f,
            0.0f
        ),
        true
    );

    ImGui::TextDisabled(
        "ASSIGNMENT TARGET"
    );

    if (selectedEntity)
    {
        ImGui::Text(
            "%s",
            selectedEntity->
                name.c_str()
        );

        ImGui::SameLine();

        ImGui::TextDisabled(
            "(%s)",
            kindName(
                selectedEntity->kind
            )
        );
    }
    else
    {
        ImGui::TextDisabled(
            "No scene entity selected"
        );
    }

    ImGui::Separator();

    if (state.selectedIndex<0 ||
        state.selectedIndex>=
        static_cast<int>(
            state.entries.size()
        ))
    {
        ImGui::TextDisabled(
            "Select an installed model."
        );

        ImGui::Spacing();

        ImGui::TextWrapped(
            "Search is case-insensitive and supports multiple terms. "
            "Field filters are also available: category:, license:, author:, format:, source:."
        );

        ImGui::EndChild();
        ImGui::End();
        return;
    }

    const auto& entry=
        state.entries[
            static_cast<
                std::size_t
            >(state.selectedIndex)
        ];

    ImGui::Checkbox(
        "Live preview",
        &state.autoPreview
    );

    drawPreview(
        state,
        entry,
        assetRoot
    );

    ImGui::Spacing();

    ImGui::Text(
        "%s",
        entry.displayName.c_str()
    );

    ImGui::TextDisabled(
        "%s",
        entry.relativePath.c_str()
    );

    ImGui::Spacing();

    ImGui::TextDisabled(
        "Category"
    );
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.category.c_str()
    );

    ImGui::TextDisabled(
        "Licence"
    );
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.licenseName.empty()
            ? "Unspecified"
            : entry.licenseName.c_str()
    );

    ImGui::TextDisabled(
        "Author"
    );
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.author.empty()
            ? "Unspecified"
            : entry.author.c_str()
    );

    ImGui::TextDisabled(
        "Format"
    );
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.format.c_str()
    );

    ImGui::TextDisabled(
        "Material"
    );
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.materialProfile.empty()
            ? "PBR"
            : entry.materialProfile.c_str()
    );

    ImGui::TextDisabled(
        "Metric scale"
    );
    ImGui::SameLine();
    ImGui::Text(
        "%.4f m/unit",
        entry.metersPerUnit
    );

    ImGui::TextDisabled(
        "AR"
    );
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.arReady
            ? "Ready"
            : "Not validated"
    );

    if (!entry.sourceUrl.empty())
    {
        ImGui::Spacing();

        ImGui::TextDisabled(
            "Source URL"
        );

        ImGui::TextWrapped(
            "%s",
            entry.sourceUrl.c_str()
        );

        if (ImGui::Button(
                "COPY SOURCE URL",
                ImVec2(
                    148.0f,
                    28.0f
                )))
        {
            ImGui::SetClipboardText(
                entry.sourceUrl.c_str()
            );

            state.status=
                "Source URL copied";
        }
    }

    if (!entry.attribution.empty())
    {
        ImGui::Spacing();

        ImGui::TextDisabled(
            "Attribution"
        );

        ImGui::TextWrapped(
            "%s",
            entry.attribution.c_str()
        );
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canAssign=
        selectedEntity!=nullptr &&
        selectedEntity->kind!=
            SceneEntityKind::Measurement;

    if (!canAssign)
        ImGui::BeginDisabled();

    if (ImGui::Button(
            "ASSIGN TO SELECTED",
            ImVec2(
                180.0f,
                32.0f
            )))
    {
        selectedEntity->
            asset.assetId=
            entry.assetId;

        selectedEntity->
            asset.sourcePath=
            entry.relativePath;

        selectedEntity->
            asset.materialProfile=
            entry.materialProfile;

        selectedEntity->
            asset.sourceUrl=
            entry.sourceUrl;

        selectedEntity->
            asset.author=
            entry.author;

        selectedEntity->
            asset.licenseName=
            entry.licenseName;

        selectedEntity->
            asset.attribution=
            entry.attribution;

        selectedEntity->
            asset.metersPerUnit=
            entry.metersPerUnit;

        selectedEntity->
            asset.arReady=
            entry.arReady;

        selectedEntity->
            asset.pbrEnabled=true;

        caseData.touch();
        renderer.clearAssetCache();

        state.status=
            "Assigned "+
            entry.displayName+
            " to "+
            selectedEntity->name;
    }

    if (!canAssign)
        ImGui::EndDisabled();

    if (selectedEntity)
    {
        const bool recommended=
            categoryRecommendedFor(
                entry,
                selectedEntity->kind
            );

        ImGui::SameLine();

        if (recommended)
        {
            ImGui::TextColored(
                ImVec4(
                    0.52f,
                    0.80f,
                    0.56f,
                    1.0f
                ),
                "TYPE MATCH"
            );
        }
        else
        {
            ImGui::TextColored(
                ImVec4(
                    0.92f,
                    0.70f,
                    0.24f,
                    1.0f
                ),
                "TYPE MISMATCH"
            );
        }

        if (!selectedEntity->
                 asset.sourcePath.empty())
        {
            ImGui::Spacing();
            ImGui::Separator();

            ImGui::TextDisabled(
                "Currently assigned"
            );

            ImGui::TextWrapped(
                "%s",
                selectedEntity->
                    asset.sourcePath.c_str()
            );

            if (ImGui::Button(
                    "CLEAR ASSIGNMENT",
                    ImVec2(
                        154.0f,
                        28.0f
                    )))
            {
                selectedEntity->asset=
                    AssetReference{};

                caseData.touch();
                renderer.clearAssetCache();

                state.status=
                    "Asset assignment cleared";
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

void shutdownAssetLibraryPreviewRenderer()
{
    gAssetPreviewRenderer.shutdown();
}

} // namespace roadsafe