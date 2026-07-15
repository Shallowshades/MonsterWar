#include "ysort_system.h"
#include "../component/render_component.h"
#include "../component/transform_component.h"
#include <entt/entity/registry.hpp>

namespace engine::system {

	void YSortSystem::update(entt::registry& registry) {
		// 让RenderComponent的深度depth等于TransformComponent的y坐标

		// 执行渲染，注意排序组件RenderComponent必须放在最前面
		// EnTT 的 view 默认会选择元素最少的 storage 驱动迭代。
		// 显式指定使用 RenderComponent，才能保证遍历顺序与上面的排序一致。
		auto view = registry.view<component::RenderComponent, const component::TransformComponent>();
		view.use<component::RenderComponent>();
		for (auto entity : view) {
			auto& render = view.get<component::RenderComponent>(entity);
			const auto& transform = view.get<const component::TransformComponent>(entity);
			render.mDepth = transform.mPosition.y;
		}

		// 对 RenderComponent storage 排序，比较规则由 RenderComponent::operator< 定义。
		registry.sort<component::RenderComponent>([](const auto& lhs, const auto& rhs) {
			return lhs < rhs;
		});
	}

}