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

void UIInteractive::addImage(entt::id_type nameId, engine::render::Image image) {
	// 可交互UI元素必须有一个size用于交互检测，因此如果参数列表中没有指定，则用图片大小作为size
	if (mSize.x == 0.0f && mSize.y == 0.0f) {
		mSize = mContext.getResourceManager().getTextureSize(image.getTextureId());
	}
	// 添加精灵
	mImages.emplace(nameId, std::move(image));
}

void UIInteractive::setImage(entt::id_type nameId) {
	if (mImages.find(nameId) != mImages.end()) {
		mCurrentImageId = nameId;
	}
	else {
		spdlog::warn("Image '{}' 未找到", nameId);
	}
}

void UIInteractive::addSound(entt::id_type nameId, entt::hashed_string hashedPath) {
	// 插入容器
	mSounds.emplace(nameId, hashedPath.value());

	// 载入音效资源
	mContext.getResourceManager().loadSound(hashedPath);
}

void UIInteractive::playSound(entt::id_type nameId) {
	if (auto it = mSounds.find(nameId); it != mSounds.end()) {
		if (mContext.getAudioPlayer().playSound(it->second) == -1) {
			spdlog::warn("UIInteractive : sound {} 未找到或无法播放", nameId);
		}
	}
	else {
		if (mContext.getAudioPlayer().playSound(nameId) == -1) {
			spdlog::error("Sound '{}' 未找到或无法播放", nameId);
		}
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
	context.getRenderer().drawUIImage(mImages[mCurrentImageId], getScreenPosition(), mSize);

	// 再渲染子元素（调用基类方法）
	UIElement::render(context);
}

} // namespace engine::ui