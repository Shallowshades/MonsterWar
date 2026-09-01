#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <SDL3/SDL_main.h>
#include <entt/signal/dispatcher.hpp>

// 只在 Windows 平台上包含 Windows.h
#ifdef _WIN32
#include <Windows.h>
#endif

#include "engine/core/game_app.h"
#include "engine/core/context.h"
#include "engine/utils/events.h"
#include "game/scene/title_scene.h"

// 在程序开始时设置控制台编码（Windows 控制台默认代码页非 UTF-8，中文日志会乱码）
void initializeEnvironment() {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif
}

int main(int, char**) {
	initializeEnvironment();
	// spdlog::set_level(spdlog::level::trace);
	engine::core::GameApp app;
	app.registerSceneSetup([](engine::core::Context& context) {
		// GameApp 在调用 run 方法之前, 先创建并设置初始场景（标题场景）
		auto titleScene = std::make_unique<game::scene::TitleScene>(context);
		context.getDispatcher().trigger<engine::utils::PushSceneEvent>(engine::utils::PushSceneEvent{ std::move(titleScene) });
	});
	app.run();

	return 0;
}
