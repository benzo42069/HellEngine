#pragma once

#include <string>
#include <vector>

namespace engine::public_api {

class IShaderPackPlugin {
  public:
    virtual ~IShaderPackPlugin() = default;
    virtual std::string id() const = 0;
    virtual std::string shaderPackPath() const = 0;
};

class IContentPackPlugin {
  public:
    virtual ~IContentPackPlugin() = default;
    virtual std::string id() const = 0;
    virtual std::vector<std::string> contentPackPaths() const = 0;
};

class IToolPanelPlugin {
  public:
    virtual ~IToolPanelPlugin() = default;
    virtual std::string panelName() const = 0;
    virtual void drawPanel() = 0;
};

void registerShaderPackPlugin(IShaderPackPlugin* plugin);
void registerContentPackPlugin(IContentPackPlugin* plugin);
void registerToolPanelPlugin(IToolPanelPlugin* plugin);

const std::vector<IShaderPackPlugin*>& shaderPackPlugins();
const std::vector<IContentPackPlugin*>& contentPackPlugins();
const std::vector<IToolPanelPlugin*>& toolPanelPlugins();

} // namespace engine::public_api
