#include "ui_pressed_state.h"
#include "ui_normal_state.h"
#include "ui_hover_state.h"
#include "../ui_interactive.h"
#include "../../input/input_manager.h"
#include "../../core/context.h"
#include <spdlog/spdlog.h>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace engine::ui::state {

	UIPressedState::UIPressedState(engine::ui::UIInteractive* owner) : UIState(owner) {
		mOwner->getContext().getInputManager().onAction("mouse_left"_hs, engine::input::ActionState::RELEASED)
			.connect<&UIPressedState::onMouseReleased>(this);
	}

	UIPressedState::~UIPressedState() {
		mOwner->getContext().getInputManager().onAction("mouse_left"_hs, engine::input::ActionState::RELEASED)
			.disconnect<&UIPressedState::onMouseReleased>(this);
	}

	void UIPressedState::enter() {
		mOwner->setCurrentImage("pressed"_hs);
		spdlog::debug("切换到按下状态");
	}

	void UIPressedState::update(float, engine::core::Context& context) {
		auto& inputManager = context.getInputManager();
		auto mousePosition = inputManager.getLogicalMousePosition();
		if (!mOwner->isPointInside(mousePosition)) {		// 按住状态下鼠标滑出元素 → 回到正常状态（不触发点击）
			mOwner->setNextState(std::make_unique<UINormalState>(mOwner));
		}
	}

	bool UIPressedState::onMouseReleased() {
		auto& inputManager = mOwner->getContext().getInputManager();
		if (mOwner->isPointInside(inputManager.getLogicalMousePosition())) {	// 在元素内松开 → 触发点击并回到悬停状态
			mOwner->clicked();
			mOwner->setNextState(std::make_unique<UIHoverState>(mOwner));
		}
		else {																	// 在元素外松开 → 回到正常状态
			mOwner->setNextState(std::make_unique<UINormalState>(mOwner));
		}
		return true;
	}

} // namespace engine::ui::state
