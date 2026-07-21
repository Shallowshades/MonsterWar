#include "game_scene.h"
#include "../component/enemy_component.h"
#include "../factory/entity_factory.h"
#include "../factory/blueprint_manager.h"
#include "../loader/entity_builder_mw.h"
#include "../system/follow_path_system.h"
#include "../system/remove_dead_system.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/component/velocity_component.h"
#include "../../engine/component/sprite_component.h"
#include "../../engine/component/render_component.h"
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

		mFollowPathSystem = std::make_unique<game::system::FollowPathSystem>();
		mRemoveDeadSystem = std::make_unique<game::system::RemoveDeadSystem>();

		spdlog::info("GameScene 构造完成");
	}

	GameScene::~GameScene() {
	}

	void GameScene::init() {
		if (!loadLevel()) {
			spdlog::error("加载关卡失败");
			return;
		}

		if (!initEventConnections()) {
			spdlog::error("初始化事件连接失败");
			return;
		}
		if (!initEntityFactory()) {
			spdlog::error("初始化实体工厂失败");
			return;
		}
		createTestEnemy();

		Scene::init();
	}

	void GameScene::update(float delta_time) {
		auto& dispatcher = mContext.getDispatcher();

		// 每一帧最先清理死亡实体（要在dispatcher处理完事件后再清理，因此放在下一帧开头）
		mRemoveDeadSystem->update(mRegistry);

		// 注意系统更新的顺序
		mFollowPathSystem->update(mRegistry, dispatcher, mWaypointNodes);

		mMovementSystem->update(mRegistry, delta_time);
		mAnimationSystem->update(mRegistry, delta_time);
		mYsortSystem->update(mRegistry);					    // 调用顺序要在MovementSystem之后

		Scene::update(delta_time);
	}

	void GameScene::render() {
		mRenderSystem->update(mRegistry, mContext.getRenderer(), mContext.getCamera());

		Scene::render();
	}

	void GameScene::clean() {
		auto& dispatcher = mContext.getDispatcher();
		// 断开所有事件连接
		dispatcher.disconnect(this);
		Scene::clean();
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

	bool GameScene::initEntityFactory() {
		// 如果蓝图管理器为空，则创建一个（将来可能由构造函数传入）
		if (!mBlueprintManager) {
			mBlueprintManager = std::make_shared<game::factory::BlueprintManager>(mContext.getResourceManager());
			if (!mBlueprintManager->loadEnemyClassBlueprints("assets/data/enemy_data.json")) {
				spdlog::error("加载蓝图失败");
				return false;
			}
		}
		mEntityFactory = std::make_unique<game::factory::EntityFactory>(mRegistry, *mBlueprintManager);
		spdlog::info("EntityFactory 加载完成");
		return true;
	}

	// --- 事件回调函数 ---
	void GameScene::onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent&) {
		spdlog::info("敌人到达基地");
		// TODO: 添加敌人到达基地的逻辑
	}

	// --- 测试函数 ---
	void GameScene::createTestEnemy() {
		// 每个起点创建一批敌人
		for (auto start_index : mStartPoints) {
			auto position = mWaypointNodes[start_index].mPosition;

			auto enemy = mRegistry.create();
			mRegistry.emplace<engine::component::TransformComponent>(enemy, position);
			mRegistry.emplace<engine::component::VelocityComponent>(enemy, glm::vec2(0, 0));
			mRegistry.emplace<game::component::EnemyComponent>(enemy, start_index, 100.0f);

			auto sprite = engine::component::Sprite("assets/textures/Enemy/wolf.png", engine::utils::Rect{ 0, 0, 192, 192 });
			// 设置精灵组件时，需设置偏移量以调整中心点位置（否则会默认以左上角为中心点）
			mRegistry.emplace<engine::component::SpriteComponent>(enemy, std::move(sprite), glm::vec2(192, 192), glm::vec2(-96, -128));
			// 暂定主战斗图层编号为10
			mRegistry.emplace<engine::component::RenderComponent>(enemy, 10);
			mEntityFactory->createEnemyUnit("wolf"_hs, position, start_index);
			mEntityFactory->createEnemyUnit("slime"_hs, position, start_index);
			mEntityFactory->createEnemyUnit("goblin"_hs, position, start_index);
			mEntityFactory->createEnemyUnit("dark_witch"_hs, position, start_index);
		}
	}

} // namespace game::scene
