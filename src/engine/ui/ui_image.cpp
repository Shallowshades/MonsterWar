#include "ui_image.h"
#include "../render/renderer.h"
#include "../render/sprite.h"
#include "../core/context.h"
#include <spdlog/spdlog.h>

namespace engine::ui {
UIImage::UIImage(std::string_view textureId,
	const glm::vec2& position,
	const glm::vec2& size,
	const std::optional<SDL_FRect>& sourceRect,
	bool isFlipped)
	: UIElement(position, size),
	mSprite(textureId, sourceRect, isFlipped)
{
	if (textureId.empty()) {
		spdlog::warn("创建了一个空纹理ID的UIImage。");
	}
	spdlog::trace("UIImage 构造完成");
}

void UIImage::render(engine::core::Context& context) {
	if (!mVisible || mSprite.getTextureId().empty()) {
		return; // 如果不可见或没有分配纹理则不渲染
	}

	// 渲染自身
	auto position = getScreenPosition();
	if (mSize.x == 0.0f && mSize.y == 0.0f) {   // 如果尺寸为0，则使用纹理的原始尺寸
		context.getRenderer().drawUISprite(mSprite, position);
	}
	else {
		context.getRenderer().drawUISprite(mSprite, position, mSize);
	}

	// 渲染子元素（调用基类方法）
	UIElement::render(context);
}
} // namespace engine::ui
