#include "sprite_component.h"
#include "transform_component.h"
#include "../object/game_object.h"
#include "../core/context.h"
#include "../render/renderer.h"
#include "../resource/resource_manager.h"
#include "../render/camera.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace engine::component {
SpriteComponent::SpriteComponent(std::string_view textureId, engine::resource::ResourceManager& resourceManager, engine::utils::Alignment alignment, std::optional<SDL_FRect> sourceRectOptional, bool isFlipped)
	: mResourceManager(&resourceManager)
	, mSprite(textureId, sourceRectOptional, isFlipped)
	, mAlignment(alignment)
{
	if (!mResourceManager) {
		// 游戏主循环不要使用try...catch/throw, 会极大影响性能
		spdlog::critical("{} 创建 SpriteComponent 时 ResourceManager 为空!. 此组件无效.", mLogTag.data());
	}
}

SpriteComponent::SpriteComponent(engine::render::Sprite&& sprite, engine::resource::ResourceManager& resourceManager, engine::utils::Alignment alignment) 
	: mResourceManager(&resourceManager), mSprite(std::move(sprite)), mAlignment(alignment)
{
	if (!mResourceManager) {
		spdlog::critical("{} 创建SpriteComponent时ResourceManager为空! 此组件无效!", mLogTag.data());
		// 游戏主循环不要使用try...catch / throw, 会极大的影响性能
	}
	spdlog::trace("{} 创建SpriteComponent, 纹理ID: {}", mLogTag.data(), mSprite.getTextureId());
}

void SpriteComponent::updateOffset() {
	if (mSpriteSize.x <= 0 || mSpriteSize.y <= 0) {
		mOffset = { 0.f, 0.f };
		return;
	}

	auto scale = mTransform->getScale();
	switch (mAlignment) {
	case engine::utils::Alignment::NONE:
		break;
	case engine::utils::Alignment::TOP_LEFT:
		mOffset = glm::vec2{ 0.f, 0.f } *scale;
		break;
	case engine::utils::Alignment::TOP_CENTER:
		mOffset = glm::vec2{ -mSpriteSize.x / 2.f, 0.f } * scale;
		break;
	case engine::utils::Alignment::TOP_RIGHT:
		mOffset = glm::vec2{ -mSpriteSize.x, 0.f } * scale;
		break;
	case engine::utils::Alignment::CENTER_LEFT:
		mOffset = glm::vec2{ 0.f, -mSpriteSize.y / 2.f } * scale;
		break;
	case engine::utils::Alignment::CENTER:
		mOffset = glm::vec2{ -mSpriteSize.x / 2.f, -mSpriteSize.y / 2.f } * scale;
		break;
	case engine::utils::Alignment::CENTER_RIGHT:
		mOffset = glm::vec2{ -mSpriteSize.x, -mSpriteSize.y / 2.f } * scale;
		break;
	case engine::utils::Alignment::BOTTOM_LEFT:
		mOffset = glm::vec2{ 0.f, -mSpriteSize.y } *scale;
		break;
	case engine::utils::Alignment::BOTTOM_CENTER:
		mOffset = glm::vec2{ -mSpriteSize.x / 2.f, -mSpriteSize.y } * scale;
		break;
	case engine::utils::Alignment::BOTTOM_RIGHT:
		mOffset = glm::vec2{ -mSpriteSize.x, -mSpriteSize.y } * scale;
		break;
	default:
		break;
	}
}

const engine::render::Sprite& SpriteComponent::getSprite() const {
	return mSprite;
}

std::string_view SpriteComponent::getTextureId() const {
	return mSprite.getTextureId();
}

bool SpriteComponent::isFlipped() const {
	return mSprite.isFlipped();
}

bool SpriteComponent::isHidden() const {
	return mIsHidden;
}

const glm::vec2& SpriteComponent::getSpriteSize() const {
	return mSpriteSize;
}

const glm::vec2& SpriteComponent::getOffset() const {
	return mOffset;
}

engine::utils::Alignment SpriteComponent::getAlignment() const {
	return mAlignment;
}

void SpriteComponent::setSpriteById(std::string_view textureId, const std::optional<SDL_FRect>& sourceRectOptional) {
	mSprite.setTextureId(textureId);
	mSprite.setSourceRect(sourceRectOptional);

	updateSpriteSize();
	updateOffset();
}

void SpriteComponent::setFlipped(bool flipped) {
	mSprite.setFlipped(flipped);
}

void SpriteComponent::setHidden(bool hidden) {
	mIsHidden = hidden;
}

void SpriteComponent::setSourceRect(const std::optional<SDL_FRect>& sourceRectOptional) {
	mSprite.setSourceRect(sourceRectOptional);
	updateSpriteSize();
	updateOffset();
}

void SpriteComponent::setAlignment(engine::utils::Alignment anchor) {
	mAlignment = anchor;
	updateOffset();
}

void SpriteComponent::updateSpriteSize() {
	if (!mResourceManager) {
		spdlog::error("{} ResourceManager 为空! 无法获取纹理尺寸.", mLogTag.data());
		return;
	}
	if (mSprite.getSourceRect().has_value()) {
		const auto& srcRect = mSprite.getSourceRect().value();
		mSpriteSize = { srcRect.w, srcRect.h };
	}
	else {
		mSpriteSize = mResourceManager->getTextureSize(mSprite.getTextureId());
	}
}

void SpriteComponent::init() {
	if (!mOwner) {
		spdlog::error("{} SpriteComponent 在初始化前未设置所有者.", mLogTag.data());
		return;
	}
	mTransform = mOwner->getComponent<TransformComponent>();
	if (!mTransform) {
		spdlog::warn("{} GameObject '{}' 上的 SpriteComponent 需要一个 TransformComponent, 但未找到.", mLogTag.data(), mOwner->getName());
		return;
	}
	// 获取大小及偏移
	updateSpriteSize();
	updateOffset();
}

void SpriteComponent::render(engine::core::Context& context) {
	if (mIsHidden || !mTransform || !mResourceManager) {
		return;
	}

	// 获取变换信息(考虑偏移量)
	const glm::vec2& position = mTransform->getPosition() + mOffset;
	const glm::vec2& scale = mTransform->getScale();
	float rotation = mTransform->getRotation();

	// 执行绘制
	context.getRenderer().drawSprite(context.getCamera(), mSprite, position, scale, rotation);
}
}
