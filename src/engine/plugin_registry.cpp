#include <engine/internal/plugin_registry.h>
#include <engine/public/api.h>

#include <algorithm>

namespace engine::internal {

PluginRegistry& pluginRegistry() {
    static PluginRegistry r;
    return r;
}

} // namespace engine::internal

namespace engine::public_api {

namespace {

template <typename T>
PluginRegistrationResult registerPlugin(engine::internal::PluginRegistryCollection<T>& dst, T* plugin) {
    if (!plugin) {
        return {PluginRegistrationError::NullPlugin, false};
    }

    const PluginMetadata meta = plugin->metadata();
    if (meta.id.empty()) {
        return {PluginRegistrationError::MissingId, false};
    }

    if (!isApiCompatible(meta.targetApiVersion, publicApiVersion())) {
        return {PluginRegistrationError::IncompatibleApiVersion, false};
    }

    if (std::find(dst.plugins.begin(), dst.plugins.end(), plugin) != dst.plugins.end()) {
        return {PluginRegistrationError::DuplicateInstance, false};
    }

    if (std::find(dst.ids.begin(), dst.ids.end(), meta.id) != dst.ids.end()) {
        return {PluginRegistrationError::DuplicateId, false};
    }

    dst.plugins.push_back(plugin);
    dst.ids.push_back(meta.id);
    return {PluginRegistrationError::None, true};
}

template <typename T>
bool unregisterPlugin(engine::internal::PluginRegistryCollection<T>& dst, const std::string& pluginId) {
    const auto idIt = std::find(dst.ids.begin(), dst.ids.end(), pluginId);
    if (idIt == dst.ids.end()) {
        return false;
    }

    const auto index = static_cast<std::size_t>(std::distance(dst.ids.begin(), idIt));
    dst.ids.erase(idIt);
    dst.plugins.erase(dst.plugins.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

} // namespace

PluginRegistrationResult registerShaderPackPlugin(IShaderPackPlugin* plugin) {
    return registerPlugin(engine::internal::pluginRegistry().shaderPacks, plugin);
}

PluginRegistrationResult registerContentPackPlugin(IContentPackPlugin* plugin) {
    return registerPlugin(engine::internal::pluginRegistry().contentPacks, plugin);
}

PluginRegistrationResult registerToolPanelPlugin(IToolPanelPlugin* plugin) {
    return registerPlugin(engine::internal::pluginRegistry().toolPanels, plugin);
}

bool unregisterShaderPackPlugin(const std::string& pluginId) {
    return unregisterPlugin(engine::internal::pluginRegistry().shaderPacks, pluginId);
}

bool unregisterContentPackPlugin(const std::string& pluginId) {
    return unregisterPlugin(engine::internal::pluginRegistry().contentPacks, pluginId);
}

bool unregisterToolPanelPlugin(const std::string& pluginId) {
    return unregisterPlugin(engine::internal::pluginRegistry().toolPanels, pluginId);
}

void clearRegisteredPlugins() {
    auto& reg = engine::internal::pluginRegistry();
    reg.shaderPacks.plugins.clear();
    reg.shaderPacks.ids.clear();
    reg.contentPacks.plugins.clear();
    reg.contentPacks.ids.clear();
    reg.toolPanels.plugins.clear();
    reg.toolPanels.ids.clear();
}

const std::vector<IShaderPackPlugin*>& shaderPackPlugins() {
    return engine::internal::pluginRegistry().shaderPacks.plugins;
}

const std::vector<IContentPackPlugin*>& contentPackPlugins() {
    return engine::internal::pluginRegistry().contentPacks.plugins;
}

const std::vector<IToolPanelPlugin*>& toolPanelPlugins() {
    return engine::internal::pluginRegistry().toolPanels.plugins;
}

} // namespace engine::public_api
