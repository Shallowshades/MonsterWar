/*****************************************************************//**
  * @file   title_scene.h
  * @brief  标题场景类
  * @version 1.0
  *
  * @author Shallowshades
  * @date   2026.08.23
  *********************************************************************/

#pragma once
#ifndef TITLE_SCENE_H
#define TITLE_SCENE_H

#include "../data/session_data.h"
#include "../data/ui_config.h"
#include "../data/level_config.h"
#include "../system/fwd.h"
#include "../../engine/scene/scene.h"
#include "../../engine/system/fwd.h"
#include <memory>

namespace game::factory {
	class BlueprintManager;
}

namespace game::scene {

/**
 * @brief 标题场景.
 *
 * 提供 开始游戏 / 确认角色 / 载入游戏 / 退出游戏 四个入口，
 * 通过 DebugUISystem(ImGui) 渲染标题UI，并把共享数据传递给后续场景。
 * @note 蓝图为引擎独立资源，可重复加载；会话数据跨场景共享（开始游戏时带入 GameScene）。
 */
class TitleScene final : public engine::scene::Scene {
	// 允许 DebugUISystem 直接访问私有成员变量及方法（ImGui 按钮回调等）
	friend class game::system::DebugUISystem;

	// 数据相关实例（可能与多个场景共享，使用 shared_ptr）
	std::shared_ptr<game::factory::BlueprintManager> mBlueprintManager;  // 蓝图管理器，负责管理蓝图数据
	std::shared_ptr<game::data::SessionData> mSessionData;               // 会话数据，关卡切换时需要传递的数据
	std::shared_ptr<game::data::UIConfig> mUIConfig;                     // UI配置，负责管理UI数据
	std::shared_ptr<game::data::LevelConfig> mLevelConfig;               // 关卡配置，负责管理关卡数据

	// 系统相关实例
	std::unique_ptr<engine::system::RenderSystem> mRenderSystem;         // 渲染系统
	std::unique_ptr<engine::system::YSortSystem> mYsortSystem;           // Y轴排序系统
	std::unique_ptr<engine::system::AnimationSystem> mAnimationSystem;   // 动画系统
	std::unique_ptr<engine::system::MovementSystem> mMovementSystem;     // 移动系统
	std::unique_ptr<game::system::DebugUISystem> mDebugUISystem;         // 调试UI系统（ImGui标题窗口）

	bool mShowUnitInfo{ false };    ///< @brief 是否显示角色列表UI
	bool mShowLoadPanel{ false };   ///< @brief 是否显示载入面板UI

public:
	/**
	 * @brief 构造函数.
	 * @note 蓝图/会话/UI/关卡配置由外部传入（共享数据可在场景切换时复用），
	 *       传 nullptr 时场景内部会懒创建。
	 * @param context 对 Context 实例的引用
	 * @param blueprint_manager 蓝图管理器（可为空，空则内部创建）
	 * @param session_data 会话数据（可为空，空则内部创建）
	 * @param ui_config UI配置（可为空，空则内部创建）
	 * @param level_config 关卡配置（可为空，空则内部创建）
	 */
	TitleScene(engine::core::Context& context,
		std::shared_ptr<game::factory::BlueprintManager> blueprint_manager = nullptr,
		std::shared_ptr<game::data::SessionData> session_data = nullptr,
		std::shared_ptr<game::data::UIConfig> ui_config = nullptr,
		std::shared_ptr<game::data::LevelConfig> level_config = nullptr);
	~TitleScene();

	bool init() override;
	void update(float delta_time) override;
	void render() override;

private:
	// 初始化函数(init函数中调用)
	[[nodiscard]] bool initSessionData();
	[[nodiscard]] bool initLevelConfig();
	[[nodiscard]] bool initBlueprintManager();
	[[nodiscard]] bool initUIConfig();
	[[nodiscard]] bool loadTitleLevel();
	[[nodiscard]] bool initSystems();
	[[nodiscard]] bool initRegistryContext();
	[[nodiscard]] bool initUI();

	// 按钮回调函数 (未来通过游戏UI调用)
	void onStartGameClick();
	void onConfirmRoleClick();
	void onLoadGameClick();
	void onQuitClick();
};

} // game::scene

#endif // TITLE_SCENE_H
