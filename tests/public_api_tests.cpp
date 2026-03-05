#include <engine/public/engine.h>

#include <cstdlib>
#include <iostream>

namespace {

class TestContentPlugin final : public engine::public_api::IContentPackPlugin {
  public:
    std::string id() const override { return "test-content"; }
    std::vector<std::string> contentPackPaths() const override { return {"does_not_exist_from_plugin.json"}; }
};

class TestShaderPlugin final : public engine::public_api::IShaderPackPlugin {
  public:
    std::string id() const override { return "test-shader"; }
    std::string shaderPackPath() const override { return "shaders/test"; }
};

class TestPanelPlugin final : public engine::public_api::IToolPanelPlugin {
  public:
    std::string panelName() const override { return "Test Panel"; }
    void drawPanel() override {}
};

} // namespace

int main() {
    TestContentPlugin content;
    TestShaderPlugin shader;
    TestPanelPlugin panel;

    engine::public_api::registerContentPackPlugin(&content);
    engine::public_api::registerShaderPackPlugin(&shader);
    engine::public_api::registerToolPanelPlugin(&panel);

    if (engine::public_api::contentPackPlugins().empty() || engine::public_api::shaderPackPlugins().empty() || engine::public_api::toolPanelPlugins().empty()) {
        std::cerr << "plugin registry registration failed\n";
        return EXIT_FAILURE;
    }

    const engine::public_api::ApiVersion v = engine::public_api::publicApiVersion();
    if (v.major < 1) {
        std::cerr << "unexpected public api version\n";
        return EXIT_FAILURE;
    }

    engine::public_api::EngineHost host;
    engine::public_api::EngineLaunchOptions opts;
    opts.headless = true;
    opts.ticks = 1;
    opts.seed = 7;
    opts.contentPackPath = "assets/patterns/sandbox_patterns.json";

    const int rc = host.run(opts);
    if (rc != 0) {
        std::cerr << "EngineHost run failed rc=" << rc << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "public_api_tests passed\n";
    return EXIT_SUCCESS;
}
