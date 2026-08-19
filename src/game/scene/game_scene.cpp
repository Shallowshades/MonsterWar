#include "game_scene.h"
#include "../component/enemy_component.h"
#include "../component/player_component.h"
#include "../component/stats_component.h"
#include "../factory/entity_factory.h"
#include "../factory/blueprint_manager.h"
#include "../loader/entity_builder_mw.h"
#include "../data/ui_config.h"
#include "../data/game_stats.h"
#include "../system/follow_path_system.h"
#include "../system/remove_dead_system.h"
#include "../system/block_system.h"
#include "../system/set_target_system.h"
#include "../system/attack_starter_system.h"
#include "../system/timer_system.h"
#include "../system/orientation_system.h"
#include "../system/animation_state_system.h"
#include "../system/animation_event_system.h"
#include "../system/combat_resolve_system.h"
#include "../system/projectile_system.h"
#include "../system/effect_system.h"
#include "../system/health_bar_system.h"
#include "../system/game_rule_system.h"
#include "../ui/units_portrait_ui.h"
#include "../defs/tags.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/component/velocity_component.h"
#include "../../engine/component/sprite_component.h"
#include "../../engine/component/render_component.h"
#include "../../engine/core/context.h"
#include "../../engine/core/game_state.h"
#include "../../engine/input/input_manager.h"
#include "../../engine/render/camera.h"
#include "../../engine/system/render_system.h"
#include "../../engine/system/movement_system.h"
#include "../../engine/system/animation_system.h"
#include "../../engine/system/ysort_system.h"
#include "../../engine/system/audio_system.h"
#include "../../engine/loader/level_loader.h"
#include "../../engine/ui/ui_manager.h"
#include <entt/core/hashed_string.hpp>
#include <entt/signal/sigh.hpp>
#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::scene {

	GameScene::GameScene(engine::core::Context& context)
		: engine::scene::Scene("GameScene", context) {
		spdlog::info("GameScene 构造完成");
	}

	GameScene::~GameScene() {
	}

	void GameScene::init() {
		if (!initSessionData()) {
			spdlog::error("初始化会话数据失败");
			return;
		}

		if (!initUIConfig()) {
			spdlog::error("初始化UI配置失败");
			return;
		}

		if (!loadLevel()) {
			spdlog::error("加载关卡失败");
			return;
		}

		if (!initEventConnections()) {
			spdlog::error("初始化事件连接失败");
			return;
		}
		if (!initInputConnections()) {
			spdlog::error("初始化输入连接失败");
			return;
		}
		if (!initEntityFactory()) {
			spdlog::error("初始化实体工厂失败");
			return;
		}
		if (!initRegistryContext()) {
			spdlog::error("初始化注册表上下文失败");
			return;
		}
		if (!initUnitsPortraitUI()) {
			spdlog::error("初始化单位肖像UI失败");
			return;
		}
		if (!initSystems()) {
			spdlog::error("初始化系统失败");
			return;
		}
		testSessionData();
		createTestEnemy();

		Scene::init();
	}

	void GameScene::update(float delta_time) {
		auto& dispatcher = mContext.getDispatcher();

		// 每一帧最先清理死亡实体（要在dispatcher处理完事件后再清理，因此放在下一帧开头）
		mRemoveDeadSystem->update(mRegistry);

		// 注意系统更新的顺序
		mTimerSystem->update(mRegistry, delta_time);
		mGameRuleSystem->update(delta_time);					// 规则系统（cost恢复、通关计时）
		mBlockSystem->update(mRegistry, dispatcher);
		mSetTargetSystem->update(mRegistry);
		mFollowPathSystem->update(mRegistry, dispatcher, mWaypointNodes);
		mOrientationSystem->update(mRegistry);					// 调用顺序要在Block、SetTarget、FollowPath之后
		mAttackStarterSystem->update(mRegistry, dispatcher);
		mProjectileSystem->update(delta_time);
		mMovementSystem->update(mRegistry, delta_time);
		mAnimationSystem->update(delta_time);
		mYsortSystem->update(mRegistry);					    // 调用顺序要在MovementSystem之后

		mUnitsPortraitUI->update(delta_time);					// 肖像UI（遮盖更新、左右滚动）

		Scene::update(delta_time);
	}

	void GameScene::render() {
		auto& renderer = mContext.getRenderer();
		auto& camera = mContext.getCamera();

		// 注意渲染顺序，保证正确的遮盖关系
		mRenderSystem->update(mRegistry, renderer, camera);
		mHealthBarSystem->update(mRegistry, renderer, camera);

		Scene::render();
	}

	void GameScene::clean() {
		auto& dispatcher = mContext.getDispatcher();
		auto& input_manager = mContext.getInputManager();
		// 断开所有事件连接
		dispatcher.disconnect(this);
		// 断开输入信号连接
		input_manager.onAction("mouse_right"_hs).disconnect<&GameScene::onCreateTestPlayerMelee>(this);
		input_manager.onAction("mouse_left"_hs).disconnect<&GameScene::onCreateTestPlayerRanged>(this);
		input_manager.onAction("pause"_hs).disconnect<&GameScene::onClearAllPlayers>(this);
		Scene::clean();
	}

	bool GameScene::initSessionData() {
		// 会话数据可能由多个场景共享，为空时才创建并加载默认数据
		if (!mSessionData) {
			mSessionData = std::make_shared<game::data::SessionData>();
			if (!mSessionData->loadDefaultData()) {
				spdlog::error("初始化会话数据失败");
				return false;
			}
		}
		mLevelNumber = mSessionData->getLevelNumber();
		return true;
	}

	bool GameScene::initUIConfig() {
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

	bool GameScene::loadLevel() {
		engine::loader::LevelLoader level_loader;
		// 设置拓展的构建器 EntityBuilderMW
		level_loader.setEntityBuilder(std::make_unique<game::loader::EntityBuilderMW>(level_loader,
			mContext,
			mRegistry,
			mWaypointNodes,
			mStartPoints)
		);
		if (!level_loader.loadLevel("assets/maps/level1.tmj", this)) {
			spdlog::error("加载关卡失败");
			return false;
		}
		return true;
	}

	bool GameScene::initEventConnections() {
		// 本场景直接处理的事件已迁移到各系统（如敌人到达基地 → GameRuleSystem）
		return true;
	}

	bool GameScene::initInputConnections() {
		auto& input_manager = mContext.getInputManager();
		// NOTE: move_left/move_right 已让给肖像UI滚动（UnitsPortraitUI::update），不再用于测试建兵
		input_manager.onAction("mouse_right"_hs).connect<&GameScene::onCreateTestPlayerMelee>(this);
		input_manager.onAction("mouse_left"_hs).connect<&GameScene::onCreateTestPlayerRanged>(this);
		input_manager.onAction("pause"_hs).connect<&GameScene::onClearAllPlayers>(this);
		return true;
	}

	bool GameScene::initEntityFactory() {
		// 如果蓝图管理器为空，则创建一个（将来可能由构造函数传入）
		if (!mBlueprintManager) {
			mBlueprintManager = std::make_shared<game::factory::BlueprintManager>(mContext.getResourceManager());
			if (!mBlueprintManager->loadEnemyClassBlueprints("assets/data/enemy_data.json") ||
				!mBlueprintManager->loadPlayerClassBlueprints("assets/data/player_data.json") ||
				!mBlueprintManager->loadProjectileBlueprints("assets/data/projectile_data.json")) {
				spdlog::error("加载蓝图失败");
				return false;
			}
		}
		mEntityFactory = std::make_unique<game::factory::EntityFactory>(mRegistry, *mBlueprintManager);
		spdlog::info("EntityFactory 加载完成");
		return true;
	}

	bool GameScene::initRegistryContext() {
		// 将关卡统计与共享数据存入 registry.ctx()，供各系统/UI 直接获取
		mRegistry.ctx().emplace<game::data::GameStats>(mGameStats);
		mRegistry.ctx().emplace<std::shared_ptr<game::factory::BlueprintManager>>(mBlueprintManager);
		mRegistry.ctx().emplace<std::shared_ptr<game::data::SessionData>>(mSessionData);
		mRegistry.ctx().emplace<std::shared_ptr<game::data::UIConfig>>(mUIConfig);
		return true;
	}

	bool GameScene::initUnitsPortraitUI() {
		mUnitsPortraitUI = std::make_unique<game::ui::UnitsPortraitUI>(mRegistry, *mUIManager, mContext);
		return true;
	}

	bool GameScene::initSystems() {
		auto& dispatcher = mContext.getDispatcher();
		// 系统初始化需要在可能的依赖模块(如实体工厂)初始化之后
		mRenderSystem = std::make_unique<engine::system::RenderSystem>();
		mMovementSystem = std::make_unique<engine::system::MovementSystem>();
		mAnimationSystem = std::make_unique<engine::system::AnimationSystem>(mRegistry, dispatcher);
		mYsortSystem = std::make_unique<engine::system::YSortSystem>();
		mAudioSystem = std::make_unique<engine::system::AudioSystem>(mRegistry, mContext);

		mFollowPathSystem = std::make_unique<game::system::FollowPathSystem>();
		mRemoveDeadSystem = std::make_unique<game::system::RemoveDeadSystem>();
		mBlockSystem = std::make_unique<game::system::BlockSystem>();
		mSetTargetSystem = std::make_unique<game::system::SetTargetSystem>();
		mAttackStarterSystem = std::make_unique<game::system::AttackStarterSystem>();
		mTimerSystem = std::make_unique<game::system::TimerSystem>();
		mOrientationSystem = std::make_unique<game::system::OrientationSystem>();
		mAnimationStateSystem = std::make_unique<game::system::AnimationStateSystem>(mRegistry, dispatcher);
		mAnimationEventSystem = std::make_unique<game::system::AnimationEventSystem>(mRegistry, dispatcher);
		mCombatResolveSystem = std::make_unique<game::system::CombatResolveSystem>(mRegistry, dispatcher);
		mProjectileSystem = std::make_unique<game::system::ProjectileSystem>(mRegistry, dispatcher, *mEntityFactory);
		mEffectSystem = std::make_unique<game::system::EffectSystem>(mRegistry, dispatcher, *mEntityFactory);
		mHealthBarSystem = std::make_unique<game::system::HealthBarSystem>();
		mGameRuleSystem = std::make_unique<game::system::GameRuleSystem>(mRegistry, dispatcher);
		spdlog::info("系统初始化完成");
		return true;
	}

	// --- 出击选择UI（已迁移至 game::ui::UnitsPortraitUI，由 initUnitsPortraitUI() 创建） ---

	// --- 测试函数 ---
	void GameScene::testSessionData() {
		spdlog::info("关卡号: {}", mLevelNumber);
		spdlog::info("积分: {}", mSessionData->getPoint());
		spdlog::info("是否通关: {}", mSessionData->isLevelClear());
		for (auto& unit : mSessionData->getUnitMap()) {
			spdlog::info("角色名: {}, 职业: {}, 等级: {}, 稀有度: {}",
				unit.second.mName, unit.second.mClass, unit.second.mLevel, unit.second.mRarity);
		}
	}

	void GameScene::createTestEnemy() {
		// 每个起点创建一批敌人
		for (auto start_index : mStartPoints) {
			auto position = mWaypointNodes[start_index].mPosition;

			mEntityFactory->createEnemyUnit("wolf"_hs, position, start_index);
			mEntityFactory->createEnemyUnit("slime"_hs, position, start_index);
			mEntityFactory->createEnemyUnit("goblin"_hs, position, start_index);
			mEntityFactory->createEnemyUnit("dark_witch"_hs, position, start_index);
		}
	}

	bool GameScene::onCreateTestPlayerMelee() {
		auto position = mContext.getInputManager().getLogicalMousePosition();
		auto entity = mEntityFactory->createPlayerUnit("warrior"_hs, position);
		// 让玩家处于受伤状态（治疗师不会锁定满血目标）
		mRegistry.emplace<game::defs::InjuredTag>(entity);
		auto& stats = mRegistry.get<game::component::StatsComponent>(entity);
		stats.mHp = stats.mMaxHp / 2;
		spdlog::info("创建战士: 位置: {}, {}", position.x, position.y);
		return true;
	}

	bool GameScene::onCreateTestPlayerRanged() {
		auto position = mContext.getInputManager().getLogicalMousePosition();
		auto entity = mEntityFactory->createPlayerUnit("archer"_hs, position);
		// 让玩家处于受伤状态（治疗师不会锁定满血目标）
		mRegistry.emplace<game::defs::InjuredTag>(entity);
		auto& stats = mRegistry.get<game::component::StatsComponent>(entity);
		stats.mHp = stats.mMaxHp / 2;
		spdlog::info("创建弓箭手: 位置: {}, {}", position.x, position.y);
		return true;
	}

	bool GameScene::onClearAllPlayers() {
		auto view = mRegistry.view<game::component::PlayerComponent>();
		for (auto entity : view) {
			mRegistry.destroy(entity);
		}
		return true;
	}

} // namespace game::scene
