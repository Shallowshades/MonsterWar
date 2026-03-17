#include "scene.h"
#include "scene_manager.h"
#include "../object/game_object.h"
#include "../core/context.h"
#include "../core/game_state.h"
#include "../physics/physics_engine.h"
#include "../render/camera.h"
#include "../ui/ui_manager.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace engine::scene {
engine::scene::Scene::Scene(std::string_view name, engine::core::Context& context, engine::scene::SceneManager& sceneManager)
	: mSceneName(name)
	, mContext(context)
	, mSceneManager(sceneManager)
	, mUIManager(std::make_unique<engine::ui::UIManager>())
	, mIsInitialized(false)
{
	spdlog::trace("{} : {} 构造完成", mLogTag.data(), mSceneName);
}
Scene::~Scene() = default;

void Scene::init() {
	mIsInitialized = true;
	spdlog::trace("{} : {} 初始化完成", mLogTag.data(), mSceneName);
}

void Scene::update(float deltaTime) {
	if (!mIsInitialized) {
		return;
	}

	// 优先移除上一帧已经标记删除的对象, 避免物理更新产生的碰撞事件持有悬空指针
	bool needRemove = false;
	for (auto& obj : mGameObjects) {
		if (!obj) {
			needRemove = true;
			spdlog::warn("{} : exist a null game object pointer, will be clean in one frame", mLogTag.data());
			continue;
		}
		if (obj->isNeedRemove()) {
			needRemove = true;
			obj->clean();
		}
	}

	// C++20, 比erase-remove_if简洁
	// 如此写需优先调用clean
	if (needRemove) {
		std::erase_if(mGameObjects, [](const std::unique_ptr<engine::object::GameObject>& obj) {
			return !obj || obj->isNeedRemove();
		});
	}

	// 只有在游戏中才更新物理引擎和相机
	if (mContext.getGameState().isPlaying()) {
		// 先更新物理引擎
		mContext.getPhysicsEngine().update(deltaTime);
		// 更新相机
		mContext.getCamera().update(deltaTime);
	}

	for (auto& obj : mGameObjects) {
		// 安全更新游戏对象
		if (obj && !obj->isNeedRemove()) {
			obj->update(deltaTime, mContext);
		}
		else if (!obj){
			spdlog::warn("{} : try to update a null game object pointer", mLogTag.data());
		}
	}

	// 更新UI管理器
	mUIManager->update(deltaTime, mContext);

	processPendingAdditions();
}

void Scene::render() {
	if (!mIsInitialized) {
		return;
	}

	// 渲染所有游戏对象
	for (const auto& obj : mGameObjects) {
		if (obj) {
			obj->render(mContext);
		}
	}

	mUIManager->render(mContext);
}

void Scene::handleInput() {
	if (!mIsInitialized) {
		return;
	}

	// 处理UI管理器的输入
	// 如果输入事件被UI处理则返回, 不再处理游戏对象输入
	if (mUIManager->handleInput(mContext)) return;

	// 遍历所有游戏对象, 删除等到update
	for (auto& obj : mGameObjects) {
		if (obj && !obj->isNeedRemove()) {
			obj->handleInput(mContext);
		}
	}
}

void Scene::clean() {
	if (!mIsInitialized) {
		return;
	}

	for (const auto& obj : mGameObjects) {
		if (obj) {
			obj->clean();
		}
	}
	mGameObjects.clear();

	mIsInitialized = false;
	spdlog::trace("{} : {} 清理完成.", mLogTag.data(), mSceneName);
}

void Scene::addGameObject(std::unique_ptr<engine::object::GameObject>&& gameObject) {
	if (gameObject) {
		mGameObjects.push_back(std::move(gameObject));
	}
	else {
		spdlog::warn("{} : {} 尝试添加空对象", mLogTag.data(), mSceneName);
	}
}

void Scene::safeAddGameObject(std::unique_ptr<engine::object::GameObject>&& gameObject) {
	if (gameObject) {
		mPendingAdditions.push_back(std::move(gameObject));
	}
	else {
		spdlog::warn("{} : {} 尝试添加空对象", mLogTag.data(), mSceneName);
	}
}

void Scene::removeGameObject(engine::object::GameObject* gameObjectPtr) {
	if (!gameObjectPtr) {
		spdlog::warn("{} : {} 尝试移除空对象", mLogTag.data(), mSceneName);
		return;
	}

	// erase-remove 移除法不可用, 因为智能指针与裸指针无法比较
	// 需要使用std::remove_if和lambda表达式自定义比较的方式
	auto iter = std::remove_if(mGameObjects.begin(), mGameObjects.end(), [gameObjectPtr](const std::unique_ptr<engine::object::GameObject>& p) {
		return p.get() == gameObjectPtr;
		});
	if (iter != mGameObjects.end()) {
		(*iter)->clean();
		mGameObjects.erase(iter, mGameObjects.end());
		spdlog::trace("{} : {} 移除游戏对象.", mLogTag.data(), mSceneName);
	}
	else {
		spdlog::warn("{} : {} 中不存在应删除的游戏对象", mLogTag.data(), mSceneName);
	}
}

void Scene::safeRemoveGameObject(engine::object::GameObject* gameObjectPtr) {
	gameObjectPtr->setNeedRemove(true);
}

const std::vector<std::unique_ptr<engine::object::GameObject>>& Scene::getGameObjects() const {
	return mGameObjects;
}

engine::object::GameObject* Scene::findGameObjectByName(std::string_view name) const {
	// 找到第一个符合条件的游戏对象就返回
	for (const auto& obj : mGameObjects) {
		if (obj && obj->getName() == name) {
			return obj.get();
		}
	}
	return nullptr;
}

void Scene::setName(std::string_view name) {
	mSceneName = name;
}

std::string_view Scene::getName() const {
	return mSceneName;
}

void Scene::setIsInitialized(bool initialized) {
	mIsInitialized = initialized;
}

bool Scene::getIsInitialized() const {
	return mIsInitialized;
}

engine::core::Context& Scene::getContext() const {
	return mContext;
}

engine::scene::SceneManager& Scene::getSceneManager() const {
	return mSceneManager;
}

std::vector<std::unique_ptr<engine::object::GameObject>>& Scene::getGameObjects() {
	return mGameObjects;
}

void Scene::processPendingAdditions() {
	// 处理待添加的游戏对象
	for (auto& gameObject : mPendingAdditions) {
		addGameObject(std::move(gameObject));
	}
	mPendingAdditions.clear();
}
}
