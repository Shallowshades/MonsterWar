#include "ui_label.h"
#include "../core/context.h"
#include "../render/text_renderer.h"
#include <spdlog/spdlog.h>

namespace engine::ui {
UILabel::UILabel(engine::render::TextRenderer& text_renderer,
	std::string_view text,
	std::string_view font_id,
	int font_size,
	const engine::utils::FColor& text_color,
	const glm::vec2& position)
	: UIElement(position),
	mTextRenderer(text_renderer),
	mText(text),
	mFontId(font_id),
	mFontSize(font_size),
	mTextFcolor(text_color) {
	// 获取文本渲染尺寸
	mSize = mTextRenderer.getTextSize(mText, mFontId, mFontSize);
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
	mSize = mTextRenderer.getTextSize(mText, mFontId, mFontSize);
}

void UILabel::setFontId(std::string_view font_id) {
	mFontId = font_id;
	mSize = mTextRenderer.getTextSize(mText, mFontId, mFontSize);
}

void UILabel::setFontSize(int font_size) {
	mFontSize = font_size;
	mSize = mTextRenderer.getTextSize(mText, mFontId, mFontSize);
}

void UILabel::setTextFColor(const engine::utils::FColor& text_fcolor) {
	mTextFcolor = text_fcolor;
	/* 颜色变化不影响尺寸 */
}
} // namespace engine::ui