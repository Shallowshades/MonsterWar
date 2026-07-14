#include "scene.h"
#include "scene_manager.h"
#include "../core/context.h"
#include "../ui/ui_manager.h"
#include "../utils/events.h"
#include <spdlog/spdlog.h>
#include <entt/signal/dispatcher.hpp>

namespace engine::scene {

	Scene::Scene(std::string_view name, engine::core::Context& context)
		: mSceneName(name),
		mContext(context),
		mUIManager(std::make_unique<engine::ui::UIManager>()),
		mIsInitialized(false) {
		spdlog::trace("场景 '{}' 构造完成。", mSceneName);
	}

	Scene::~Scene() = default;

	void Scene::init() {
		mIsInitialized = true;     // 子类应该最后调用父类的 init 方法
		spdlog::trace("场景 '{}' 初始化完成。", mSceneName);
	}

	void Scene::update(float delta_time) {
		if (!mIsInitialized) return;

		// 更新UI管理器
		mUIManager->update(delta_time, mContext);
	}

	void Scene::render() {
		if (!mIsInitialized) return;

		// 渲染UI管理器
		mUIManager->render(mContext);
	}

	void Scene::handleInput() {
		if (!mIsInitialized) return;

		// 处理UI管理器输入
		if (mUIManager->handleInput(mContext)) return;   // 如果输入事件被UI处理则返回，不再处理游戏对象输入
	}

	void Scene::clean() {
		if (!mIsInitialized) return;

		mRegistry.clear();
		mIsInitialized = false;        // 清理完成后，设置场景为未初始化
		spdlog::trace("场景 '{}' 清理完成。", mSceneName);
	}

	void Scene::requestPopScene()
	{
		mContext.getDispatcher().trigger<engine::utils::PopSceneEvent>();
	}

	void Scene::requestPushScene(std::unique_ptr<engine::scene::Scene>&& scene)
	{
		mContext.getDispatcher().trigger<engine::utils::PushSceneEvent>(engine::utils::PushSceneEvent{ std::move(scene) });
	}

	void Scene::requestReplaceScene(std::unique_ptr<engine::scene::Scene>&& scene)
	{
		mContext.getDispatcher().trigger<engine::utils::ReplaceSceneEvent>(engine::utils::ReplaceSceneEvent{ std::move(scene) });
	}

	void Scene::quit()
	{
		mContext.getDispatcher().trigger<engine::utils::QuitEvent>();
	}
}