#pragma once

#include <engine/public/versioning.h>

#include <string>
#include <vector>

namespace engine::public_api {

// Public plugin API policy:
// - Implementations may be loaded/unloaded by hosts.
// - Plugin objects are owned by the host, not by the engine registry.
// - Runtime/internal registry ordering/storage are intentionally not part of ABI/API contract.

struct PluginMetadata {
    std::string id;
    std::string displayName;
    ApiVersion targetApiVersion {};
};

class IPlugin {
  public:
    virtual ~IPlugin() = default;
    virtual PluginMetadata metadata() const = 0;
};

class IShaderPackPlugin : public IPlugin {
  public:
    virtual std::string shaderPackPath() const = 0;
};

class IContentPackPlugin : public IPlugin {
  public:
    virtual std::vector<std::string> contentPackPaths() const = 0;
};

class IToolPanelPlugin : public IPlugin {
  public:
    virtual std::string panelName() const = 0;
    virtual void drawPanel() = 0;
};

enum class PluginRegistrationError {
    None = 0,
    NullPlugin,
    IncompatibleApiVersion,
    DuplicateId,
    DuplicateInstance,
    MissingId
};

struct PluginRegistrationResult {
    PluginRegistrationError error {PluginRegistrationError::None};
    bool accepted {false};
};

// Optional convenience helper for host diagnostics/UI.
const char* pluginRegistrationErrorMessage(PluginRegistrationError error);

PluginRegistrationResult registerShaderPackPlugin(IShaderPackPlugin* plugin);
PluginRegistrationResult registerContentPackPlugin(IContentPackPlugin* plugin);
PluginRegistrationResult registerToolPanelPlugin(IToolPanelPlugin* plugin);

bool unregisterShaderPackPlugin(const std::string& pluginId);
bool unregisterContentPackPlugin(const std::string& pluginId);
bool unregisterToolPanelPlugin(const std::string& pluginId);

void clearRegisteredPlugins();

std::vector<IShaderPackPlugin*> shaderPackPlugins();
std::vector<IContentPackPlugin*> contentPackPlugins();
std::vector<IToolPanelPlugin*> toolPanelPlugins();

// Returns true when plugin target version is accepted by current runtime policy.
bool isPluginTargetCompatible(const PluginMetadata& metadata);

} // namespace engine::public_api
