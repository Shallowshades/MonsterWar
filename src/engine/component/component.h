/*****************************************************************//**
 * @file   component.h
 * @brief  游戏对象组件类
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2025.10.22
 *********************************************************************/

#pragma once
#ifndef COMPONENT_H
#define COMPONENT_H

namespace engine::object {
	class GameObject;
}

namespace engine::core {
	class Context;
}

namespace engine::component {
/**
* @brief 组件的抽象基类.
*
* 所有具体组件都应从此类继承
* 定义组件生命周期可能调用的通用方法.
*/
class Component {
	friend class engine::object::GameObject;
public:
	Component() = default;									///< @brief 默认构造
	virtual ~Component() = default;							///< @brief 虚构造函数确保正确清理派生类

	// 禁用拷贝和移动语义, 组件通常不应被拷贝或移动(更改mOwner相当于移动)
	Component(const Component&) = delete;					///< @brief 删除拷贝构造
	Component& operator=(const Component&) = delete;		///< @brief 删除拷贝赋值构造
	Component(Component&&) = delete;						///< @brief 删除移动构造
	Component& operator=(Component&&) = delete;				///< @brief 删除移动赋值构造

	void setOwner(engine::object::GameObject* owner);		///< @brief 设置拥有此组件的GameObject
	engine::object::GameObject* getOwner() const;			///< @brief 获取拥有此组件的GameObject

protected:
	// 关键循环函数, 全部设为保护, 只有GameObject需要(可以)调用
	// 其中改为纯虚函数
	virtual void init() {};									///< @brief 保留两段初始化机制, GameObject添加组件时自动调用, 不需要外部调用
	virtual void handleInput(engine::core::Context&) {};	///< @brief 处理输入
	virtual void update(float, engine::core::Context&) = 0;	///< @brief 更新, 必须实现
	virtual void render(engine::core::Context&) {};			///< @brief 渲染
	virtual void clean() {};								///< @brief 清理
protected:
	engine::object::GameObject* mOwner = nullptr;			///< @brief 指向拥有此组件的GameObject
};
}

#endif // COMPONENT_H
