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
class GameScene : public engine::scene::Scene {
public:
	GameScene(engine::core::Context& context, engine::scene::SceneManager& sceneManager);
	~GameScene();
};
} // namespace game::scene
