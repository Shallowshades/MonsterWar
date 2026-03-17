#include "game_scene.h"

namespace game::scene {
GameScene::GameScene(engine::core::Context& context, engine::scene::SceneManager& sceneManager) 
	: Scene("GameScene", context, sceneManager) 
{

}

GameScene::~GameScene(){

}
} // namespace game::scene
