/*****************************************************************//**
 * @file   game_scene.h
 * @brief  
 * 
 * @author Shallowshades
 * @date   2026.03.17
 *********************************************************************/

#pragma once

#include "../../engine/scene/scene.h"

namespace game::scene {
/**
 * @brief 游戏场景.
 */
class GameScene final : public engine::scene::Scene {
public:
	GameScene(engine::core::Context& context, engine::scene::SceneManager& sceneManager);
	~GameScene();

	void init() override;
	void clean() override;

private:
	// 测试输入回调事件
	void onAttack();
	void onJump();
};
} // namespace game::scene
