#include "game_scene.h"
#include "../../engine/core/context.h"
#include "../../engine/input/input_manager.h"
#include "../../engine/utils/events.h"
#include <entt/signal/sigh.hpp>
#include <entt/signal/dispatcher.hpp>
#include <spdlog/spdlog.h>

namespace game::scene {
GameScene::GameScene(engine::core::Context& context, engine::scene::SceneManager& sceneManager) 
	: Scene("GameScene", context, sceneManager) 
{

}

GameScene::~GameScene(){

}

void GameScene::init() {
	// 注册输入回调事件 (J, K 键)
	auto& inputManager = mContext.getInputManager();
	inputManager.onAction("Attack").connect<&GameScene::onAttack>(this);
	inputManager.onAction("Jump", engine::input::ActionState::RELEASED).connect<&GameScene::onJump>(this);
}

void GameScene::clean() {
	// 断开输入回调事件 (J, K 键)
	auto& inputManager = mContext.getInputManager();
	inputManager.onAction("Attack").disconnect<&GameScene::onAttack>(this);
	inputManager.onAction("Jump", engine::input::ActionState::RELEASED).disconnect<&GameScene::onJump>(this);
}

void GameScene::onAttack() {
	spdlog::info("{} : onAttack", mLogTag.data());
	mContext.getDispatcher().enqueue<engine::utils::QuitEvent>();
}

void GameScene::onJump() {
	spdlog::info("{} : onJump", mLogTag.data());
}
} // namespace game::scene
