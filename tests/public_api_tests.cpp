#include <engine/public/engine.h>

#include <cstdlib>
#include <iostream>

namespace {

class TestContentPlugin final : public engine::public_api::IContentPackPlugin {
  public:
    engine::public_api::PluginMetadata metadata() const override {
        return {.id = "test-content", .displayName = "Test Content", .targetApiVersion = engine::public_api::publicApiVersion()};
    }

    std::vector<std::string> contentPackPaths() const override { return {"does_not_exist_from_plugin.json"}; }
};

class TestShaderPlugin final : public engine::public_api::IShaderPackPlugin {
  public:
    engine::public_api::PluginMetadata metadata() const override {
        return {.id = "test-shader", .displayName = "Test Shader", .targetApiVersion = engine::public_api::publicApiVersion()};
    }

    std::string shaderPackPath() const override { return "shaders/test"; }
};

class TestPanelPlugin final : public engine::public_api::IToolPanelPlugin {
  public:
    engine::public_api::PluginMetadata metadata() const override {
        return {.id = "test-panel", .displayName = "Test Panel", .targetApiVersion = engine::public_api::publicApiVersion()};
    }

    std::string panelName() const override { return "Test Panel"; }
    void drawPanel() override {}
};

class IncompatiblePlugin final : public engine::public_api::IShaderPackPlugin {
  public:
    engine::public_api::PluginMetadata metadata() const override {
        auto v = engine::public_api::publicApiVersion();
        v.major += 1;
        return {.id = "bad-version", .displayName = "Bad Version", .targetApiVersion = v};
    }

    std::string shaderPackPath() const override { return "shaders/bad"; }
};

} // namespace

int main() {
    engine::public_api::clearRegisteredPlugins();

    TestContentPlugin content;
    TestShaderPlugin shader;
    TestPanelPlugin panel;

    if (!engine::public_api::registerContentPackPlugin(&content).accepted || !engine::public_api::registerShaderPackPlugin(&shader).accepted ||
        !engine::public_api::registerToolPanelPlugin(&panel).accepted) {
        std::cerr << "plugin registry registration failed\n";
        return EXIT_FAILURE;
    }

    if (engine::public_api::contentPackPlugins().empty() || engine::public_api::shaderPackPlugins().empty() || engine::public_api::toolPanelPlugins().empty()) {
        std::cerr << "plugin registry registration failed\n";
        return EXIT_FAILURE;
    }

    if (engine::public_api::registerShaderPackPlugin(&shader).error != engine::public_api::PluginRegistrationError::DuplicateInstance) {
        std::cerr << "expected duplicate instance rejection\n";
        return EXIT_FAILURE;
    }

    IncompatiblePlugin incompatible;
    if (engine::public_api::registerShaderPackPlugin(&incompatible).error != engine::public_api::PluginRegistrationError::IncompatibleApiVersion) {
        std::cerr << "expected version rejection\n";
        return EXIT_FAILURE;
    }

    if (!engine::public_api::unregisterToolPanelPlugin("test-panel") || !engine::public_api::toolPanelPlugins().empty()) {
        std::cerr << "expected successful panel unregister\n";
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
