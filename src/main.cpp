#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "engine/core/game_app.h"
#include "engine/scene/scene_manager.h"
#include "game/scene/game_scene.h"

int main(int, char**) {

	spdlog::set_level(spdlog::level::trace);
	engine::core::GameApp app;
	app.registerSceneSetup([](engine::scene::SceneManager& sceneManager) {
		// GameApp 在调用 run 方法之前, 先创建并设置初始场景
		auto gameScene = std::make_unique<game::scene::GameScene>(sceneManager.getContext(), sceneManager);
		sceneManager.requestPushScene(std::move(gameScene));
		});
	app.run();

	// 执行测试spdlog json 库
	{
		// testSpdlog();
		// testJson();
	}

	return 0;
}
