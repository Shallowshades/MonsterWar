/*****************************************************************//**
 * @file   ui_state.h
 * @brief  可交互元素在特定状态下的行为接口
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.01.20
 *********************************************************************/

#pragma once

#ifndef UI_STATE_H
#define UI_STATE_H

#include <memory>

namespace engine::core {
    class Context;
}

namespace engine::ui {
    class UIInteractive;
}

namespace engine::ui::state {

/**
* @brief 可交互UI元素在特定状态下的行为接口。
*
* 该接口定义了所有具体UI状态必须实现的通用操作，
* 例如更新状态逻辑以及确定视觉表现。
* @note 输入不再由状态机轮询，而是通过 InputManager 的信号回调驱动（onMousePressed/onMouseReleased）。
*/
class UIState {
    friend class engine::ui::UIInteractive;
public:
    /**
	* @brief 构造函数传入父节点指针
	*/
    UIState(engine::ui::UIInteractive* owner);
    virtual ~UIState() = default;

    // 删除拷贝和移动构造函数/赋值运算符
    UIState(const UIState&) = delete;
    UIState& operator=(const UIState&) = delete;
    UIState(UIState&&) = delete;
    UIState& operator=(UIState&&) = delete;

protected:
    // --- 核心方法 ---
    virtual void enter() = 0;                               ///< @brief 进入状态时调用（纯虚，必须实现）
    virtual void update(float deltaTime, engine::core::Context& context);   ///< @brief 每帧更新状态逻辑
protected:
	engine::ui::UIInteractive* mOwner = nullptr;            ///< @brief 指向父节点
};

} // namespace engine::ui::state
#endif // !UI_STATE_H
