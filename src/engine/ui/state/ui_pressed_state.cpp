#include "ui_pressed_state.h"
#include "ui_normal_state.h"
#include "ui_hover_state.h"
#include "../ui_interactive.h"
#include "../../input/input_manager.h"
#include "../../core/context.h"
#include <spdlog/spdlog.h>

namespace engine::ui::state {

void UIPressedState::enter() {
	mOwner->setSprite("pressed");
	mOwner->playSound("pressed");
	spdlog::debug("切换到按下状态");
}

std::unique_ptr<UIState> UIPressedState::handleInput(engine::core::Context& context) {
	auto& inputManager = context.getInputManager();
	auto mousePosition = inputManager.getLogicalMousePosition();
	if (inputManager.isActionReleased("MouseLeftClick")) {
		if (!mOwner->isPointInside(mousePosition)) {        // 松开鼠标时，如果不在UI元素内，则切换到正常状态
			return std::make_unique<engine::ui::state::UINormalState>(mOwner);
		}
		else {												// 松开鼠标时，如果还在UI元素内，则触发点击事件
			mOwner->clicked();
			return std::make_unique<engine::ui::state::UIHoverState>(mOwner);
		}
	}

	return nullptr;
}

} // namespace engine::ui::state