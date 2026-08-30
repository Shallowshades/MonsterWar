/*****************************************************************//**
  * @file   title_scene.cpp
  * @brief  标题场景实现
  * @version 1.0
  *
  * @author Shallowshades
  * @date   2026.08.23
  *********************************************************************/

#include "title_scene.h"
#include "game_scene.h"
#include "../data/ui_config.h"
#include "../data/session_data.h"
#include "../factory/blueprint_manager.h"
#include "../system/debug_ui_system.h"
#include "../../engine/ui/ui_manager.h"
#include "../../engine/core/context.h"
#include "../../engine/core/time.h"
#include "../../engine/core/game_state.h"
#include "../../engine/audio/audio_player.h"
#include "../../engine/utils/events.h"
#include "../../engine/system/render_system.h"
#include "../../engine/system/ysort_system.h"
#include "../../engine/system/animation_system.h"
#include "../../engine/system/movement_system.h"
#include "../../engine/loader/level_loader.h"
#include <spdlog/spdlog.h>
#include <entt/entity/registry.hpp>
#include <utility>

using namespace entt::literals;

namespace game::scene {

	TitleScene::TitleScene(engine::core::Context& context,
		std::shared_ptr<game::factory::BlueprintManager> blueprint_manager,
		std::shared_ptr<game::data::SessionData> session_data,
		std::shared_ptr<game::data::UIConfig> ui_config,
		std::shared_ptr<game::data::LevelConfig> level_config)
		: engine::scene::Scene("TitleScene", context)
		, mBlueprintManager(std::move(blueprint_manager))
		, mSessionData(std::move(session_data))
		, mUIConfig(std::move(ui_config))
		, mLevelConfig(std::move(level_config)) {
		spdlog::info("TitleScene 构造完成");
	}

	TitleScene::~TitleScene() = default;

	bool TitleScene::init() {
		if (!initSessionData()) { spdlog::error("初始化会话数据失败"); return false; }
		if (!initLevelConfig()) { spdlog::error("初始化关卡配置失败"); return false; }
		if (!initBlueprintManager()) { spdlog::error("初始化蓝图管理器失败"); return false; }
		if (!initUIConfig()) { spdlog::error("初始化UI配置失败"); return false; }
		if (!loadTitleLevel()) { spdlog::error("加载标题关卡失败"); return false; }
		if (!initSystems()) { spdlog::error("初始化系统失败"); return false; }
		if (!initRegistryContext()) { spdlog::error("初始化注册表上下文失败"); return false; }
		if (!initUI()) { spdlog::error("初始化UI失败"); return false; }

		// 进入标题状态，并重置游戏速度（从战斗倍速返回标题时也生效）
		mContext.getGameState().setState(engine::core::State::Title);
		mContext.getTime().setTimeScale(1.0f);

		return Scene::init();
	}

	void TitleScene::update(float delta_time) {
		Scene::update(delta_time);
		mAnimationSystem->update(delta_time);
		mMovementSystem->update(mRegistry, delta_time);
		mYsortSystem->update(mRegistry);
	}

	void TitleScene::render() {
		auto& renderer = mContext.getRenderer();
		auto& camera = mContext.getCamera();

		mRenderSystem->update(mRegistry, renderer, camera);

		Scene::render();
		mDebugUISystem->updateTitle(*this);    // 调试UI的显示优先级最高，最后渲染
	}

	bool TitleScene::initSessionData() {
		// 会话数据可能由多个场景共享，为空时才创建并加载默认数据
		if (!mSessionData) {
			mSessionData = std::make_shared<game::data::SessionData>();
			if (!mSessionData->loadDefaultData()) {
				spdlog::error("初始化会话数据失败");
				return false;
			}
		}
		return true;
	}

	bool TitleScene::initLevelConfig() {
		// 关卡配置可能由多个场景共享，为空时才创建并加载
		if (!mLevelConfig) {
			mLevelConfig = std::make_shared<game::data::LevelConfig>();
			if (!mLevelConfig->loadFromFile("assets/data/level_config.json")) {
				spdlog::error("加载关卡配置失败");
				return false;
			}
		}
		return true;
	}

