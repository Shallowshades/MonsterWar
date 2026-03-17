/*****************************************************************//**
 * @file   animation.h
 * @brief  动画类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.11.30
 *********************************************************************/

#pragma once
#ifndef ANIMATION_H
#define ANIMATION_H

#include <SDL3/SDL_rect.h>
#include <vector>
#include <string>
#include <string_view>

namespace engine::render {

/**
 * @brief 代表动画中的单个帧.
 * 
 * 包含纹理图集上的源矩阵和该帧的显示持续时间.
 */
struct AnimationFrame {
	SDL_FRect mSourceRect;														///< @brief 纹理图集上此帧的区域
	float mDuration;															///< @brief 此帧显示的持续时间 (秒)
};

/**
 * @brief 管理一系列动画帧.
 * 
 * 存储动画的帧, 总时长, 名称和循环行为
 */
class Animation final {
public:
	Animation(std::string_view name = "default", bool loop = true);				///< @brief 构造函数 动画名称 是否循环
	~Animation() = default;														///< @brief 默认析构函数

	// 禁用拷贝和移动语义
	Animation(const Animation&) = delete;										///< @brief 删除拷贝构造
	Animation& operator=(const Animation&) = delete;							///< @brief 删除拷贝赋值构造
	Animation(Animation&&) = delete;											///< @brief 删除移动构造
	Animation& operator=(Animation&&) = delete;									///< @brief 删除移动赋值构造

	void addFrame(const SDL_FRect& sourceRect, float duration);					///< @brief 向动画添加一帧
	const AnimationFrame& getFrame(float time) const;							///< @brief 获取给定时间点应该显示的动画帧

	std::string_view getName() const;											///< @brief 获取动画名称
	void setName(std::string_view name);										///< @brief 设置动画名称
	const std::vector<AnimationFrame>& getFrames() const;						///< @brief 获取动画帧列表
	size_t getFrameCount() const;												///< @brief 获取帧数量
	float getTotalDuration() const;												///< @brief 获取动画的总持续时间 (秒)
	bool getIsLooping() const;													///< @brief 检测动画是否循环播放
	void setLooping(bool looping);												///< @brief 设置动画是否循环播放
	bool getIsEmpty() const;													///< @brief 检测动画是否没有帧

private:
	static constexpr std::string_view mLogTag = "Animation";					///< @brief 日志标识
	std::string mName;															///< @brief 动画名称, 例如walk, idle	
	std::vector<AnimationFrame> mFrames;										///< @brief 动画帧列表
	float mTotalDuration = 0.f;													///< @brief 动画的总持续时间 (秒)
	bool mLoop = true;															///< @brief 默认动画是循环的
};
} // namespace engine::render

#endif // !ANIMATION_H
