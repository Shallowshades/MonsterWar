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
	int width, height;
	// SDL3获取逻辑分辨率的方法
	SDL_GetRenderLogicalPresentation(mRenderer, &width, &height, NULL);
	return glm::vec2(width, height);
}

void GameState::setLogicalSize(glm::vec2 newSize) {
	SDL_SetRenderLogicalPresentation(mRenderer,
		static_cast<int>(newSize.x),
		static_cast<int>(newSize.y),
		SDL_LOGICAL_PRESENTATION_LETTERBOX);
	spdlog::trace("逻辑分辨率设置为: {}x{}", newSize.x, newSize.y);
}

} // namespace engine::core 