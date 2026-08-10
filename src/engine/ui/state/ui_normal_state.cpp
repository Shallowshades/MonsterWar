#include "ui_normal_state.h"
#include "ui_hover_state.h"
#include "../ui_interactive.h"
#include "../../input/input_manager.h"
#include "../../core/context.h"
#include "../../audio/audio_player.h"
#include <spdlog/spdlog.h>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace engine::ui::state {

void UINormalState::enter() {
	mOwner->setCurrentImage("normal"_hs);
	spdlog::debug("切换到正常状态");
}

void UINormalState::update(float, engine::core::Context& context) {
	auto& inputManager = context.getInputManager();
	auto mousePosition = inputManager.getLogicalMousePosition();
	if (mOwner->isPointInside(mousePosition)) {         // 如果鼠标进入UI元素内，则延迟切换到悬停状态
		mOwner->setNextState(std::make_unique<UIHoverState>(mOwner));
	}
}

} // namespace engine::ui::state