	bool TitleScene::initBlueprintManager() {
		// 蓝图管理器可能由多个场景共享，为空时才创建并加载
		if (!mBlueprintManager) {
			mBlueprintManager = std::make_shared<game::factory::BlueprintManager>(mContext.getResourceManager());
			if (!mBlueprintManager->loadEnemyClassBlueprints("assets/data/enemy_data.json") ||
				!mBlueprintManager->loadPlayerClassBlueprints("assets/data/player_data.json") ||
				!mBlueprintManager->loadProjectileBlueprints("assets/data/projectile_data.json") ||
				!mBlueprintManager->loadEffectBlueprints("assets/data/effect_data.json") ||
				!mBlueprintManager->loadSkillBlueprints("assets/data/skill_data.json")) {
				spdlog::error("加载蓝图失败");
				return false;
			}
		}
		return true;
	}

	bool TitleScene::initUIConfig() {
		// UI配置可能由多个场景共享，为空时才创建并加载
		if (!mUIConfig) {
			mUIConfig = std::make_shared<game::data::UIConfig>();
			if (!mUIConfig->loadFromFile("assets/data/ui_config.json")) {
				spdlog::error("加载UI配置失败");
				return false;
			}
		}
		return true;
	}

	bool TitleScene::loadTitleLevel() {
		engine::loader::LevelLoader level_loader;
		if (!level_loader.loadLevel("assets/maps/title.tmj", this)) {
			spdlog::error("加载标题关卡失败");
			return false;
		}
		return true;
	}

	bool TitleScene::initSystems() {
		auto& dispatcher = mContext.getDispatcher();
		mDebugUISystem = std::make_unique<game::system::DebugUISystem>(mRegistry, mContext);
		mRenderSystem = std::make_unique<engine::system::RenderSystem>();
		mYsortSystem = std::make_unique<engine::system::YSortSystem>();
		mAnimationSystem = std::make_unique<engine::system::AnimationSystem>(mRegistry, dispatcher);
		mMovementSystem = std::make_unique<engine::system::MovementSystem>();
		return true;
	}

	bool TitleScene::initRegistryContext() {
		// 让注册表存储一些数据类型实例作为上下文，方便各系统/UI直接获取
		mRegistry.ctx().emplace<std::shared_ptr<game::data::SessionData>>(mSessionData);
		mRegistry.ctx().emplace<std::shared_ptr<game::factory::BlueprintManager>>(mBlueprintManager);
		mRegistry.ctx().emplace<std::shared_ptr<game::data::UIConfig>>(mUIConfig);
		return true;
	}

	bool TitleScene::initUI() {
		auto window_size = mContext.getGameState().getLogicalSize();
		if (!mUIManager->init(window_size)) return false;

		// 设置标题场景背景音乐
		mContext.getAudioPlayer().playMusic("title_bgm"_hs);

		/* 先用ImGui实现UI，未来再使用游戏内UI */
		return true;
	}

	void TitleScene::onStartGameClick() {
		// 如果数据是读档载入的，有可能已经通关，此时需要进入下一关
		if (mSessionData->isLevelClear()) {
			mSessionData->setLevelClear(false);
			mSessionData->addOneLevel();
		}
		// 把共享的蓝图/会话/UI/关卡配置传给 GameScene，把进度带入战斗
		requestReplaceScene(std::make_unique<game::scene::GameScene>(mContext,
			mBlueprintManager, mSessionData, mUIConfig, mLevelConfig));
	}

	void TitleScene::onConfirmRoleClick() {
		mShowUnitInfo = !mShowUnitInfo;
		/* 用ImGui快速实现逻辑，未来再完善游戏内UI */
	}

	void TitleScene::onLoadGameClick() {
		mShowLoadPanel = !mShowLoadPanel;
		/* 用ImGui快速实现逻辑，未来再完善游戏内UI */
	}

	void TitleScene::onQuitClick() {
		quit();
	}

} // namespace game::scene
