#include "ui_normal_state.h"
#include "ui_hover_state.h"
#include "../ui_interactive.h"
#include "../../input/input_manager.h"
#include "../../core/context.h"
#include "../../audio/audio_player.h"
#include <spdlog/spdlog.h>

namespace engine::ui::state {

void UINormalState::enter() {
	mOwner->setSprite("normal");
	spdlog::debug("切换到正常状态");
}

std::unique_ptr<UIState> UINormalState::handleInput(engine::core::Context& context) {
	auto& inputManager = context.getInputManager();
	auto mousePosition = inputManager.getLogicalMousePosition();
	if (mOwner->isPointInside(mousePosition)) {         // 如果鼠标在UI元素内，则切换到悬停状态
		mOwner->playSound("hover");
		return std::make_unique<engine::ui::state::UIHoverState>(mOwner);
	}
	return nullptr;
}

} // namespace engine::ui::state