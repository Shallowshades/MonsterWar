#include "component.h"
#include "parallax_component.h"
#include "transform_component.h"
#include "../render/renderer.h"
#include "../render/camera.h"
#include "../render/sprite.h"
#include "../object/game_object.h"
#include "../core/context.h"
#include <spdlog/spdlog.h>

namespace engine::component {
ParallaxComponent::ParallaxComponent(std::string_view textureId, const glm::vec2& scrollFactor, const glm::bvec2& repeat)
	: mSprite(textureId)
	, mScrollFactor(scrollFactor)
	, mRepeat(repeat)
{
	spdlog::trace("{} 初始化完成, 纹理ID : {}", mLogTag.data(), textureId);
}
void ParallaxComponent::setSprite(const engine::render::Sprite& sprite) {
	mSprite = sprite;
}

void ParallaxComponent::setScrollFactor(const glm::vec2& scrollFactor) {
	mScrollFactor = scrollFactor;
}

void ParallaxComponent::setRepeat(const glm::bvec2& repeat) {
	mRepeat = repeat;
}

void ParallaxComponent::setHidden(bool hidden) {
	mIsHidden = hidden;
}

const engine::render::Sprite& ParallaxComponent::getSprite() const {
	return mSprite;
}

const glm::vec2& ParallaxComponent::getScrollFactor() const {
	return mScrollFactor;
}

const glm::bvec2& ParallaxComponent::getRepeat() const {
	return mRepeat;
}

bool ParallaxComponent::getIsHidden() const {
	return mIsHidden;
}

void ParallaxComponent::init() {
	if (!mOwner) {
		spdlog::error("{} 初始化时, GameObject为空", mLogTag.data());
		return;
	}
	
	mTransform = mOwner->getComponent<TransformComponent>();
	if (!mTransform) {
		spdlog::error("{} 初始化时, GameObject没有对应的TransformComponent组件");
		return;
	}
}

void ParallaxComponent::update(float, engine::core::Context&) {}

void ParallaxComponent::render(engine::core::Context& context) {
	if (mIsHidden || !mTransform) {
		return;
	}

	// 直接调用视差滚动绘制函数
	context.getRenderer().drawParallax(context.getCamera(), mSprite, mTransform->getPosition(), mScrollFactor, mRepeat, mTransform->getScale());
}
} // engine::component
