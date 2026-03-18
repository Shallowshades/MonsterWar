#include "ui_button.h"
#include "state/ui_normal_state.h"
#include <spdlog/spdlog.h>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace engine::ui {
UIButton::UIButton(engine::core::Context& context,
	std::string_view normalImageId,
	std::string_view hoverImageId,
	std::string_view pressedImageId,
	const glm::vec2& position,
	const glm::vec2& size,
	std::function<void()> callback)
	: UIInteractive(context, position, size), mCallback(std::move(callback))
{
	addImage("normal"_hs, engine::render::Image(normalImageId));
	addImage("hover"_hs, engine::render::Image(hoverImageId));
	addImage("pressed"_hs, engine::render::Image(pressedImageId));

	// 设置默认状态为"normal"
	setState(std::make_unique<engine::ui::state::UINormalState>(this));

	// 设置默认音效
	addSound("hover"_hs, "assets/audio/button_hover.wav"_hs);
	addSound("pressed"_hs, "assets/audio/button_click.wav"_hs);
	spdlog::trace("UIButton 构造完成");
}

void UIButton::clicked()
{
	if (mCallback) mCallback();
}

} // namespace engine::ui