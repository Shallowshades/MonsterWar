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

	/// @brief 动画播放完成事件
	struct AnimationFinishedEvent {
		entt::entity mEntity{ entt::null };           		///< @brief 目标实体
		entt::id_type mAnimationId{ entt::null };    		///< @brief 动画ID
	};

	/// @brief 动画事件
	struct AnimationEvent {
		entt::entity mEntity{ entt::null };					///< @brief 目标实体
		entt::id_type mEventNameId{ entt::null };			///< @brief 事件名称ID
		entt::id_type mAnimationNameId{ entt::null };		///< @brief 动画名称ID
	};

	/// @brief 播放音效事件
	struct PlaySoundEvent {
		entt::entity mEntity{ entt::null };					///< @brief 目标实体（可以为空，即播放全局音效）
		entt::id_type mSoundId{ entt::null };				///< @brief 音效ID
	};
} // namespace engine::utils
