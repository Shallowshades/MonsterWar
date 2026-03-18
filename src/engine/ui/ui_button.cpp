#include "ui_button.h"
#include "state/ui_normal_state.h"
#include <spdlog/spdlog.h>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace engine::ui {
UIButton::UIButton(engine::core::Context& context,
	std::string_view normalSpriteId,
	std::string_view hoverSpriteId,
	std::string_view pressedSpriteId,
	const glm::vec2& position,
	const glm::vec2& size,
	std::function<void()> callback)
	: UIInteractive(context, position, size), mCallback(std::move(callback))
{
	addSprite("normal"_hs, engine::render::Sprite(normalSpriteId));
	addSprite("hover"_hs, engine::render::Sprite(hoverSpriteId));
	addSprite("pressed"_hs, engine::render::Sprite(pressedSpriteId));

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