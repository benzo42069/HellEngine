#pragma once

#include <engine/public/versioning.h>

#include <string>
#include <vector>

namespace engine::public_api {

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

PluginRegistrationResult registerShaderPackPlugin(IShaderPackPlugin* plugin);
PluginRegistrationResult registerContentPackPlugin(IContentPackPlugin* plugin);
PluginRegistrationResult registerToolPanelPlugin(IToolPanelPlugin* plugin);

bool unregisterShaderPackPlugin(const std::string& pluginId);
bool unregisterContentPackPlugin(const std::string& pluginId);
bool unregisterToolPanelPlugin(const std::string& pluginId);

void clearRegisteredPlugins();

const std::vector<IShaderPackPlugin*>& shaderPackPlugins();
const std::vector<IContentPackPlugin*>& contentPackPlugins();
const std::vector<IToolPanelPlugin*>& toolPanelPlugins();

} // namespace engine::public_api
