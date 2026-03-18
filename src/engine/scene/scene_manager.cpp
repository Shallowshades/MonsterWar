#include "scene_manager.h"
#include "scene.h"
#include "../core/context.h"
#include <spdlog/spdlog.h>
#include <entt/signal/dispatcher.hpp>

namespace engine::scene {
engine::scene::SceneManager::SceneManager(engine::core::Context& context) 
	: mContext(context)
{
	mContext.getDispatcher().sink<engine::utils::PopSceneEvent>().connect<&SceneManager::onPopScene>(this);
	mContext.getDispatcher().sink<engine::utils::PushSceneEvent>().connect<&SceneManager::onPushScene>(this);
	mContext.getDispatcher().sink<engine::utils::ReplaceSceneEvent>().connect<&SceneManager::onReplaceScene>(this);
	spdlog::trace("{} 构造完成", mLogTag.data());
}

SceneManager::~SceneManager() {
	clean();
	spdlog::trace("{} 析构完成", mLogTag.data());
}

Scene* SceneManager::getCurrentScene() const {
	if (mSceneStack.empty()) {
		return nullptr;
	}
	return mSceneStack.back().get();
}

engine::core::Context& SceneManager::getContext() const {
	return mContext;
}

void SceneManager::update(float deltaTime) {
	// 只更新栈顶元素
	Scene* currentScene = getCurrentScene();
	if (currentScene) {
		currentScene->update(deltaTime);
	}
	// 执行可能的切换场景操作
	processPendingActions();
}

void SceneManager::render() {
	// 渲染时需要叠加渲染所有场景, 而不只是栈顶
	for (const auto& scene : mSceneStack) {
		if (scene) {
			scene->render();
		}
	}
}

void SceneManager::handleInput() {
	Scene* currentScene = getCurrentScene();
	if (currentScene) {
		currentScene->handleInput();
	}
}

void SceneManager::clean() {
	spdlog::trace("{} 正在清理场景管理器并清空场景栈...", mLogTag.data());
	while (!mSceneStack.empty()) {
		if (mSceneStack.back()) {
			spdlog::debug("{} 正在清理场景 '{}'", mLogTag.data(), mSceneStack.back()->getName());
			mSceneStack.back()->clean();
		}
		mSceneStack.pop_back();
	}

	// 断开事件处理函数 (一次断开所有和当前实例绑定的回调函数)
	mContext.getDispatcher().disconnect(this);
}

void SceneManager::onPopScene() {
	mPendingAction = PendingAction::Pop;
}

void SceneManager::onPushScene(engine::utils::PushSceneEvent& event) {
	mPendingAction = PendingAction::Push;
	mPendingScene = std::move(event.scene);
}

void SceneManager::onReplaceScene(engine::utils::ReplaceSceneEvent& event) {
	mPendingAction = PendingAction::Replace;
	mPendingScene = std::move(event.scene);
}

void SceneManager::processPendingActions() {
	switch (mPendingAction) {
	case PendingAction::None:
		return;
	case PendingAction::Push:
		pushScene(std::move(mPendingScene));
		break;
	case PendingAction::Pop:
		popScene();
		break;
	case PendingAction::Replace:
		replaceScene(std::move(mPendingScene));
		break;
	default:
		break;
	}
	mPendingAction = PendingAction::None;
}

void SceneManager::pushScene(std::unique_ptr<Scene>&& scene) {
	if (!scene) {
		spdlog::warn("{} 尝试将空场景压入栈中", mLogTag.data());
		return;
	}

	spdlog::debug("{} 正在将场景 '{}' 压入栈中", mLogTag.data(), scene->getName());

	// 初始化新场景
	if (!scene->getIsInitialized()) {
		scene->init();
	}

	// 将新场景移入栈顶
	mSceneStack.push_back(std::move(scene));
}

void SceneManager::popScene() {
	if (mSceneStack.empty()) {
		spdlog::warn("{} 尝试从空场景栈中弹出");
		return;
	}
	spdlog::debug("{} 正在从栈中弹出场景 '{}'", mLogTag.data(), mSceneStack.back()->getName());

	// 清除并移除栈顶场景
	if (mSceneStack.back()) {
		mSceneStack.back()->clean();
	}
	mSceneStack.pop_back();

	if (mSceneStack.empty()) {
		spdlog::warn("{} : 弹出最后一个场景, 退出游戏", mLogTag.data());
		mContext.getDispatcher().enqueue<engine::utils::QuitEvent>();
	}
}

void SceneManager::replaceScene(std::unique_ptr<Scene>&& scene) {
	if (!scene) {
		spdlog::warn("{} 尝试用空场景替换", mLogTag.data());
		return;
	}

	spdlog::debug("{} 正在用场景 '{}' 替换场景 '{}'", mLogTag.data(), scene->getName(), mSceneStack.back()->getName());
	while (!mSceneStack.empty()) {
		if (mSceneStack.back()) {
			mSceneStack.back()->clean();
		}
		mSceneStack.pop_back();
	}

	// 初始化新场景
	if (!scene->getIsInitialized()) {
		scene->init();
	}

	// 将新场景移入栈顶
	mSceneStack.push_back(std::move(scene));
}
}
