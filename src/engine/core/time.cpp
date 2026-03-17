#include <SDL3/SDL_timer.h>
#include <spdlog/spdlog.h>
#include <algorithm>

#include "time.h"

engine::core::Time::Time() {
	// 初始化mLastTime和mFrameStartTime为当前时间,避免第一帧DeltaTime过大
	mLastTime = SDL_GetTicksNS();
	mFrameStartTime = mLastTime;
	spdlog::trace("Time初始化. Last Time: {}", mLastTime);
}

void engine::core::Time::update() {
	mFrameStartTime = SDL_GetTicksNS();		// 记录进入update的时间戳
	double currentDeltaTime = static_cast<double>(mFrameStartTime - mLastTime) / SDL_NS_PER_SECOND;
	if (mTargetFrameTime > 0.0) {			// 若设定帧率,则限制;否则不限制
		limitFrameRate(static_cast<float>(currentDeltaTime));
	}
	else {
		mDeltaTime = currentDeltaTime;
	}
	mLastTime = SDL_GetTicksNS();			// 记录离开update的时间戳
}

float engine::core::Time::getDeltaTime() const {
	return static_cast<float>(mDeltaTime * mTimeScale);
}

float engine::core::Time::getUnscaledDeltaTime() const {
	return static_cast<float>(mDeltaTime);
}

void engine::core::Time::setTimeScale(float scale) {
	if (scale < 0.0) {
		spdlog::warn("Time Scale不能为负. Clamping to 0.");
		scale = 0.0;	// 防止负时间缩放
	}
	mTimeScale = scale;
}

float engine::core::Time::getTimeScale() const {
	return static_cast<float>(mTimeScale);
}

void engine::core::Time::setTargetFps(int fps) {
	if (fps < 0) {
		spdlog::warn("Target FPS不能为负. Setting to 0.(Unlimited)");
		mTargetFps = 0;
	}
	else {
		mTargetFps = fps;
	}

	if (mTargetFps > 0) {
		mTargetFrameTime = 1.0 / static_cast<double>(mTargetFps);
		spdlog::info("Target FPS设置为: {} (Frame time: {:.6f}s)", mTargetFps, mTargetFrameTime);
	}
	else {
		mTargetFrameTime = 0.0;
		spdlog::info("Target FPS设置为: Unlimited");
	}
}

int engine::core::Time::getTargetFps() const {
	return mTargetFps;
}

void engine::core::Time::limitFrameRate(float currentDeltaTime) {
	// 如果当前帧耗费的时间小于目标帧时间,则等待剩余时间
	if (currentDeltaTime < mTargetFrameTime) {
		double timeToWait = mTargetFrameTime - currentDeltaTime;
		Uint64 nsToWait = static_cast<Uint64>(timeToWait * SDL_NS_PER_SECOND);
		SDL_DelayNS(nsToWait);
		mDeltaTime = static_cast<double>(SDL_GetTicksNS() - mLastTime) / SDL_NS_PER_SECOND;
	}
	// 否则使用当前帧耗费的时间
	else {
		// 限制最大 delta time 为 0.1秒 (即最低 10 FPS)，防止刚启动或卡顿时的物理穿模
		mDeltaTime = std::min(static_cast<double>(currentDeltaTime), 0.1);
	}
}

