#include "ui_element.h"
#include "../core/context.h"
#include <algorithm>
#include <utility>
#include <spdlog/spdlog.h>

namespace engine::ui {
UIElement::UIElement(const glm::vec2& position, const glm::vec2& size)
	: mPosition(position), mSize(size) {
}

void UIElement::update(float deltaTime, engine::core::Context& context) {
	if (!mVisible) return;

	// 遍历所有子节点，并删除标记了移除的元素
	for (auto it = mChildren.begin(); it != mChildren.end();) {
		if (*it && !(*it)->isNeedRemove()) {
			(*it)->update(deltaTime, context);
			++it;
		}
		else {
			it = mChildren.erase(it);
		}
	}
}

void UIElement::render(engine::core::Context& context) {
	if (!mVisible) return;

	// 渲染子元素
	for (const auto& child : mChildren) {
		if (child) child->render(context);
	}
}

void UIElement::addChild(std::unique_ptr<UIElement> child, int orderIndex) {
	if (child) {
		if (orderIndex >= 0) child->setOrderIndex(orderIndex);  // 显式指定排序索引
		child->setParent(this); // 设置父指针
		mChildren.push_back(std::move(child));
	}
}

void UIElement::sortChildrenByOrderIndex() {
	// 稳定排序：orderIndex 小的在前，同索引保持插入顺序
	std::stable_sort(mChildren.begin(), mChildren.end(),
		[](const std::unique_ptr<UIElement>& a, const std::unique_ptr<UIElement>& b) {
			return a->getOrderIndex() < b->getOrderIndex();
		});
}

std::unique_ptr<UIElement> UIElement::removeChild(UIElement* child_ptr) {
	// 使用 std::remove_if 和 lambda 表达式自定义比较的方式移除
	auto it = std::find_if(mChildren.begin(), mChildren.end(),
		[child_ptr](const std::unique_ptr<UIElement>& p) {
			return p.get() == child_ptr;
		});

	if (it != mChildren.end()) {
		std::unique_ptr<UIElement> removedChild = std::move(*it);
		mChildren.erase(it);
		removedChild->setParent(nullptr);      // 清除父指针
		return removedChild;                   // 返回被移除的子元素（可以挂载到别处）
	}
	return nullptr; // 未找到子元素
}

std::unique_ptr<UIElement> UIElement::removeChildById(entt::id_type id) {
	auto it = std::find_if(mChildren.begin(), mChildren.end(),
		[id](const std::unique_ptr<UIElement>& p) {
			return p->getId() == id;
		});

	if (it != mChildren.end()) {
		std::unique_ptr<UIElement> removedChild = std::move(*it);
		mChildren.erase(it);
		removedChild->setParent(nullptr);      // 清除父指针
		return removedChild;                   // 返回被移除的子元素（可以挂载到别处）
	}
	return nullptr; // 未找到子元素
}

UIElement* UIElement::getChildById(entt::id_type id) {
	auto it = std::find_if(mChildren.begin(), mChildren.end(),
		[id](const std::unique_ptr<UIElement>& p) {
			return p->getId() == id;
		});

	if (it != mChildren.end()) {
		return it->get();
	}
	return nullptr;
}

void UIElement::removeAllChildren() {
	for (auto& child : mChildren) {
		child->setParent(nullptr); // 清除父指针
	}
	mChildren.clear();
}

glm::vec2 UIElement::getScreenPosition() const {
	if (mParent) {
		return mParent->getScreenPosition() + mPosition;
	}
	return mPosition; // 根元素的位置已经是相对屏幕的绝对位置
}

engine::utils::Rect UIElement::getBounds() const {
	auto absPosition = getScreenPosition();
	return engine::utils::Rect(absPosition, mSize);
}

bool UIElement::isPointInside(const glm::vec2& point) const {
	auto bounds = getBounds();
	return (point.x >= bounds.mPosition.x && point.x < (bounds.mPosition.x + bounds.mSize.x) &&
		point.y >= bounds.mPosition.y && point.y < (bounds.mPosition.y + bounds.mSize.y));
}
}
