#include "ui_button.h"
#include "state/ui_normal_state.h"
#include <spdlog/spdlog.h>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace engine::ui {

UIButton::UIButton(engine::core::Context& context,
	engine::render::Image normalImage,
	engine::render::Image hoverImage,
	engine::render::Image pressedImage,
	const glm::vec2& position,
	const glm::vec2& size,
	std::function<void()> clickCallback,
	std::function<void()> hoverEnterCallback,
	std::function<void()> hoverLeaveCallback)
	: UIInteractive(context, position, size)
{
	addImage("normal"_hs, std::move(normalImage));
	addImage("hover"_hs, std::move(hoverImage));
	addImage("pressed"_hs, std::move(pressedImage));

	// 设置默认状态为"normal"
	setState(std::make_unique<engine::ui::state::UINormalState>(this));

	if (clickCallback) {
		setClickCallback(std::move(clickCallback));
	}
	if (hoverEnterCallback) {
		setHoverEnterCallback(std::move(hoverEnterCallback));
	}
	if (hoverLeaveCallback) {
		setHoverLeaveCallback(std::move(hoverLeaveCallback));
	}

	// 默认音效（本地保留：AudioPlayer 对未加载的音效会打印 error）
	setHoverSound("assets/audio/button_hover.wav");
	setClickSound("assets/audio/button_click.wav");
	spdlog::trace("UIButton 构造完成");
}

void UIButton::clicked() {
	if (mClickCallback) {
		mClickCallback();
	}
	playSound("pressed"_hs);
}

void UIButton::hover_enter() {
	if (mHoverEnterCallback) {
		mHoverEnterCallback();
	}
	playSound("hover"_hs);
}

void UIButton::hover_leave() {
	if (mHoverLeaveCallback) {
		mHoverLeaveCallback();
	}
}

void UIButton::setClickCallback(std::function<void()> callback) {
	mClickCallback = std::move(callback);
}

void UIButton::setHoverEnterCallback(std::function<void()> callback) {
	mHoverEnterCallback = std::move(callback);
}

void UIButton::setHoverLeaveCallback(std::function<void()> callback) {
	mHoverLeaveCallback = std::move(callback);
}

void UIButton::setHoverSound(std::string_view filePath) {
	addSound("hover"_hs, filePath);
}

void UIButton::setClickSound(std::string_view filePath) {
	addSound("pressed"_hs, filePath);
}

} // namespace engine::ui
