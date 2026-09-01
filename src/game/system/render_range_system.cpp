/*****************************************************************//**
 * @file   render_range_system.cpp
 * @brief  渲染范围系统实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.19
 *********************************************************************/

#include "render_range_system.h"
#include "../component/unit_prep_component.h"
#include "../component/stats_component.h"
#include "../defs/tags.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/render/renderer.h"
#include "../../engine/render/camera.h"
#include <entt/entity/registry.hpp>

namespace game::system {

    void RenderRangeSystem::update(entt::registry& registry, engine::render::Renderer& renderer, const engine::render::Camera& camera) {
        // 准备放置类型的单位（跟随鼠标的待放置单位）
        auto view_prep = registry.view<game::defs::ShowRangeTag, engine::component::TransformComponent, game::component::UnitPrepComponent>();
        for (auto entity : view_prep) {
            auto& transform = view_prep.get<engine::component::TransformComponent>(entity);
            auto& prep = view_prep.get<game::component::UnitPrepComponent>(entity);
            // 圈半径：远程 = 攻击范围；近战 = 自身半径 + 攻击范围（保证近战也有可见的放置选区）
            float radius = prep.mRange;
            if (prep.mType == game::defs::PlayerType::MELEE) {
                radius = game::defs::UNIT_RADIUS + prep.mRange;
            }
            // 攻击范围显示为透明绿色圆形
            renderer.drawFilledCircle(camera, transform.mPosition, radius, game::defs::RANGE_COLOR);
        }
        // 地图上的单位（已放置的远程单位显示攻击范围圆）
        auto view_remote = registry.view<game::defs::ShowRangeTag, engine::component::TransformComponent, game::component::StatsComponent>();
        for (auto entity : view_remote) {
            auto& transform = view_remote.get<engine::component::TransformComponent>(entity);
            auto& stats = view_remote.get<game::component::StatsComponent>(entity);
            // 攻击范围显示为透明绿色圆形
            renderer.drawFilledCircle(camera, transform.mPosition, stats.mRange, game::defs::RANGE_COLOR);
        }
    }

}   // namespace game::system
