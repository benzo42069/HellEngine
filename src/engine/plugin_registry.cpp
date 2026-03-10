#include <engine/internal/plugin_registry.h>
#include <engine/public/api.h>

#include <algorithm>
#include <mutex>

namespace engine::internal {

PluginRegistry& pluginRegistry() {
    static PluginRegistry r;
    return r;
}

std::mutex& pluginRegistryMutex() {
    static std::mutex m;
    return m;
}

} // namespace engine::internal

namespace engine::public_api {

bool isPluginTargetCompatible(const PluginMetadata& metadata);

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

    if (!isPluginTargetCompatible(meta)) {
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

const char* pluginRegistrationErrorMessage(PluginRegistrationError error) {
    switch (error) {
    case PluginRegistrationError::None:
        return "ok";
    case PluginRegistrationError::NullPlugin:
        return "plugin pointer is null";
    case PluginRegistrationError::IncompatibleApiVersion:
        return "plugin target API version is incompatible with runtime";
    case PluginRegistrationError::DuplicateId:
        return "plugin id is already registered";
    case PluginRegistrationError::DuplicateInstance:
        return "plugin instance is already registered";
    case PluginRegistrationError::MissingId:
        return "plugin id is empty";
    }
    return "unknown registration error";
}

bool isPluginTargetCompatible(const PluginMetadata& metadata) {
    return isApiCompatible(metadata.targetApiVersion, publicApiVersion());
}

PluginRegistrationResult registerShaderPackPlugin(IShaderPackPlugin* plugin) {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    return registerPlugin(engine::internal::pluginRegistry().shaderPacks, plugin);
}

PluginRegistrationResult registerContentPackPlugin(IContentPackPlugin* plugin) {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    return registerPlugin(engine::internal::pluginRegistry().contentPacks, plugin);
}

PluginRegistrationResult registerToolPanelPlugin(IToolPanelPlugin* plugin) {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    return registerPlugin(engine::internal::pluginRegistry().toolPanels, plugin);
}

bool unregisterShaderPackPlugin(const std::string& pluginId) {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    return unregisterPlugin(engine::internal::pluginRegistry().shaderPacks, pluginId);
}

bool unregisterContentPackPlugin(const std::string& pluginId) {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    return unregisterPlugin(engine::internal::pluginRegistry().contentPacks, pluginId);
}

bool unregisterToolPanelPlugin(const std::string& pluginId) {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    return unregisterPlugin(engine::internal::pluginRegistry().toolPanels, pluginId);
}

void clearRegisteredPlugins() {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    auto& reg = engine::internal::pluginRegistry();
    reg.shaderPacks.plugins.clear();
    reg.shaderPacks.ids.clear();
    reg.contentPacks.plugins.clear();
    reg.contentPacks.ids.clear();
    reg.toolPanels.plugins.clear();
    reg.toolPanels.ids.clear();
}

std::vector<IShaderPackPlugin*> shaderPackPlugins() {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    return engine::internal::pluginRegistry().shaderPacks.plugins;
}

std::vector<IContentPackPlugin*> contentPackPlugins() {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    return engine::internal::pluginRegistry().contentPacks.plugins;
}

std::vector<IToolPanelPlugin*> toolPanelPlugins() {
    std::scoped_lock lock(engine::internal::pluginRegistryMutex());
    return engine::internal::pluginRegistry().toolPanels.plugins;
}

} // namespace engine::public_api
