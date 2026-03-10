#pragma once

#include <engine/public/plugins.h>

#include <mutex>
#include <string>
#include <vector>

namespace engine::internal {

template <typename T>
struct PluginRegistryCollection {
    std::vector<T*> plugins;
    std::vector<std::string> ids;
};

struct PluginRegistry {
    PluginRegistryCollection<public_api::IShaderPackPlugin> shaderPacks;
    PluginRegistryCollection<public_api::IContentPackPlugin> contentPacks;
    PluginRegistryCollection<public_api::IToolPanelPlugin> toolPanels;
};

PluginRegistry& pluginRegistry();
std::mutex& pluginRegistryMutex();

} // namespace engine::internal
