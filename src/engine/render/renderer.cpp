#include "renderer.h"
#include "../resource/resource_manager.h"
#include "camera.h"
#include "sprite.h"
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
		// ResourceManager是drawSprite所必需的
		throw std::runtime_error(mLogTag.data() + std::string(" 构造失败: 提供mResourceManager指针为空"));
	}
	setDrawColor(0, 0, 0, 255);
	spdlog::trace("{} 构造成功", mLogTag.data());
}
void Renderer::drawSprite(const Camera& camera, const Sprite& sprite, const glm::vec2& positioin, const glm::vec2& scale, double angle) {
	auto texture = mResourceManager->getTexture(sprite.getTextureId());
	if (!texture) {
		spdlog::error("{} 无法为ID: {} 获取纹理.", mLogTag.data(), sprite.getTextureId());
		return;
	}

	auto srcRect = getSpriteSourceRect(sprite);
	if (!srcRect.has_value()) {
		spdlog::error("{} 无法获取精灵的源矩阵, ID: {}", mLogTag.data(), sprite.getTextureId());
		return;
	}

	// 应用相机变换
	glm::vec2 positionScreen = camera.worldToScreen(positioin);
	// 缩放后的纹理
	float scaledW = srcRect.value().w * scale.x;
	float scaledH = srcRect.value().h * scale.y;
	SDL_FRect destRect = {
		positionScreen.x,
		positionScreen.y,
		scaledW,
		scaledH
	};

	// 视口裁剪; 如果精灵超出视口, 则不绘制
	if (!isRectInViewPort(camera, destRect)) {
		// spdlog::info("{} 精灵超出视口范围, ID: {}", mLogTag.data(), sprite.getTextureId());
		return;
	}

	// 执行绘制(默认旋转中心为精灵的中心点)
	if (!SDL_RenderTextureRotated(mRenderer, texture, &srcRect.value(), &destRect, angle, nullptr, sprite.isFlipped() ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE)) {
		spdlog::error("{} 渲染旋转纹理失败(ID: {}) : {}", mLogTag.data(), sprite.getTextureId(), SDL_GetError());
	}
}

void Renderer::drawParallax(const Camera& camera, const Sprite& sprite, const glm::vec2& position, const glm::vec2& scrollFactor, const glm::bvec2& repeat, const glm::vec2& scale) {
	auto texture = mResourceManager->getTexture(sprite.getTextureId());
	if (!texture) {
		spdlog::error("{} 无法为ID: {} 获取纹理", mLogTag.data(), sprite.getTextureId());
		return;
	}

	auto srcRect = getSpriteSourceRect(sprite);
	if (!srcRect.has_value()) {
		spdlog::error("{} 无法获取精灵图的源矩阵, ID: {}", mLogTag.data(), sprite.getTextureId());
		return;
	}

	// 应用相机变换
	glm::vec2 positionScreen = camera.worldToScreenWithParallax(position,scrollFactor);
	// 缩放后的纹理
	float scaledW = srcRect.value().w * scale.x;
	float scaledH = srcRect.value().h * scale.y;

	glm::vec2 start, stop;
	glm::vec2 viewPortSize = camera.getViewPortSize();

	if (repeat.x) {
		// 使用glm::mod进行浮点数取模
		start.x = glm::mod(positionScreen.x, scaledW) - scaledW;
		stop.x = viewPortSize.x;
	}
	else {
		start.x = positionScreen.x;
		stop.x = glm::min(positionScreen.x + scaledW, viewPortSize.x);
	}

	if (repeat.y) {
		start.y = glm::mod(positionScreen.y, scaledH) - scaledH;
		stop.y = viewPortSize.y;
	}
	else {
		start.y = positionScreen.y;
		stop.y = glm::min(positionScreen.y + scaledH, viewPortSize.y);
	}

	for (float y = start.y; y < stop.y; y += scaledH) {
		for (float x = start.x; x < stop.x; x += scaledW) {
			SDL_FRect dstRect = { x, y, scaledW, scaledH };
			if (!SDL_RenderTexture(mRenderer, texture, nullptr, &dstRect)) {
				spdlog::error("{} 渲染视差纹理失败 (ID: {}) : {}", mLogTag.data(), sprite.getTextureId(), SDL_GetError());
				return;
			}
		}
	}
}

void Renderer::drawUISprite(const Sprite& sprite, const glm::vec2& position, const std::optional<glm::vec2>& size) {
	auto texture = mResourceManager->getTexture(sprite.getTextureId());
	if (!texture) {
		spdlog::error("{} 无法为ID: {} 获取纹理", mLogTag.data(), sprite.getTextureId());
		return;
	}

	auto srcRect = getSpriteSourceRect(sprite);
	if (!srcRect.has_value()) {
		spdlog::error("{} 无法获取精灵图的源矩阵, ID: {}", mLogTag.data(), sprite.getTextureId());
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
	if (!SDL_RenderTextureRotated(mRenderer, texture, &srcRect.value(), &dstRect, 0.0, nullptr, sprite.isFlipped() ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE)) {
		spdlog::error("{} 渲染 UI Sprite 失败 (ID: {}): {}", mLogTag.data(), sprite.getTextureId(), SDL_GetError());
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

std::optional<SDL_FRect> Renderer::getSpriteSourceRect(const Sprite& sprite) {
	SDL_Texture* texture = mResourceManager->getTexture(sprite.getTextureId());
	if (!texture) {
		spdlog::error("{} 无法为 ID {} 获取纹理", mLogTag.data(), sprite.getTextureId());
		return std::nullopt;
	}

	auto srcRect = sprite.getSourceRect();
	if (srcRect.has_value()) {
		if (srcRect.value().w <= 0 || srcRect.value().h <= 0) {
			spdlog::error("{} 源矩阵尺寸无效, ID: {}", mLogTag.data(), sprite.getTextureId());
			return std::nullopt;
		}
		return srcRect;
	}
	else {
		SDL_FRect result = { 0, 0, 0, 0 };
		if (!SDL_GetTextureSize(texture, &result.w, &result.h)) {
			spdlog::error("{} 无法获取纹理尺寸, ID: {}", mLogTag.data(), sprite.getTextureId());
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
