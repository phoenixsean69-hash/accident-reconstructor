// ROADSAFE_ASSET_LIBRARY_V1

#include "roadsafe_asset_library.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <system_error>

namespace roadsafe
{
namespace
{

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
        if (c=='_' || c=='-')
            c=' ';
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
                        static_cast<unsigned char>(c)
                    )
                );

            uppercaseNext=false;
        }
    }

    return value;
}

static std::string categoryFromFolder(
    const std::filesystem::path& relative)
{
    const auto parent=
        relative.parent_path();

    if (parent.empty())
        return "Imported";

    auto iterator=
        parent.begin();

    if (iterator==parent.end())
        return "Imported";

    return
        titleFromStem(
            iterator->string()
        );
}

static std::string normalizedRelativePath(
    const std::filesystem::path& path)
{
    return
        path.generic_string();
}

static bool entryMatchesSearch(
    const AssetLibraryEntry& entry,
    const char* search)
{
    if (!search || !search[0])
        return true;

    const std::string query=
        lower(search);

    const std::string haystack=
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
            entry.relativePath
        );

    return
        haystack.find(query)!=
        std::string::npos;
}

static const char* categoryFilterName(
    int index)
{
    static const char* names[]={
        "All",
        "Vehicles",
        "Road Infrastructure",
        "Evidence",
        "Environment",
        "People",
        "Street Furniture",
        "Forensic Markers",
        "Imported"
    };

    index=
        std::max(
            0,
            std::min(
                8,
                index
            )
        );

    return names[index];
}

