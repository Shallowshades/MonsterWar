/*****************************************************************//**
 * @file   animation_component.h
 * @brief  动画组件
 * @version 1.0
 *
 * @author ShallowShades
 * @date   2026.07.14
 *********************************************************************/

#pragma once
#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H

#include "../../engine/utils/math.h"
#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>
#include <unordered_map>
#include <vector>

namespace engine::component {
	/**
	 * @brief 动画帧数据结构
	 *
	 * 包含帧源矩形和帧间隔（毫秒）。

	 */
	struct AnimationFrame {
		engine::utils::Rect mSourceRect{};                              ///< @brief 帧源矩形
		float mDuration_ms{ 100.0f };                                   ///< @brief 帧间隔（毫秒）
		AnimationFrame(engine::utils::Rect sourceRect, float duration_ms = 100.0f)
			: mSourceRect(std::move(sourceRect)), mDuration_ms(duration_ms) {
		}
	};

	/**
	 * @brief 动画数据结构
	 *
	 * 包含动画名称、帧列表、总时长、当前播放时间、是否循环等属性。
	 */
	struct Animation {
		std::vector<AnimationFrame> mFrames;                            ///< @brief 动画帧
		std::unordered_map<int, entt::id_type> mEvents;					///< @brief 动画事件，键为帧索引，值为事件ID
		float mTotalDuration_ms{};                                      ///< @brief 动画总时长（毫秒）
		bool mLoop{ true };                                             ///< @brief 是否循环

		/**
		 * @brief 构造函数
		 * @param name 动画名称
		 * @param frames 动画帧
		 * @param events 动画事件，默认为空
		 * @param loop 是否循环，默认true
		 */
		Animation(std::vector<AnimationFrame> frames, std::unordered_map<int, entt::id_type> events = {}, bool loop = true) :
			mFrames(std::move(frames)), mLoop(loop), mEvents(std::move(events)) {
			// 计算动画总时长 (总时长 = 所有帧时长之和)
			mTotalDuration_ms = 0.0f;
			for (const auto& frame : mFrames) {
				mTotalDuration_ms += frame.mDuration_ms;
			}
		}
	};

	/**
	 * @brief 动画组件
	 *
	 * 包含动画名称、帧列表、总时长、当前播放时间、是否循环等属性。
	 */
	struct AnimationComponent {
		std::unordered_map<entt::id_type, Animation> mAnimations;		///< @brief 动画集合
		entt::id_type mCurrentAnimationId{ entt::null };				///< @brief 当前播放的动画名称
		size_t mCurrentFrameIndex{};									///< @brief 当前播放的帧索引
		float mCurrentTime_ms{};										///< @brief 当前播放时间（毫秒）
		float mSpeed{ 1.0f };											///< @brief 播放速度

		/**
		 * @brief 构造函数
		 * @param animations 动画集合
		 * @param current_animation_name 当前播放的动画名称
		 * @param current_frame_index 当前播放的帧索引
		 * @param current_time_ms 当前播放时间（毫秒）
		 * @param speed 播放速度
		 */
		AnimationComponent(std::unordered_map<entt::id_type, Animation> animations,
			entt::id_type current_animation_id,
			size_t current_frame_index = 0,
			float current_time_ms = 0.0f,
			float speed = 1.0f) :
			mAnimations(std::move(animations)),
			mCurrentAnimationId(current_animation_id),
			mCurrentFrameIndex(current_frame_index),
			mCurrentTime_ms(current_time_ms),
			mSpeed(speed) {
		}
	};
}
#endif // ANIMATION_COMPONENT_H
