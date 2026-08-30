#include "game_state.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace engine::core {
GameState::GameState(SDL_Window* window, SDL_Renderer* renderer, State initialState)
	: mWindow(window), mRenderer(renderer), mCurrentState(initialState) {
	if (mWindow == nullptr || mRenderer == nullptr) {
		spdlog::error("窗口或渲染器为空");
		throw std::runtime_error("窗口或渲染器不能为空");
	}
	if (!syncLogicalPresentationState()) {
		spdlog::warn("无法读取初始逻辑分辨率，将在首次设置时更新缓存。");
	}
	spdlog::trace("游戏状态初始化完成");
}

void GameState::setState(State newState) {
	if (mCurrentState != newState) {
		spdlog::debug("游戏状态改变");
		mCurrentState = newState;
	}
	else {
		spdlog::debug("尝试设置相同的游戏状态，跳过");
	}
}

glm::vec2 GameState::getWindowSize() const {
	int width, height;
	// SDL3获取窗口大小的方法
	SDL_GetWindowSize(mWindow, &width, &height);
	return glm::vec2(width, height);
}

void GameState::setWindowSize(glm::vec2 newSize) {
	SDL_SetWindowSize(mWindow, static_cast<int>(newSize.x), static_cast<int>(newSize.y));
}

glm::vec2 GameState::getLogicalSize() const {
	int width = 0;
	int height = 0;
	SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
	if (!SDL_GetRenderLogicalPresentation(mRenderer, &width, &height, &mode)) {
		spdlog::error("获取逻辑分辨率失败: {}", SDL_GetError());
		return glm::vec2(mLogicalWidth, mLogicalHeight);
	}
	// SDL3.4 在逻辑分辨率被禁用时返回 0x0，此时回退到缓存值
	if (mode == SDL_LOGICAL_PRESENTATION_DISABLED &&
		mLogicalWidth > 0 && mLogicalHeight > 0) {
		return glm::vec2(mLogicalWidth, mLogicalHeight);
	}
	return glm::vec2(width, height);
}

void GameState::setLogicalSize(glm::vec2 newSize) {
	const int logicalWidth = static_cast<int>(newSize.x);
	const int logicalHeight = static_cast<int>(newSize.y);
	if (!SDL_SetRenderLogicalPresentation(mRenderer,
		logicalWidth,
		logicalHeight,
		SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
		spdlog::error("设置逻辑分辨率失败: {}", SDL_GetError());
		return;
	}
	mLogicalWidth = logicalWidth;
	mLogicalHeight = logicalHeight;
	mLogicalMode = SDL_LOGICAL_PRESENTATION_LETTERBOX;
	mLogicalPresentationDisabled = false;
	spdlog::trace("逻辑分辨率设置为: {}x{}", newSize.x, newSize.y);
}

bool GameState::disableLogicalPresentation() {
	int width = 0;
	int height = 0;
	SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
	if (!SDL_GetRenderLogicalPresentation(mRenderer, &width, &height, &mode)) {
		spdlog::error("读取当前逻辑分辨率失败: {}", SDL_GetError());
		return false;
	}
	// 已经是禁用状态，直接标记并返回（不重复设置）
	if (mode == SDL_LOGICAL_PRESENTATION_DISABLED) {
		mLogicalPresentationDisabled = true;
		return true;
	}

	// 先缓存当前配置，之后再真正关闭
	mLogicalWidth = width;
	mLogicalHeight = height;
	mLogicalMode = mode;
	if (!SDL_SetRenderLogicalPresentation(mRenderer,
		mLogicalWidth,
		mLogicalHeight,
		SDL_LOGICAL_PRESENTATION_DISABLED)) {
		spdlog::error("关闭逻辑分辨率失败: {}", SDL_GetError());
		return false;
	}
	mLogicalPresentationDisabled = true;
	return true;
}

bool GameState::enableLogicalPresentation() {
	// 未处于关闭状态，无需恢复
	if (!mLogicalPresentationDisabled) {
		return true;
	}
	// 没有可恢复的配置，无法恢复
	if (mLogicalWidth <= 0 || mLogicalHeight <= 0 ||
		mLogicalMode == SDL_LOGICAL_PRESENTATION_DISABLED) {
		spdlog::error("没有可恢复的逻辑分辨率配置。");
		return false;
	}
	if (!SDL_SetRenderLogicalPresentation(mRenderer,
		mLogicalWidth,
		mLogicalHeight,
		mLogicalMode)) {
		spdlog::error("恢复逻辑分辨率失败: {}", SDL_GetError());
		return false;
	}
	mLogicalPresentationDisabled = false;
	return true;
}

bool GameState::syncLogicalPresentationState() {
	int width = 0;
	int height = 0;
	SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
	if (!SDL_GetRenderLogicalPresentation(mRenderer, &width, &height, &mode)) {
		spdlog::error("同步逻辑分辨率状态失败: {}", SDL_GetError());
		return false;
	}
	// 仅在逻辑分辨率启用时更新缓存（禁用状态保持缓存不变）
	if (mode != SDL_LOGICAL_PRESENTATION_DISABLED) {
		mLogicalWidth = width;
		mLogicalHeight = height;
		mLogicalMode = mode;
		mLogicalPresentationDisabled = false;
	}
	return true;
}

} // namespace engine::core