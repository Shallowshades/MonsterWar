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
#include "../data/session_data.h"
#include "../data/ui_config.h"
#include "../data/game_stats.h"
#include "../data/level_config.h"
#include "../defs/events.h"
#include "../system/fwd.h"
#include "../../engine/scene/scene.h"
#include "../../engine/system/fwd.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <entt/entity/entity.hpp>

namespace game::ui {
	class UnitsPortraitUI;
}
namespace game::factory {
	class EntityFactory;
	class BlueprintManager;
}
namespace game::spawner {
	class EnemySpawner;
}
namespace game::system {
	class BlockSystem;
	class ProjectileSystem;
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
		[[nodiscard]] bool initSessionData();
		[[nodiscard]] bool initUIConfig();
		[[nodiscard]] bool loadLevel();
		[[nodiscard]] bool initEventConnections();
		[[nodiscard]] bool initInputConnections();
		[[nodiscard]] bool initEntityFactory();
		[[nodiscard]] bool initRegistryContext();
		[[nodiscard]] bool initUnitsPortraitUI();
		[[nodiscard]] bool initSystems();
		[[nodiscard]] bool initLevelConfig();
		[[nodiscard]] bool initEnemySpawner();

		// 输入回调函数

		// 测试函数
		void testSessionData();
		[[nodiscard]] bool onClearAllPlayers();

	private:
		// 引擎系统
		std::unique_ptr<engine::system::RenderSystem> mRenderSystem;
		std::unique_ptr<engine::system::MovementSystem> mMovementSystem;
		std::unique_ptr<engine::system::AnimationSystem> mAnimationSystem;
		std::unique_ptr<engine::system::YSortSystem> mYsortSystem;
		std::unique_ptr<engine::system::AudioSystem> mAudioSystem;
		// 游戏系统
		std::unique_ptr<game::system::FollowPathSystem> mFollowPathSystem;
		std::unique_ptr<game::system::RemoveDeadSystem> mRemoveDeadSystem;
		std::unique_ptr<game::system::BlockSystem> mBlockSystem;
		std::unique_ptr<game::system::SetTargetSystem> mSetTargetSystem;
		std::unique_ptr<game::system::AttackStarterSystem> mAttackStarterSystem;
		std::unique_ptr<game::system::TimerSystem> mTimerSystem;
		std::unique_ptr<game::system::OrientationSystem> mOrientationSystem;
		std::unique_ptr<game::system::AnimationStateSystem> mAnimationStateSystem;
		std::unique_ptr<game::system::AnimationEventSystem> mAnimationEventSystem;
		std::unique_ptr<game::system::CombatResolveSystem> mCombatResolveSystem;
		std::unique_ptr<game::system::ProjectileSystem> mProjectileSystem;
		std::unique_ptr<game::system::EffectSystem> mEffectSystem;
		std::unique_ptr<game::system::HealthBarSystem> mHealthBarSystem;
		std::unique_ptr<game::system::GameRuleSystem> mGameRuleSystem;       // 游戏规则系统（cost/关卡状态）
		std::unique_ptr<game::system::PlaceUnitSystem> mPlaceUnitSystem;     // 放置单位系统（准备/放置出击单位）
		std::unique_ptr<game::system::RenderRangeSystem> mRenderRangeSystem; // 渲染范围系统（远程攻击范围圆）
		std::unique_ptr<game::system::DebugUISystem> mDebugUISystem;         // 调试UI系统（ImGui调试窗口）
		std::unique_ptr<game::system::SelectionSystem> mSelectionSystem;     // 选择单位系统（鼠标悬浮/选中单位）

		std::unique_ptr<game::ui::UnitsPortraitUI> mUnitsPortraitUI;         // 封装的单位肖像UI，负责管理单位肖像UI的创建、更新和排列

		std::unordered_map<int, game::data::WaypointNode> mWaypointNodes;  // 路径节点ID到节点数据的映射
		std::vector<int> mStartPoints;                                     // 起点ID列表
		game::data::GameStats mGameStats;                                  // 关卡内游戏统计数据

		std::unique_ptr<game::factory::EntityFactory> mEntityFactory;        // 实体工厂，负责创建和管理实体

		// 管理数据的实例很可能同时被多个场景使用，因此使用共享指针
		std::shared_ptr<game::factory::BlueprintManager> mBlueprintManager;  // 蓝图管理器，负责管理蓝图数据
		std::shared_ptr<game::data::SessionData> mSessionData;               // 会话数据，关卡切换时需要传递的数据
		std::shared_ptr<game::data::UIConfig> mUIConfig;                     // UI配置，负责管理UI数据
		std::shared_ptr<game::data::LevelConfig> mLevelConfig;               // 关卡配置，负责管理关卡数据

		game::data::Waves mWaves;                                            // 当前关卡的波次数据（从关卡配置复制）
		std::unique_ptr<game::spawner::EnemySpawner> mEnemySpawner;          // 敌人生成器，按波次生成敌人

		int mLevelNumber{ 1 };    // 当前关卡号（会话数据的缓存副本）
		entt::entity mSelectedUnit{ entt::null };    // 游戏中鼠标选中的单位
		entt::entity mHoveredUnit{ entt::null };     // 游戏中鼠标悬浮的单位
	};

} // game::scene
