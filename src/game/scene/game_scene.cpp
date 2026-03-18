#include "game_scene.h"
#include "../../engine/core/context.h"
#include "../../engine/input/input_manager.h"
#include <entt/signal/sigh.hpp>
#include <spdlog/spdlog.h>

namespace game::scene {
GameScene::GameScene(engine::core::Context& context) 
	: Scene("GameScene", context) 
{

}

GameScene::~GameScene(){

}

void GameScene::init() {
	// 测试场景编号, 每创建一个场景, 编号+1
	static int32_t count = 0;
	mSceneNum = count++;
	spdlog::info("{} : 场景编号 : {}", mLogTag.data(), mSceneNum);

	// 注册输入回调事件 (J, K 键)
	auto& inputManager = mContext.getInputManager();
	inputManager.onAction("Jump").connect<&GameScene::onReplace>(this);
	inputManager.onAction("MouseLeftClick").connect<&GameScene::onPush>(this);
	inputManager.onAction("MouseRightClick").connect<&GameScene::onPop>(this);
	inputManager.onAction("Pause").connect<&GameScene::onQuit>(this);

	Scene::init();
}

void GameScene::clean() {
	// 断开输入回调事件 (J, K 键)
	auto& inputManager = mContext.getInputManager();
	inputManager.onAction("Jump").disconnect<&GameScene::onReplace>(this);
	inputManager.onAction("MouseLeftClick").disconnect<&GameScene::onPush>(this);
	inputManager.onAction("MouseRightClick").disconnect<&GameScene::onPop>(this);
	inputManager.onAction("Pause").disconnect<&GameScene::onQuit>(this);

	Scene::clean();
}

void GameScene::onReplace() {
	spdlog::info("{} : onReplace, 切换场景", mLogTag.data());
	requestReplaceScene(std::make_unique<game::scene::GameScene>(mContext));
}

void GameScene::onPush() {
	spdlog::info("{} : onPush, 压入场景", mLogTag.data());
	requestPushScene(std::make_unique<game::scene::GameScene>(mContext));

}

void GameScene::onPop() {
	spdlog::info("{} : onPop, 弹出编号 {} 场景", mLogTag.data(), mSceneNum);
	requestPopScene();
}

void GameScene::onQuit() {
	spdlog::info("{} :onQuit", mLogTag.data());
	quit();
}
} // namespace game::scene
