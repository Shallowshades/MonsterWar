/*****************************************************************//**
 * @file   scene.h
 * @brief  场景基类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.11.04
 *********************************************************************/

#pragma once
#ifndef SCENE_H
#define SCENE_H
#include <vector>
#include <memory>
#include <string>
#include <string_view>

namespace engine::core { class Context; }
namespace engine::ui { class UIManager; }
namespace engine::object { class GameObject; }
namespace engine::scene { class SceneManager; }

namespace engine::scene {

/**
 * @brief 场景基类, 负责管理场景中的游戏对象和场景生命周期.
 * 
 * 包含一组游戏对象, 并提供更新, 渲染, 处理输入和清理的接口
 * 派生类应实现具体的场景逻辑
 */
class Scene {
public:
	/**
	 * @brief 构造函数.
	 * 
	 * @param context 场景上下文
	 * @param sceneManager 场景管理器
	 */
	Scene(std::string_view name, engine::core::Context& context, engine::scene::SceneManager& sceneManager);
	virtual ~Scene();

	// 禁用拷贝和移动语义
	Scene(const Scene&) = delete;							///< @brief 删除拷贝构造
	Scene& operator=(const Scene&) = delete;				///< @brief 删除拷贝赋值构造
	Scene(Scene&&) = delete;								///< @brief 删除移动构造
	Scene& operator=(Scene&&) = delete;						///< @brief 删除移动赋值构造

	// 核心循环函数
	virtual void init();									///< @brief 初始化场景
	virtual void update(float deltaTime);					///< @brief 更新场景
	virtual void render();									///< @brief 渲染场景
	virtual void handleInput();								///< @brief 处理输入
	virtual void clean();									///< @brief 清理场景

	/**
	 * @brief 直接向场景中添加一个游戏对象.
	 * 初始化时可用, 游戏进行中不安全; 右值引用&&传参与std::move配合, 避免拷贝
	 */
	virtual void addGameObject(std::unique_ptr<engine::object::GameObject>&& gameObject);
	
	/**
	 * @brief 安全地添加游戏对象. (添加到pendingAdditions中).
	 */
	virtual void safeAddGameObject(std::unique_ptr<engine::object::GameObject>&& gameObject);
	
	/**
	 * @brief 直接从场景中移除一个游戏对象. (一般不使用, 但保留实现逻辑)
	 */
	virtual void removeGameObject(engine::object::GameObject* gameObjectPtr);
	
	/**
	 * @brief 安全地移除游戏对象. 设置needRemove标记
	 */
	virtual void safeRemoveGameObject(engine::object::GameObject* gameObjectPtr);
	
	/**
	 * @brief 获取场景中游戏对象的容器.
	 */
	const std::vector<std::unique_ptr<engine::object::GameObject>>& getGameObjects() const;
	
	/**
	 * @brief 根据名称查找游戏对象(返回找到的第一个对象).
	 */
	engine::object::GameObject* findGameObjectByName(std::string_view name) const;

	void setName(std::string_view name);												///< @brief 设置场景名称
	std::string_view getName() const;													///< @brief 获取场景名称
	void setIsInitialized(bool initialized);											///< @brief 设置场景是否已初始化
	bool getIsInitialized() const;														///< @brief 获取场景是否已初始化

	engine::core::Context& getContext() const;											///< @brief 获取上下文引用
	engine::scene::SceneManager& getSceneManager() const;								///< @brief 获取场景管理器
	std::vector<std::unique_ptr<engine::object::GameObject>>& getGameObjects();			///< @brief 获取场景中的游戏对象

protected:
	void processPendingAdditions();														///< @brief 处理待添加的游戏对象

protected:
	constexpr static std::string_view mLogTag = "Scene";								///< @brief 日志标识

	std::string mSceneName;																///< @brief 场景名称
	engine::core::Context& mContext;													///< @brief 上下文引用
	engine::scene::SceneManager& mSceneManager;											///< @brief 场景管理器引用
	std::unique_ptr<engine::ui::UIManager> mUIManager;									///< @brief UI管理器(初始化时自动创建)
	bool mIsInitialized;																///< @brief 场景是否已被初始化
	std::vector<std::unique_ptr<engine::object::GameObject>> mGameObjects;				///< @brief 场景中的游戏对象
	std::vector<std::unique_ptr<engine::object::GameObject>> mPendingAdditions;			///< @brief 待添加的游戏对象
};
} // namespace engine::scene

#endif // SCENE_H