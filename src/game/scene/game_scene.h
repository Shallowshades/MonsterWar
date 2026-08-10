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
#include "../defs/events.h"
#include "../system/fwd.h"
#include "../../engine/scene/scene.h"
#include "../../engine/system/fwd.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace engine::ui {
	class UIElement;
}
namespace game::factory {
	class EntityFactory;
	class BlueprintManager;
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
		[[nodiscard]] bool initSystems();

		void createUnitsPortraitUI();    ///< @brief 创建画面下方的单位肖像UI
		void arrangeUnitsPortraitUI(engine::ui::UIElement* anchor_panel, const glm::vec2& frame_size, float padding);    ///< @brief 排列画面下方的单位肖像UI (肖像增/减时调用)

		// 事件回调函数
		void onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent& event);

		// 输入回调函数

		// 测试函数
		void testSessionData();
		void createTestEnemy();
		[[nodiscard]] bool onCreateTestPlayerMelee();
		[[nodiscard]] bool onCreateTestPlayerRanged();
		[[nodiscard]] bool onCreateTestPlayerHealer();
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

		std::unordered_map<int, game::data::WaypointNode> mWaypointNodes;  // 路径节点ID到节点数据的映射
		std::vector<int> mStartPoints;                                     // 起点ID列表

		std::unique_ptr<game::factory::EntityFactory> mEntityFactory;        // 实体工厂，负责创建和管理实体

		// 管理数据的实例很可能同时被多个场景使用，因此使用共享指针
		std::shared_ptr<game::factory::BlueprintManager> mBlueprintManager;  // 蓝图管理器，负责管理蓝图数据
		std::shared_ptr<game::data::SessionData> mSessionData;               // 会话数据，关卡切换时需要传递的数据
		std::shared_ptr<game::data::UIConfig> mUIConfig;                     // UI配置，负责管理UI数据

		int mLevelNumber{ 1 };    // 当前关卡号（会话数据的缓存副本）
	};

} // game::scene
