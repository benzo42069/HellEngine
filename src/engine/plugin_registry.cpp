#include <engine/internal/plugin_registry.h>

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
void pushUnique(std::vector<T*>& dst, T* plugin) {
    if (!plugin) return;
    if (std::find(dst.begin(), dst.end(), plugin) == dst.end()) {
        dst.push_back(plugin);
    }
}
} // namespace

void registerShaderPackPlugin(IShaderPackPlugin* plugin) {
    pushUnique(engine::internal::pluginRegistry().shaderPacks, plugin);
}

void registerContentPackPlugin(IContentPackPlugin* plugin) {
    pushUnique(engine::internal::pluginRegistry().contentPacks, plugin);
}

void registerToolPanelPlugin(IToolPanelPlugin* plugin) {
    pushUnique(engine::internal::pluginRegistry().toolPanels, plugin);
}

const std::vector<IShaderPackPlugin*>& shaderPackPlugins() {
    return engine::internal::pluginRegistry().shaderPacks;
}

const std::vector<IContentPackPlugin*>& contentPackPlugins() {
    return engine::internal::pluginRegistry().contentPacks;
}

const std::vector<IToolPanelPlugin*>& toolPanelPlugins() {
    return engine::internal::pluginRegistry().toolPanels;
}

} // namespace engine::public_api
