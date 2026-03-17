#include "ui_interactive.h"
#include "state/ui_state.h"
#include "../core/context.h"
#include "../render/renderer.h"
#include "../resource/resource_manager.h"
#include "../audio/audio_player.h"
#include <spdlog/spdlog.h>

namespace engine::ui {

UIInteractive::~UIInteractive() = default;

UIInteractive::UIInteractive(engine::core::Context& context, const glm::vec2& position, const glm::vec2& size)
	: UIElement(position, size), mContext(context)
{
	spdlog::trace("UIInteractive 构造完成");
}

void UIInteractive::setState(std::unique_ptr<engine::ui::state::UIState> state) {
	if (!state) {
		spdlog::warn("尝试设置空的状态！");
		return;
	}

	mState = std::move(state);
	mState->enter();
}

void UIInteractive::addSprite(std::string_view name, std::unique_ptr<engine::render::Sprite> sprite) {
	// 可交互UI元素必须有一个size用于交互检测，因此如果参数列表中没有指定，则用图片大小作为size
	if (mSize.x == 0.0f && mSize.y == 0.0f) {
		mSize = mContext.getResourceManager().getTextureSize(sprite->getTextureId());
	}
	// 添加精灵
	mSprites[std::string(name)] = std::move(sprite);
}

void UIInteractive::setSprite(std::string_view name) {
	if (mSprites.find(std::string(name)) != mSprites.end()) {
		mCurrentSprite = mSprites[std::string(name)].get();
	}
	else {
		spdlog::warn("Sprite '{}' 未找到", name.data());
	}
}

void UIInteractive::addSound(std::string_view name, std::string_view path) {
	mSounds[std::string(name)] = path;
}

void UIInteractive::playSound(std::string_view name) {
	if (mSounds.find(std::string(name)) != mSounds.end()) {
		mContext.getAudioPlayer().playSound(mSounds[std::string(name)]);
	}
	else {
		spdlog::error("Sound '{}' 未找到", name);
	}
}

bool UIInteractive::handleInput(engine::core::Context& context) {
	if (UIElement::handleInput(context)) {
		return true;
	}

	// 先更新子节点，再更新自己（状态）
	if (mState && mInteractive) {
		if (auto nextState = mState->handleInput(context); nextState) {
			setState(std::move(nextState));
			return true;
		}
	}
	return false;
}

void UIInteractive::render(engine::core::Context& context) {
	if (!mVisible) return;

	// 先渲染自身
	context.getRenderer().drawUISprite(*mCurrentSprite, getScreenPosition(), mSize);

	// 再渲染子元素（调用基类方法）
	UIElement::render(context);
}

} // namespace engine::ui