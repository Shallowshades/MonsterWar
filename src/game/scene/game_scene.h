/*****************************************************************//**
  * @file   game_scene.h
  * @brief  游戏场景类
  * @version 1.0
  *
  * @author Shallowshades
  * @date   2026.07.14
  *********************************************************************/

#pragma once

#include "../data/waypoint_node.h"
#include "../defs/events.h"
#include "../system/fwd.h"
#include "../../engine/scene/scene.h"
#include "../../engine/system/fwd.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace game::factory {
	class EntityFactory;
	class BlueprintManager;
}

namespace game::scene {

	class GameScene final : public engine::scene::Scene {
	public:
		GameScene(engine::core::Context& context);
		~GameScene();

		void init() override;
		void update(float delta_time) override;
		void render() override;
		void clean() override;

	private:
		[[nodiscard]] bool loadLevel();
		[[nodiscard]] bool initEventConnections();
		[[nodiscard]] bool initEntityFactory();

		// 事件回调函数
		void onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent& event);

		// 测试函数
		void createTestEnemy();

	private:
		std::unique_ptr<engine::system::RenderSystem> mRenderSystem;
		std::unique_ptr<engine::system::MovementSystem> mMovementSystem;
		std::unique_ptr<engine::system::AnimationSystem> mAnimationSystem;
		std::unique_ptr<engine::system::YSortSystem> mYsortSystem;
		std::unique_ptr<game::system::FollowPathSystem> mFollowPathSystem;
		std::unique_ptr<game::system::RemoveDeadSystem> mRemoveDeadSystem;

		std::unordered_map<int, game::data::WaypointNode> mWaypointNodes;  // 路径节点ID到节点数据的映射
		std::vector<int> mStartPoints;                                     // 起点ID列表

		std::unique_ptr<game::factory::EntityFactory> mEntityFactory;        // 实体工厂，负责创建和管理实体

		// 管理数据的实例很可能同时被多个场景使用，因此使用共享指针
		std::shared_ptr<game::factory::BlueprintManager> mBlueprintManager;  // 蓝图管理器，负责管理蓝图数据
	};

} // game::scene
