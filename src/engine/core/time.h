#pragma once

#ifndef TIME_H
#define TIME_H

#include <SDL3/SDL_stdinc.h>

namespace engine::core {

/**
 * @brief管理游戏中的时间, 计算帧间时间差.
 * 
 * 使用SDL的高精度性能计数器来确保时间测量的准确性.
 * 提供获取缩放和未缩放delta的方法,以及设置时间缩放因子的能力.
 */
class Time final {
public:
	Time();

	// 删除拷贝移动构造
	Time(const Time&) = delete;
	Time& operator=(const Time&) = delete;
	Time(Time&&) = delete;
	Time& operator=(Time&&) = delete;

	/**
	 * @brief 每帧开始时调用, 更新内部时间状态并计算DeltaTime.
	 */
	void update();

	/**
	 * @brief 获取经过时间缩放调整后的帧间时间差DeltaTime.
	 * 
	 * @return double 缩放后的DeltaTime(秒)
	 */
	float getDeltaTime() const;

	/**
	 * @brief 获取未经过时间缩放的帧间时间差DeltaTime.
	 *
	 * @return double 未缩放的DeltaTime(秒)
	 */
	float getUnscaledDeltaTime() const;

	/**
	 * @brief 设置时间缩放因子.
	 * 
	 * @param scale 时间缩放值. 1.0正常速度, <1.0为慢动作, >1.0为快进, 不允许为负值
	 */
	void setTimeScale(float scale);

	/**
	 * @brief 获取当前时间缩放因子.
	 * 
	 * @param float 当前时间缩放因子
	 */
	float getTimeScale() const;

	/**
	 * @brief 设置目标帧率.
	 * 
	 * @param fps 目标每秒帧数. 设置0表示不限制帧率;负值被视为0
	 */
	void setTargetFps(int fps);

	/**
	 * @brief 获取当前设置的目标帧率.
	 * 
	 * @return int 目标FPS, 0表示不限制
	 */
	int getTargetFps() const;

private:
	/**
	 * @brief update中调用, 用于限制帧率.如果设置了mTargetFps>0, 且当前帧执行时间小于目标帧时间,则会调用SDL_DelayNS()来等待剩余时间
	 * 
	 * @param currentDeltaTime 当前帧的执行时间(秒)
	 */
	void limitFrameRate(float currentDeltaTime);

private:
	Uint64 mLastTime = 0;			///< @brief 上一帧的时间戳
	Uint64 mFrameStartTime = 0;		///< @brief 当前帧开始的时间戳
	double mDeltaTime = 0.0;		///< @brief 未缩放的帧间时间差
	double mTimeScale = 1.0;		///< @brief 时间缩放因子

	// 帧率限制
	int mTargetFps = 0;				///< @brief 目标FPS, 0表示不限制
	double mTargetFrameTime = 0.0;	///< @brief 目标每帧时间(秒)
};
}

#endif // !TIME_H

