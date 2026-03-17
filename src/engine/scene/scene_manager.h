/*****************************************************************//**
 * @file   scene_manager.h
 * @brief  场景管理类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.11.04
 *********************************************************************/

#pragma once
#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <memory>
#include <string>
#include <vector>

namespace engine::core { class Context; }
namespace engine::scene { class Scene; }

namespace engine::scene {

/**
 * @brief 管理游戏中的场景栈, 处理场景切换和生命周期.
 */
class SceneManager final {
public:

	explicit SceneManager(engine::core::Context& context);				///< @brief 显示构造函数
	~SceneManager();													///< @brief 析构函数

	// 禁用拷贝和移动语义
	SceneManager(const SceneManager&) = delete;							///< @brief 删除拷贝构造
	SceneManager& operator=(const SceneManager&) = delete;				///< @brief 删除拷贝赋值构造
	SceneManager(SceneManager&&) = delete;								///< @brief 删除移动构造
	SceneManager& operator=(SceneManager&&) = delete;					///< @brief 删除移动赋值构造

	// 延时切换场景
	void requestPushScene(std::unique_ptr<Scene>&& scene);				///< @brief 请求压入一个新场景
	void requestPopScene();												///< @brief 请求弹出当前场景
	void requestReplaceScene(std::unique_ptr<Scene>&& scene);			///< @brief 请求替换当前场景

	Scene* getCurrentScene() const;										///< @brief 获取当前活动场景
	engine::core::Context& getContext() const;							///< @brief 获取引擎上下文引用

	// 核心循环函数
	void update(float deltaTime);										///< @brief 更新
	void render();														///< @brief 渲染
	void handleInput();													///< @brief 处理输入
	void clean();														///< @brief 清理

private:
	void processPendingActions();										///< @brief 处理挂起的场景操作
	void pushScene(std::unique_ptr<Scene>&& scene);						///< @brief 将一个新场景压入栈顶, 使其成为活动场景
	void popScene();													///< @brief 移除栈顶场景
	void replaceScene(std::unique_ptr<Scene>&& scene);					///< @brief 清理场景栈所有场景, 将此场景设为栈顶场景

private:
	constexpr static std::string_view mLogTag = "SceneManager";			///< @brief 日志标识

	engine::core::Context& mContext;									///< @brief 引擎上下文
	std::vector<std::unique_ptr<Scene>> mSceneStack;					///< @brief 场景栈
	enum class PendingAction {None, Push, Pop, Replace };				///< @brief 待处理的动作
	PendingAction mPendingAction = PendingAction::None;					///< @brief 待处理的动作
	std::unique_ptr<Scene> mPendingScene;								///< @brief 待处理的场景
};
} // namespace engine::scene

#endif // SCENE_MANAGER_H
