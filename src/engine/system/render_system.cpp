#include "render_system.h"
#include "../render/renderer.h"
#include "../render/camera.h"
#include "../component/render_component.h"
#include "../component/transform_component.h"
#include "../component/sprite_component.h"

namespace engine::system {

	void RenderSystem::update(entt::registry& registry, render::Renderer& renderer, const render::Camera& camera) {
		// 用 RenderComponent 驱动遍历，保证按 YSortSystem 排序后的 (layer, depth) 顺序渲染
		auto view = registry.view<component::RenderComponent, component::TransformComponent, component::SpriteComponent>();
		view.use<component::RenderComponent>();
		for (auto entity : view) {
			const auto& render = view.get<component::RenderComponent>(entity);
			const auto& transform = view.get<component::TransformComponent>(entity);
			const auto& sprite = view.get<component::SpriteComponent>(entity);
			auto position = transform.mPosition + sprite.mOffset;
			auto size = sprite.mSize * transform.mScale;
			renderer.drawSprite(camera, sprite.mSprite, position, size, transform.mRotation);
		}
	}

} // namespace engine::system
