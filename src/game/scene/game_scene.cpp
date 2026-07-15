#include "game_scene.h"
#include "../../engine/core/context.h"
#include "../../engine/system/render_system.h"
#include "../../engine/system/movement_system.h"
#include "../../engine/system/animation_system.h"
#include "../../engine/system/ysort_system.h"
#include "../../engine/loader/level_loader.h"
#include <entt/core/hashed_string.hpp>
#include <entt/signal/sigh.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::scene {

	GameScene::GameScene(engine::core::Context& context)
		: engine::scene::Scene("GameScene", context) {

		// 初始化系统
		mRenderSystem = std::make_unique<engine::system::RenderSystem>();
		mMovementSystem = std::make_unique<engine::system::MovementSystem>();
		mAnimationSystem = std::make_unique<engine::system::AnimationSystem>();
		mYsortSystem = std::make_unique<engine::system::YSortSystem>();

		spdlog::info("GameScene 构造完成");
	}

	GameScene::~GameScene() {
	}

	void GameScene::init() {
		if (!loadLevel()) {
			spdlog::error("加载关卡失败");
			return;
		}

		Scene::init();
	}

	void GameScene::update(float delta_time) {
		mMovementSystem->update(mRegistry, delta_time);
		mAnimationSystem->update(mRegistry, delta_time);
		mYsortSystem->update(mRegistry);					// 调用顺序要在MovementSystem之后

		Scene::update(delta_time);
	}

	void GameScene::render() {
		mRenderSystem->update(mRegistry, mContext.getRenderer(), mContext.getCamera());

		Scene::render();
	}

	void GameScene::clean() {

		Scene::clean();
	}

	bool GameScene::loadLevel() {
		engine::loader::LevelLoader level_loader;
		// 不调用setEntityBuilder，则使用默认的BasicEntityBuilder
		if (!level_loader.loadLevel("assets/maps/level1.tmj", this)) {
			spdlog::error("加载关卡失败");
			return false;
		}
		return true;
	}

} // namespace game::scene
