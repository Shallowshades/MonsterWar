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
	GameScene(engine::core::Context& context);
	~GameScene();

	void init() override;
	void clean() override;

private:
	// 测试输入回调事件
	int32_t mSceneNum{ 0 };
	void onReplace();
	void onPush();
	void onPop();
	void onQuit();
};
} // namespace game::scene
