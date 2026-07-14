/*****************************************************************//**
  * @file   game_scene.h
  * @brief  游戏场景类
  * @version 1.0
  *
  * @author Shallowshades
  * @date   2026.07.14
  *********************************************************************/

#pragma once

#include "../../engine/scene/scene.h"
#include "../../engine/system/fwd.h"
#include <memory>

namespace game::scene {

	class GameScene final : public engine::scene::Scene {
	public:
		GameScene(engine::core::Context& context);
		~GameScene();

		void init() override;
		void update(float delta_time) override;
		void render() override;
		void clean() override;

	private:
		// --- 测试资源管理器 ---
		void testResourceManager();
		// --- 测试ECS ---
		void testECS();

	private:
		std::unique_ptr<engine::system::RenderSystem> mRenderSystem;
		std::unique_ptr<engine::system::MovementSystem> mMovementSystem;
		std::unique_ptr<engine::system::AnimationSystem> mAnimationSystem;
	};


} // game::scene
