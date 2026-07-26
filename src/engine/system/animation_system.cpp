#include "animation_system.h"
#include "../component/animation_component.h"
#include "../component/sprite_component.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>

namespace engine::system {
	AnimationSystem::AnimationSystem(entt::registry& registry, entt::dispatcher& dispatcher)
		: mRegistry(registry), mDispatcher(dispatcher) {
		mDispatcher.sink<engine::utils::PlayAnimationEvent>().connect<&AnimationSystem::onPlayAnimationEvent>(this);
	}

	AnimationSystem::~AnimationSystem() {
		mDispatcher.disconnect(this);
	}

	void AnimationSystem::update(float dt) {
		auto view = mRegistry.view<engine::component::AnimationComponent, engine::component::SpriteComponent>();
		for (auto entity : view) {
			auto& anim_component = view.get<engine::component::AnimationComponent>(entity);
			auto& sprite_component = view.get<engine::component::SpriteComponent>(entity);

			// 如果动画不存在，则跳过
			auto it = anim_component.mAnimations.find(anim_component.mCurrentAnimationId);
			if (it == anim_component.mAnimations.end()) {
				continue;
			}

			// 获取当前动画
			auto& current_animation = it->second;
			// 如果没有帧，则跳过
			if (current_animation.mFrames.empty()) {
				continue;
			}

			// 更新当前播放时间 (推进计时器)
			anim_component.mCurrentTime_ms += dt * 1000.0f * anim_component.mSpeed;

			// 获取当前帧
			const auto& current_frame = current_animation.mFrames[anim_component.mCurrentFrameIndex];

			// 检查是否需要切换到下一帧
			if (anim_component.mCurrentTime_ms >= current_frame.mDuration_ms) {
				anim_component.mCurrentTime_ms -= current_frame.mDuration_ms;
				anim_component.mCurrentFrameIndex++;

				// 处理动画播放完成
				if (anim_component.mCurrentFrameIndex >= current_animation.mFrames.size()) {
					if (current_animation.mLoop) {
						anim_component.mCurrentFrameIndex = 0;
					}
					else {
						// 动画播放完毕且不循环，停在最后一帧
						anim_component.mCurrentFrameIndex = current_animation.mFrames.size() - 1;
						// 发送动画播放完成事件
						mDispatcher.enqueue(engine::utils::AnimationFinishedEvent{ entity, anim_component.mCurrentAnimationId });
					}
				}
			}

			// 更新 SpriteComponent 的源矩形 （根据当前动画帧的源矩形信息）
			const auto& next_frame = current_animation.mFrames[anim_component.mCurrentFrameIndex];
			sprite_component.mSprite.mSourceRect = next_frame.mSourceRect;
		}
	}

	void AnimationSystem::onPlayAnimationEvent(const engine::utils::PlayAnimationEvent& event) {
		// 使用try_get方法来安全获取可能存在的组件。如果不存在则返回nullptr
		if (auto anim = mRegistry.try_get<engine::component::AnimationComponent>(event.mEntity); anim) {
			anim->mCurrentAnimationId = event.mAnimationId;
			anim->mCurrentFrameIndex = 0;
			anim->mCurrentTime_ms = 0.0f;
			anim->mAnimations.at(event.mAnimationId).mLoop = event.mLoop;
		}
	}

} // namespace engine::system
