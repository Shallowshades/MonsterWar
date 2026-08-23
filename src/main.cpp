#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <SDL3/SDL_main.h>
#include <entt/signal/dispatcher.hpp>

#include "engine/core/game_app.h"
#include "engine/core/context.h"
#include "engine/utils/events.h"
#include "game/scene/title_scene.h"

int main(int, char**) {

	spdlog::set_level(spdlog::level::trace);
	engine::core::GameApp app;
	app.registerSceneSetup([](engine::core::Context& context) {
		// GameApp 在调用 run 方法之前, 先创建并设置初始场景（标题场景）
		auto titleScene = std::make_unique<game::scene::TitleScene>(context);
		context.getDispatcher().trigger<engine::utils::PushSceneEvent>(engine::utils::PushSceneEvent{ std::move(titleScene) });
	});
	app.run();

	return 0;
}
