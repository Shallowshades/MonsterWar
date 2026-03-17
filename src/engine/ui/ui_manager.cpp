#include "ui_manager.h"
#include "ui_panel.h"
#include "ui_element.h"
#include <spdlog/spdlog.h>

namespace engine::ui {
UIManager::~UIManager() = default;

UIManager::UIManager() {
	// 创建一个无特定大小和位置的Panel，它的子元素将基于它定位。
	mRootElement = std::make_unique<UIPanel>(glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f });
	spdlog::trace("UI管理器构造完成。");
}

bool UIManager::init(const glm::vec2& windowSize) {
	mRootElement->setSize(windowSize);
	spdlog::trace("UI管理器已初始化根面板。");
	return true;
}

void UIManager::addElement(std::unique_ptr<UIElement> element) {
	if (mRootElement) {
		mRootElement->addChild(std::move(element));
	}
	else {
		spdlog::error("无法添加元素：mRootElement 为空！");
	}
}

void UIManager::clearElements() {
	if (mRootElement) {
		mRootElement->removeAllChildren();
		spdlog::trace("所有UI元素已从UI管理器中清除。");
	}
}

bool UIManager::handleInput(engine::core::Context& context) {
	if (mRootElement && mRootElement->isVisible()) {
		// 从根元素开始向下分发事件
		if (mRootElement->handleInput(context)) return true;
	}
	return false;
}

void UIManager::update(float deltaTime, engine::core::Context& context) {
	if (mRootElement && mRootElement->isVisible()) {
		// 从根元素开始向下更新
		mRootElement->update(deltaTime, context);
	}
}

void UIManager::render(engine::core::Context& context) {
	if (mRootElement && mRootElement->isVisible()) {
		// 从根元素开始向下渲染
		mRootElement->render(context);
	}
}

UIPanel* UIManager::getRootElement() const {
	return mRootElement.get();
}

} // namespace engine::ui
