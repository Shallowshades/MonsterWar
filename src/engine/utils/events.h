/*****************************************************************//**
 * @file   events.h
 * @brief  事件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.03.17
 *********************************************************************/

#pragma once

#include <memory>
#include <entt/entity/entity.hpp>

namespace engine::scene {
	class Scene;
}

namespace engine::utils {

	struct QuitEvent {};									///< @brief 退出事件
	struct PopSceneEvent {};								///< @brief 弹出场景事件
	struct PushSceneEvent {									///< @brief 压入场景事件
		std::unique_ptr<engine::scene::Scene> scene;
	};
	struct ReplaceSceneEvent {								///< @brief 替换场景事件
		std::unique_ptr<engine::scene::Scene> scene;
	};

	/// @brief 播放动画事件
	struct PlayAnimationEvent {
		entt::entity mEntity{ entt::null };					///< @brief 目标实体
		entt::id_type mAnimationId{ entt::null };			///< @brief 动画ID
		bool mLoop{ true };									///< @brief 是否循环
	};
} // namespace engine::utils
