#include "ui_state.h"
#include "../../core/context.h"

namespace engine::ui::state {
UIState::UIState(engine::ui::UIInteractive* owner) : mOwner(owner) {

}

void UIState::update(float, engine::core::Context&) {

}
}
