/*****************************************************************//**
 * @file   animation_component.h
 * @brief  动画组件
 * @version 1.0
 * 
 * @author ShallowShades
 * @date   2025.11.30
 *********************************************************************/

#pragma once
#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include "component.h"

namespace engine::render { class Animation; }
namespace engine::component { class SpriteComponent; }

namespace engine::component {

/**
 * @brief GameObject 的动画组件.
 * 
 * 持有一组Animation对象并控制其播放.
 * 根据当前帧更新关联的SpriteComponent
 */
class AnimationComponent final : public Component {
	friend class engine::object::GameObject;
public:
	AnimationComponent() = default;																///< @brief 构造函数
	~AnimationComponent() override = default;													///< @brief 析构函数

	// 禁用拷贝和移动语义
	AnimationComponent(const AnimationComponent&) = delete;										///< @brief 删除拷贝构造
	AnimationComponent& operator=(const AnimationComponent&) = delete;							///< @brief 删除拷贝赋值构造
	AnimationComponent(AnimationComponent&&) = delete;											///< @brief 删除移动构造
	AnimationComponent& operator=(AnimationComponent&&) = delete;								///< @brief 删除移动赋值构造

	void addAnimation(std::unique_ptr<engine::render::Animation> animation);					///< @brief 添加一个动画
	void playAnimation(std::string_view name);													///< @brief 播放指定名称的动画	
	void stopAnimation();																		///< @brief 停止当前动画播放
	void resumeAnimation();																		///< @brief 恢复当前动画播放

	std::string_view getCurrentAnimationName() const;												///< @brief 获取当前动画名称
	bool getIsPlaying() const;																	///< @brief 获取是否正在播放
	bool getIsAnimationFinished() const;														///< @brief 获取动画是否结束
	bool getIsOneShotRemoval() const;															///< @brief 获取是否在动画结束后删除整个 GameObject
	void setOneShotRemoval(bool isOneShotRemoval);												///< @brief 设置是否在动画结束后删除整个 GameObject

protected:
	// 核心循环方法
	void init() override;																		///< @brief 初始化
	void update(float, engine::core::Context&) override;										///< @brief 更新

private:
	static constexpr std::string_view mLogTag = "AnimationComponent";							///< @brief 日志标识
	std::unordered_map<std::string, std::unique_ptr<engine::render::Animation>> mAnimations;	///< @brief 动画名称到Animation对象的映射
	SpriteComponent* mSpriteComponent = nullptr;												///< @brief 指向必需的SpriteComponent的指针
	engine::render::Animation* mCurrentAnimation = nullptr;										///< @brief 指向当前播放动画的原始指针
																								
	float mAnimationTimer = 0.f;																///< @brief 动画播放中的计时器
	bool mIsPlaying = false;																	///< @brief 当前是否有动画正在播放
	bool mIsOneShotRemoval = false;																///< @brief 是否在动画结束后删除整个 GameObject
};																								
}

#endif // !ANIMATION_COMPONENT_H
