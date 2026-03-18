#include "renderer.h"
#include "../resource/resource_manager.h"
#include "camera.h"
#include "image.h"
#include <SDL3/SDL.h>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace engine::render {
Renderer::Renderer(SDL_Renderer* renderer, engine::resource::ResourceManager* resourceManager)
	: mRenderer(renderer), mResourceManager(resourceManager)
{
	spdlog::trace("{} 构造Renderer...", mLogTag.data());
	if (!mRenderer) {
		throw std::runtime_error(mLogTag.data() + std::string(" 构造失败: 提供的SDL_Renderer指针为空"));
	}
	if (!mResourceManager) {
		throw std::runtime_error(mLogTag.data() + std::string(" 构造失败: 提供mResourceManager指针为空"));
	}
	setDrawColor(0, 0, 0, 255);
	spdlog::trace("{} 构造成功", mLogTag.data());
}

void Renderer::drawUIImage(const Image& image, const glm::vec2& position, const std::optional<glm::vec2>& size) {
	auto texture = mResourceManager->getTexture(image.getTextureId());
	if (!texture) {
		spdlog::error("{} 无法为ID: {} 获取纹理", mLogTag.data(), image.getTextureId());
		return;
	}

	auto srcRect = getImageSourceRect(image);
	if (!srcRect.has_value()) {
		spdlog::error("{} 无法获取精灵图的源矩阵, ID: {}", mLogTag.data(), image.getTextureId());
		return;
	}

	SDL_FRect dstRect = { position.x, position.y, 0, 0 };
	if (size.has_value()) {
		dstRect.w = size.value().x;
		dstRect.h = size.value().y;
	}
	else {
		dstRect.w = srcRect.value().w;
		dstRect.h = srcRect.value().h;
	}

	// 执行绘制(未考虑UI旋转)
	if (!SDL_RenderTextureRotated(mRenderer, texture, &srcRect.value(), &dstRect, 0.0, nullptr, image.isFlipped() ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE)) {
		spdlog::error("{} 渲染 UI Image 失败 (ID: {}): {}", mLogTag.data(), image.getTextureId(), SDL_GetError());
	}
}

void Renderer::drawUIFilledRect(const engine::utils::Rect& rect, const engine::utils::FColor& color) {
	setDrawColorFloat(color.r, color.g, color.b, color.a);
	SDL_FRect sdlRect = { rect.position.x, rect.position.y, rect.size.x, rect.size.y };
	if (!SDL_RenderFillRect(mRenderer, &sdlRect)) {
		spdlog::error("{} 绘制填充矩形失败: {}", mLogTag.data(), SDL_GetError());
	}
	setDrawColorFloat(0.f, 0.f, 0.f, 1.f);
}

void Renderer::present() {
	SDL_RenderPresent(mRenderer);
}

void Renderer::clearScreen() {
	if (!SDL_RenderClear(mRenderer)) {
		spdlog::error("{} 清除渲染器失败: {}", mLogTag.data(), SDL_GetError());
	}
}

void Renderer::setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	if (!SDL_SetRenderDrawColor(mRenderer, r, g, b, a)) {
		spdlog::error("{} 设置渲染器绘制颜色失败: {}", mLogTag.data(), SDL_GetError());
	}
}

void Renderer::setDrawColorFloat(float r, float g, float b, float a) {
	if (!SDL_SetRenderDrawColorFloat(mRenderer, r, g, b, a)) {
		spdlog::error("{} 设置渲染器绘制颜色失败: {}", mLogTag.data(), SDL_GetError());
	}
}

SDL_Renderer* Renderer::getSDLRenderer() const {
	return mRenderer;
}

std::optional<SDL_FRect> Renderer::getImageSourceRect(const Image& image) {
	SDL_Texture* texture = mResourceManager->getTexture(image.getTextureId());
	if (!texture) {
		spdlog::error("{} 无法为 ID {} 获取纹理", mLogTag.data(), image.getTextureId());
		return std::nullopt;
	}

	auto srcRect = image.getSourceRect();
	if (srcRect.has_value()) {
		if (srcRect.value().size.x <= 0 || srcRect.value().size.y <= 0) {
			spdlog::error("{} 源矩阵尺寸无效, ID: {}, path : {}", mLogTag.data(), image.getTextureId(), image.getTexturePath());
			return std::nullopt;
		}
		return SDL_FRect{
			srcRect.value().position.x,
			srcRect.value().position.y,
			srcRect.value().size.x,
			srcRect.value().size.y
		};
	}
	else {
		SDL_FRect result = { 0, 0, 0, 0 };
		if (!SDL_GetTextureSize(texture, &result.w, &result.h)) {
			spdlog::error("{} 无法获取纹理尺寸, ID: {}, Path: {}", mLogTag.data(), image.getTextureId(), image.getTexturePath());
			return std::nullopt;
		}
		return result;
	}
}

bool Renderer::isRectInViewPort(const Camera& camera, const SDL_FRect& rect) {
	glm::vec2 viewPortSize = camera.getViewPortSize();
	// 相当于AABB碰撞检测
	return rect.x + rect.w >= 0 && rect.x <= viewPortSize.x && rect.y + rect.h >= 0 && rect.y <= viewPortSize.y;
}
} // engine::render
