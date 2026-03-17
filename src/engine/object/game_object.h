/*****************************************************************//**
 * @file   game_object.h
 * @brief  游戏对象类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.10.22
 *********************************************************************/

#pragma once
#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <utility>
#include <string>
#include <string_view>
#include <spdlog/spdlog.h>
#include "../component/component.h"

namespace engine::core {
	class Context;
}

namespace engine::object {
/**
 * @brief 游戏对象类, 负责管理游戏对象的组件.
 * 
 * 该类管理游戏对象的组件, 并提供添加,获取,检查和移除组件的功能.
 * 他还提供更新和渲染游戏对象的方法.
 */
class GameObject final {
public:
	/**
	 * @brief 构造函数.
	 * 
	 * @param name 名称
	 * @param tag 标签
	 */
	GameObject(std::string_view name = "", std::string_view tag = "");

	// 禁用拷贝和移动语义
	GameObject(const GameObject&) = delete;							///< @brief 删除拷贝构造
	GameObject& operator=(const GameObject&) = delete;				///< @brief 删除拷贝赋值构造
	GameObject(GameObject&&) = delete;								///< @brief 删除移动构造
	GameObject& operator=(GameObject&&) = delete;					///< @brief 删除移动赋值构造

	void setName(std::string_view name);							///< @brief 设置名称
	std::string_view getName() const;								///< @brief 获取名称
	void setTag(std::string_view tag);								///< @brief 设置标签
	std::string_view getTag() const;								///< @brief 获取标签
	void setNeedRemove(bool needRemove);							///< @brief 设置是否需要删除
	bool isNeedRemove() const;										///< @brief 获取是否需要删除

	/// 组件相关函数

	/**
	 * @brief 添加组件 里面完成组件的init.
	 * 
	 * @tparam T 组件类型
	 * @tparam Args 组件构造函数参数类型
	 * @param args 组件构造函数参数
	 * @return 组件指针
	 */
	template<typename T, typename... Args>
	T* addComponent(Args&&... args);

	/**
	 * @brief 获取组件.
	 * 
	 * @tparam T 组件类型
	 * @return 组件指针
	 */
	template<typename T>
	T* getComponent() const;

	/**
	 * @brief 检查是否存在组件.
	 * 
	 * @tparam T 组件类型
	 * @return 是否存在组件
	 */
	template<typename T>
	bool hasComponent() const;

	/**
	 * @brief 移除组件.
	 * 
	 * @tparam T 组件类型
	 */
	template<typename T>
	void removeComponent();

	// 关键循环函数
	void update(float deltaTime, engine::core::Context& context);	///< @brief 更新所有组件
	void render(engine::core::Context& context);						///< @brief 渲染所有组件
	void clean();													///< @brief 清理所有组件
	void handleInput(engine::core::Context& context);				///< @brief 处理输入

private:
	static constexpr std::string_view mLogTag = "GameObject";

	std::string mName;
	std::string mTag;
	std::unordered_map<std::type_index, std::unique_ptr<engine::component::Component>> mComponents;
	bool mNeedRemove = false;
};

// 模板函数写于同一文件

template<typename T, typename ...Args>
inline T* GameObject::addComponent(Args && ...args) {
	// 检测组件是否合法
	// static_assert(condition, message); 静态断言, 编译期检测, 无任何性能影响
	// std::is_base_of<Base, Derived>::value 判断Base是否是Derived的基类
	static_assert(std::is_base_of<engine::component::Component, T>::value, "T 必须继承自Componet");

	// 获取类型标识
	auto typeIndex = std::type_index(typeid(T));
	// 如果组件已存在, 则直接返回组件指针
	if (hasComponent<T>()) {
		return getComponent<T>();
	}
	// 如果不存在则创建组件, args为组件构造时带的参数
	auto component = std::make_unique<T>(std::forward<Args>(args)...);
	T* ptr = component.get();
	component->setOwner(this);
	mComponents[typeIndex] = std::move(component);
	ptr->init();
	spdlog::debug("{} addComponent: {} added component {}", mLogTag.data(), mName, typeid(T).name());
	return ptr;
}

template<typename T>
inline T* GameObject::getComponent() const {
	static_assert(std::is_base_of<engine::component::Component, T>::value, "T 必须是继承自Component");
	auto typeIndex = std::type_index(typeid(T));
	auto iter = mComponents.find(typeIndex);
	if (iter != mComponents.end()) {
		// 必定为T类型, 显示转换只是为了易读
		return static_cast<T*>(iter->second.get());
	}
	return nullptr;
}

template<typename T>
inline bool GameObject::hasComponent() const {
	static_assert(std::is_base_of<engine::component::Component, T>::value, "T 必须是继承自Component");
	return mComponents.contains(std::type_index(typeid(T)));	// C++20新增
}

template<typename T>
inline void GameObject::removeComponent() {
	static_assert(std::is_base_of<engine::component::Component, T>::value, "T 必须是继承自Component");
	auto tyoeIndex = std::type_index(typeid(T));
	auto iter = mComponents.find(tyoeIndex);
	if (iter != mComponents.end()) {
		iter->second->clean();
		mComponents.erase(iter);
	}
}
} // namespace engine::object

#endif // GAME_OBJECT_H
