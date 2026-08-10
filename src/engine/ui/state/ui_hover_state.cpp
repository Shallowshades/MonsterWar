#include "ui_hover_state.h"
#include "ui_normal_state.h"
#include "ui_pressed_state.h"
#include "../ui_interactive.h"
#include "../../input/input_manager.h"
#include "../../core/context.h"
#include <spdlog/spdlog.h>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace engine::ui::state {

	UIHoverState::UIHoverState(engine::ui::UIInteractive* owner) : UIState(owner) {
		mOwner->getContext().getInputManager().onAction("mouse_left"_hs).connect<&UIHoverState::onMousePressed>(this);
	}

	UIHoverState::~UIHoverState() {
		mOwner->getContext().getInputManager().onAction("mouse_left"_hs).disconnect<&UIHoverState::onMousePressed>(this);
	}

	void UIHoverState::enter() {
		mOwner->setCurrentImage("hover"_hs);
		mOwner->hover_enter();
		spdlog::debug("切换到悬停状态");
	}

	void UIHoverState::update(float, engine::core::Context& context) {
		auto& inputManager = context.getInputManager();
		auto mousePosition = inputManager.getLogicalMousePosition();
		if (!mOwner->isPointInside(mousePosition)) {			// 鼠标离开元素 → 通知离开并回到正常状态
			mOwner->hover_leave();
			mOwner->setNextState(std::make_unique<UINormalState>(mOwner));
		}
	}

	bool UIHoverState::onMousePressed() {
		mOwner->setNextState(std::make_unique<UIPressedState>(mOwner));
		return true;
	}

} // namespace engine::ui::state
