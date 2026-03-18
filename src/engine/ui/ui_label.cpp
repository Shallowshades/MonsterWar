#include "ui_label.h"
#include "../core/context.h"
#include "../render/text_renderer.h"
#include <spdlog/spdlog.h>

namespace engine::ui {
UILabel::UILabel(engine::render::TextRenderer& text_renderer,
	std::string_view text,
	std::string_view fontPath,
	int font_size,
	const engine::utils::FColor& text_color,
	const glm::vec2& position)
	: UIElement(position),
	mTextRenderer(text_renderer),
	mText(text),
	mFontPath(fontPath),
	mFontId(entt::hashed_string(fontPath.data())),
	mFontSize(font_size),
	mTextFcolor(text_color) {
	// 获取文本渲染尺寸
	mSize = mTextRenderer.getTextSize(mText, mFontId, mFontSize, fontPath);
	spdlog::trace("UILabel 构造完成");
}

void UILabel::render(engine::core::Context& context) {
	if (!mVisible || mText.empty()) return;

	mTextRenderer.drawUIText(mText, mFontId, mFontSize, getScreenPosition(), mTextFcolor);

	// 渲染子元素（调用基类方法）
	UIElement::render(context);
}

void UILabel::setText(std::string_view text) {
	mText = text;
	mSize = mTextRenderer.getTextSize(mText, mFontId, mFontSize, mFontPath);
}

void UILabel::setFontPath(std::string_view fontPath) {
	mFontPath = fontPath;
	mFontId = entt::hashed_string(fontPath.data());
	mSize = mTextRenderer.getTextSize(mText, mFontId, mFontSize, mFontPath);
}

void UILabel::setFontSize(int font_size) {
	mFontSize = font_size;
	mSize = mTextRenderer.getTextSize(mText, mFontId, mFontSize, mFontPath);
}

void UILabel::setTextFColor(const engine::utils::FColor& text_fcolor) {
	mTextFcolor = text_fcolor;
	/* 颜色变化不影响尺寸 */
}
} // namespace engine::ui