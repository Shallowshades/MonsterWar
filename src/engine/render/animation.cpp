#include "animation.h"
#include <glm/common.hpp>
#include <spdlog/spdlog.h>

namespace engine::render {
Animation::Animation(std::string_view name, bool loop)
	: mName(name), mLoop(loop)
{
}

void Animation::addFrame(const SDL_FRect& sourceRect, float duration) {
	if (duration <= 0.f) {
		spdlog::warn("{} : 尝试向动画 '{}' 添加无效持续时间的帧", mLogTag.data(), mName);
		return;
	}

	mFrames.push_back(AnimationFrame(sourceRect, duration));
	mTotalDuration += duration;
}

const AnimationFrame& Animation::getFrame(float time) const {
	if (mFrames.empty()) {
		spdlog::error("{} : 动画 '{}' 没有帧, 无法获取帧", mLogTag.data(), mName);
		return mFrames.back();
	}

	float currentTime = time;
	if (mLoop && mTotalDuration > 0.f) {
		// 对循环动画使用模运算获取有效时间
		currentTime = glm::mod(time, mTotalDuration);
	}
	else {
		// 对于非循环动画, 如果时间超过总时长, 则停留在最后一帧
		if (currentTime >= mTotalDuration) {
			return mFrames.back();
		}
	}

	// 遍历帧以找到正确的帧
	float accumulatedTime = 0.f;
	for (const auto& frame : mFrames) {
		accumulatedTime += frame.mDuration;
		if (currentTime < accumulatedTime) {
			return frame;
		}
	}
	
	// 理论上在不应到达这里, 但为了安全起见, 返回最后一帧
	spdlog::warn("{} : 动画 '{}' 在获取帧信息时出现错误.", mLogTag.data(), mName);
	return mFrames.back();
}

std::string_view Animation::getName() const {
	return mName;
}

void Animation::setName(std::string_view name) {
	mName = name;
}

const std::vector<AnimationFrame>& Animation::getFrames() const {
	return mFrames;
}

size_t Animation::getFrameCount() const {
	return mFrames.size();
}

float Animation::getTotalDuration() const {
	return mTotalDuration;
}

bool Animation::getIsLooping() const {
	return mLoop;
}

void Animation::setLooping(bool looping) {
	mLoop = looping;
}

bool Animation::getIsEmpty() const {
	return mFrames.empty();
}
} // namespace engine::render
