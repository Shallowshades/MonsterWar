#include "game_scene.h"
#include "../component/enemy_component.h"
#include "../component/player_component.h"
#include "../component/stats_component.h"
#include "../factory/entity_factory.h"
#include "../factory/blueprint_manager.h"
#include "../loader/entity_builder_mw.h"
#include "../data/ui_config.h"
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
#include "../../engine/ui/ui_panel.h"
#include "../../engine/ui/ui_image.h"
#include "../../engine/ui/ui_button.h"
#include "../../engine/ui/ui_label.h"
#include <entt/core/hashed_string.hpp>
#include <entt/signal/sigh.hpp>
#include <glm/vec2.hpp>
#include <cmath>
#include <string>
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
		if (!initSystems()) {
			spdlog::error("初始化系统失败");
			return;
		}
		testSessionData();
		createTestEnemy();
		createUnitsPortraitUI();

		Scene::init();
	}

	void GameScene::update(float delta_time) {
		auto& dispatcher = mContext.getDispatcher();

		// 每一帧最先清理死亡实体（要在dispatcher处理完事件后再清理，因此放在下一帧开头）
		mRemoveDeadSystem->update(mRegistry);

		// 注意系统更新的顺序
		mTimerSystem->update(mRegistry, delta_time);
		mBlockSystem->update(mRegistry, dispatcher);
		mSetTargetSystem->update(mRegistry);
		mFollowPathSystem->update(mRegistry, dispatcher, mWaypointNodes);
		mOrientationSystem->update(mRegistry);					// 调用顺序要在Block、SetTarget、FollowPath之后
		mAttackStarterSystem->update(mRegistry, dispatcher);
		mProjectileSystem->update(delta_time);
		mMovementSystem->update(mRegistry, delta_time);
		mAnimationSystem->update(delta_time);
		mYsortSystem->update(mRegistry);					    // 调用顺序要在MovementSystem之后

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
		input_manager.onAction("move_left"_hs).disconnect<&GameScene::onCreateTestPlayerHealer>(this);
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
		auto& dispatcher = mContext.getDispatcher();
		// 连接事件
		dispatcher.sink<game::defs::EnemyArriveHomeEvent>().connect<&GameScene::onEnemyArriveHome>(this);
		return true;
	}

	bool GameScene::initInputConnections() {
		auto& input_manager = mContext.getInputManager();
		input_manager.onAction("mouse_right"_hs).connect<&GameScene::onCreateTestPlayerMelee>(this);
		input_manager.onAction("mouse_left"_hs).connect<&GameScene::onCreateTestPlayerRanged>(this);
		input_manager.onAction("pause"_hs).connect<&GameScene::onClearAllPlayers>(this);
		input_manager.onAction("move_left"_hs).connect<&GameScene::onCreateTestPlayerHealer>(this);
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
		spdlog::info("系统初始化完成");
		return true;
	}

	// --- 事件回调函数 ---
	void GameScene::onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent&) {
		spdlog::info("敌人到达基地");
		// TODO: 添加敌人到达基地的逻辑
	}

	// --- 出击选择UI ---
	void GameScene::createUnitsPortraitUI() {
		if (!mUIManager->init(mContext.getGameState().getLogicalSize())) return;

		auto padding = mUIConfig->getUnitPanelPadding();
		auto& unit_map = mSessionData->getUnitMap();
		auto unit_num = unit_map.size();

		// --- 在屏幕下方创建一个panel UI 条，用于显示角色肖像 ---
		// 获取窗口大小和角色肖像框大小
		auto window_size = mContext.getGameState().getLogicalSize();
		auto frame_size = mUIConfig->getUnitPanelFrameSize();
		// 根据角色数量、角色肖像框大小、间隔计算panel的位置和大小
		auto pos = glm::vec2(0.0f, window_size.y - frame_size.y - 2 * padding);
		auto size = glm::vec2(unit_num * frame_size.x + (unit_num + 1) * padding, frame_size.y + 2 * padding);
		auto anchor_panel = std::make_unique<engine::ui::UIPanel>(pos, size);
		// 设置背景色
		anchor_panel->setBackgroundColor(engine::utils::FColor(0.1f, 0.1f, 0.1f, 0.1f));
		// 设置ID，以后即可根据ID找到该panel
		anchor_panel->setId("unit_panel"_hs);

		// 依次添加角色肖像，每个肖像显示由四部分依次叠加：portrait，frame，icon，cost
		// 可以通过一个frame_panel定位（位于上层anchor_panel之中）
		int index = 0;
		for (auto& [name_id, unit_data] : unit_map) {
			auto portrait = mUIConfig->getPortrait(name_id);
			auto frame = mUIConfig->getPortraitFrame(unit_data.mRarity);
			auto icon = mUIConfig->getIcon(unit_data.mClassId);
			auto cost = mBlueprintManager->getPlayerClassBlueprint(unit_data.mClassId).mPlayer.mCost;
			cost = static_cast<int>(std::round(engine::utils::statModify(static_cast<float>(cost), 1, unit_data.mRarity))); // 只有稀有度对cost有影响

			// 创建每个肖像的 frame_panel
			auto frame_pos = glm::vec2(padding + index * (frame_size.x + padding), padding);
			auto frame_panel = std::make_unique<engine::ui::UIPanel>(frame_pos, frame_size);
			frame_panel->setId(name_id);

			// 依次添加四个元素，为了能够交互，将frame设置为按钮，并绑定点击事件
			frame_panel->addChild(std::make_unique<engine::ui::UIImage>(portrait, glm::vec2(0.0f, 0.0f), frame_size));
			frame_panel->addChild(std::make_unique<engine::ui::UIButton>(mContext,
				frame,
				frame,
				frame,
				glm::vec2(0.0f, 0.0f),
				frame_size
				// TODO: 添加点击事件回调函数
			));
			frame_panel->addChild(std::make_unique<engine::ui::UIImage>(icon, glm::vec2(0.0f, 0.0f), frame_size / 2.0f));
			frame_panel->addChild(std::make_unique<engine::ui::UILabel>(mContext.getTextRenderer(),
				std::to_string(cost),
				mUIConfig->getUnitPanelFontPath(),
				mUIConfig->getUnitPanelFontSize(),
				engine::utils::FColor::yellow(),
				mUIConfig->getUnitPanelFontOffset()
			));
			// 最后添加一个灰色的遮盖panel，cost不足以支持该角色出击时显示
			auto cover_panel = std::make_unique<engine::ui::UIPanel>(glm::vec2(0.0f, 0.0f), frame_size);
			cover_panel->setBackgroundColor(engine::utils::FColor(0.0f, 0.0f, 0.0f, 0.2f));
			cover_panel->setId("cover_panel"_hs);
			frame_panel->addChild(std::move(cover_panel));

			// 将frame_panel添加到anchor_panel中，并使用cost作为排序键
			anchor_panel->addChild(std::move(frame_panel), cost);
			index++;
		}

		// 对anchor_panel中的子元素(frame_panel)进行排序
		anchor_panel->sortChildrenByOrderIndex();
		// 按顺序排列anchor_panel中的子元素(frame_panel)的位置
		arrangeUnitsPortraitUI(anchor_panel.get(), frame_size, padding);

		mUIManager->addElement(std::move(anchor_panel));
	}

	void GameScene::arrangeUnitsPortraitUI(engine::ui::UIElement* anchor_panel, const glm::vec2& frame_size, float padding) {
		// 遍历panel中的子元素(定位panel)，并依次设定位置
		for (size_t i = 0; i < anchor_panel->getChildren().size(); i++) {
			auto& child = anchor_panel->getChildren()[i];
			child->setPosition(glm::vec2(padding + i * (frame_size.x + padding), padding));
		}
		// 更新panel的size
		anchor_panel->setSize(glm::vec2(padding + anchor_panel->getChildren().size() * (frame_size.x + padding),
			frame_size.y + 2 * padding));
	}

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

	bool GameScene::onCreateTestPlayerHealer() {
		auto position = mContext.getInputManager().getLogicalMousePosition();
		mEntityFactory->createPlayerUnit("witch"_hs, position);
		spdlog::info("创建治疗者: 位置: {}, {}", position.x, position.y);
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