static bool entryMatchesCategory(
    const AssetLibraryEntry& entry,
    int categoryIndex)
{
    if (categoryIndex<=0)
        return true;

    return
        lower(entry.category)==
        lower(
            categoryFilterName(
                categoryIndex
            )
        );
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
    const std::string category=
        lower(entry.category);

    switch (kind)
    {
        case SceneEntityKind::Vehicle:
            return
                category=="vehicles";

        case SceneEntityKind::Evidence:
            return
                category=="evidence" ||
                category=="forensic markers";

        case SceneEntityKind::Environment:
            return
                category=="road infrastructure" ||
                category=="environment" ||
                category=="street furniture" ||
                category=="people" ||
                category=="imported";

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

    while (std::getline(input,line))
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

} // namespace

void refreshAssetLibrary(
    AssetLibraryState& state,
    const std::filesystem::path& assetRoot)
{
    state.entries.clear();
    state.selectedIndex=-1;

    const auto root=
        libraryRoot(
            assetRoot
        );

    std::error_code error;

    if (!std::filesystem::exists(
            root,
            error))
    {
        std::filesystem::create_directories(
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

    std::filesystem::recursive_directory_iterator iterator(
        root,
        std::filesystem::directory_options::
            skip_permission_denied,
        error
    );

    const std::filesystem::recursive_directory_iterator end;

    for (;
         !error && iterator!=end;
         iterator.increment(error))
    {
        if (!iterator->is_regular_file(error))
            continue;

        const auto filePath=
            iterator->path();

        const std::string extension=
            lower(
                filePath.extension().string()
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
                filePath.stem().string()
            );

        entry.category=
            categoryFromFolder(
                std::filesystem::relative(
                    filePath,
                    root,
                    error
                )
            );

        if (error)
        {
            error.clear();
            entry.category="Imported";
        }

        entry.assetId=
            "local:"+
            entry.relativePath;

        entry.materialProfile=
            "Metallic-Roughness PBR";

        entry.fileSizeBytes=
            std::filesystem::file_size(
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
            if (a.category!=b.category)
                return
                    a.category<
                    b.category;

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
            1080.0f,
            650.0f
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
        "| Free/licensed assets keep source + licence metadata"
    );

    ImGui::Separator();

    if (ImGui::Button(
            "REFRESH",
            ImVec2(82.0f,28.0f)))
    {
        refreshAssetLibrary(
            state,
            assetRoot
        );

        renderer.clearAssetCache();
    }

    ImGui::SameLine();

    ImGui::SetNextItemWidth(
        300.0f
    );

    ImGui::InputTextWithHint(
        "##AssetLibrarySearch",
        "Search models, categories, licence...",
        state.search,
        sizeof(state.search)
    );

    ImGui::SameLine();

    ImGui::SetNextItemWidth(
        190.0f
    );

    const char* categories[]={
        "All",
        "Vehicles",
        "Road Infrastructure",
        "Evidence",
        "Environment",
        "People",
        "Street Furniture",
        "Forensic Markers",
        "Imported"
    };

    ImGui::Combo(
        "##AssetCategory",
        &state.categoryIndex,
        categories,
        9
    );

    ImGui::SameLine();

    ImGui::TextDisabled(
        "%s",
        state.status.c_str()
    );

    ImGui::Spacing();

    const float detailsWidth=
        std::min(
            390.0f,
            std::max(
                300.0f,
                ImGui::GetContentRegionAvail().x*
                0.36f
            )
        );

    ImGui::BeginChild(
        "##AssetLibraryBrowser",
        ImVec2(
            -detailsWidth-8.0f,
            0.0f
        ),
        true
    );

    if (ImGui::BeginTable(
        "##AssetLibraryTable",
        5,
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp,
        ImVec2(0.0f,0.0f)))
    {
        ImGui::TableSetupScrollFreeze(
            0,
            1
        );

        ImGui::TableSetupColumn(
            "NAME",
            ImGuiTableColumnFlags_WidthStretch,
            2.2f
        );

        ImGui::TableSetupColumn(
            "CATEGORY",
            ImGuiTableColumnFlags_WidthStretch,
            1.35f
        );

        ImGui::TableSetupColumn(
            "LICENCE",
            ImGuiTableColumnFlags_WidthStretch,
            1.2f
        );

        ImGui::TableSetupColumn(
            "FORMAT",
            ImGuiTableColumnFlags_WidthFixed,
            65.0f
        );

        ImGui::TableSetupColumn(
            "SIZE",
            ImGuiTableColumnFlags_WidthFixed,
            82.0f
        );

        ImGui::TableHeadersRow();

        for (std::size_t index=0;
             index<state.entries.size();
             ++index)
        {
            const auto& entry=
                state.entries[index];

            if (!entryMatchesSearch(
                    entry,
                    state.search) ||
                !entryMatchesCategory(
                    entry,
                    state.categoryIndex))
            {
                continue;
            }

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            const bool selected=
                state.selectedIndex==
                static_cast<int>(index);

            ImGui::PushID(
                static_cast<int>(index)
            );

            if (ImGui::Selectable(
                    entry.displayName.c_str(),
                    selected,
                    ImGuiSelectableFlags_SpanAllColumns))
            {
                state.selectedIndex=
                    static_cast<int>(
                        index
                    );
            }

            ImGui::PopID();

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(
                entry.category.c_str()
            );

            ImGui::TableSetColumnIndex(2);

            ImGui::TextUnformatted(
                entry.licenseName.empty()
                    ? "Unspecified"
                    : entry.licenseName.c_str()
            );

            ImGui::TableSetColumnIndex(3);

            const std::filesystem::path source(
                entry.relativePath
            );

            ImGui::TextUnformatted(
                source.extension().string().
                    c_str()
            );

            ImGui::TableSetColumnIndex(4);

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

    SceneEntityRecord* selectedEntity=
        caseData.findSceneEntity(
            selectedEntityId
        );

    ImGui::TextDisabled(
        "ASSIGNMENT TARGET"
    );

    if (selectedEntity)
    {
        ImGui::Text(
            "%s",
            selectedEntity->name.c_str()
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
            "RoadSafe scans assets/library recursively for .glb and .gltf files. "
            "Optional .roadsafeasset sidecars provide licence, source, scale and attribution metadata."
        );

        ImGui::EndChild();
        ImGui::End();
        return;
    }

    const auto& entry=
        state.entries[
            static_cast<std::size_t>(
                state.selectedIndex
            )
        ];

    ImGui::Text(
        "%s",
        entry.displayName.c_str()
    );

    ImGui::TextDisabled(
        "%s",
        entry.relativePath.c_str()
    );

    ImGui::Spacing();

    ImGui::TextDisabled("Category");
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.category.c_str()
    );

    ImGui::TextDisabled("Licence");
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.licenseName.empty()
            ? "Unspecified"
            : entry.licenseName.c_str()
    );

    ImGui::TextDisabled("Author");
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.author.empty()
            ? "Unspecified"
            : entry.author.c_str()
    );

    ImGui::TextDisabled("Material");
    ImGui::SameLine();
    ImGui::Text(
        "%s",
        entry.materialProfile.empty()
            ? "PBR"
            : entry.materialProfile.c_str()
    );

    ImGui::TextDisabled("Metric scale");
    ImGui::SameLine();
    ImGui::Text(
        "%.4f m/unit",
        entry.metersPerUnit
    );

    ImGui::TextDisabled("AR");
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
                ImVec2(148.0f,28.0f)))
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
            ImVec2(174.0f,32.0f)))
    {
        selectedEntity->asset.assetId=
            entry.assetId;

        selectedEntity->asset.sourcePath=
            entry.relativePath;

        selectedEntity->asset.materialProfile=
            entry.materialProfile;

        selectedEntity->asset.sourceUrl=
            entry.sourceUrl;

        selectedEntity->asset.author=
            entry.author;

        selectedEntity->asset.licenseName=
            entry.licenseName;

        selectedEntity->asset.attribution=
            entry.attribution;

        selectedEntity->asset.metersPerUnit=
            entry.metersPerUnit;

        selectedEntity->asset.arReady=
            entry.arReady;

        selectedEntity->asset.pbrEnabled=true;

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

        ImGui::Spacing();

        if (recommended)
        {
            ImGui::TextColored(
                ImVec4(
                    0.52f,
                    0.80f,
                    0.56f,
                    1.0f
                ),
                "Recommended category match"
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
                "Category differs from selected scene type"
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
                    ImVec2(148.0f,28.0f)))
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

} // namespace roadsafe