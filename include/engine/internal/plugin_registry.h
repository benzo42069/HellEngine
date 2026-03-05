#pragma once

#include <engine/public/plugins.h>

namespace engine::internal {

struct PluginRegistry {
    std::vector<public_api::IShaderPackPlugin*> shaderPacks;
    std::vector<public_api::IContentPackPlugin*> contentPacks;
    std::vector<public_api::IToolPanelPlugin*> toolPanels;
};

PluginRegistry& pluginRegistry();

} // namespace engine::internal
