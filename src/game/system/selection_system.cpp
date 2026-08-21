/*****************************************************************//**
 * @file   selection_system.cpp
 * @brief  选择单位系统实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.21
 *********************************************************************/

#include "selection_system.h"
#include "../component/player_component.h"
#include "../component/enemy_component.h"
#include "../defs/constants.h"
#include "../defs/tags.h"
#include "../../engine/core/context.h"
#include "../../engine/input/input_manager.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/utils/math.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/sigh.hpp>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace game::system {

    SelectionSystem::SelectionSystem(entt::registry& registry, engine::core::Context& context)
        : mRegistry(registry), mContext(context) {
        mContext.getInputManager().onAction("mouse_left"_hs).connect<&SelectionSystem::onMouseLeftClick>(this);
        mContext.getInputManager().onAction("mouse_right"_hs).connect<&SelectionSystem::onMouseRightClick>(this);
    }

    SelectionSystem::~SelectionSystem() {
        mContext.getInputManager().onAction("mouse_left"_hs).disconnect<&SelectionSystem::onMouseLeftClick>(this);
        mContext.getInputManager().onAction("mouse_right"_hs).disconnect<&SelectionSystem::onMouseRightClick>(this);
    }

    void SelectionSystem::update() {
        auto mouse_pos = mContext.getInputManager().getLogicalMousePosition();
        // 优先判断玩家单位
        auto view_player = mRegistry.view<engine::component::TransformComponent, game::component::PlayerComponent>();
        for (auto entity : view_player) {
            auto& transform = view_player.get<engine::component::TransformComponent>(entity);
            // 判断是否在鼠标悬浮检测范围内
            if (engine::utils::distanceSquared(transform.mPosition, mouse_pos) <= game::defs::HOVER_RADIUS * game::defs::HOVER_RADIUS) {
                mRegistry.ctx().get<entt::entity&>("hovered_unit"_hs) = entity;
                return;   // 找到悬浮单位，直接返回
            }
        }
        // 如果玩家单位没有被选中，再判断敌方单位
        auto view_enemy = mRegistry.view<engine::component::TransformComponent, game::component::EnemyComponent>();
        for (auto entity : view_enemy) {
            auto& transform = view_enemy.get<engine::component::TransformComponent>(entity);
            if (engine::utils::distanceSquared(transform.mPosition, mouse_pos) <= game::defs::HOVER_RADIUS * game::defs::HOVER_RADIUS) {
                mRegistry.ctx().get<entt::entity&>("hovered_unit"_hs) = entity;
                return;
            }
        }
        // 如果都没有被悬浮，则不悬浮任何单位
        mRegistry.ctx().get<entt::entity&>("hovered_unit"_hs) = entt::null;
    }

    void SelectionSystem::clearCurrentSelection() {
        auto current_selected_unit = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
        // 移除之前选中的单位，并移除范围显示标签
        if (current_selected_unit != entt::null && mRegistry.valid(current_selected_unit)) {
            mRegistry.remove<game::defs::ShowRangeTag>(current_selected_unit);
        }
        mRegistry.ctx().get<entt::entity&>("selected_unit"_hs) = entt::null;
    }

    // --- 输入控制回调函数 ---
    bool SelectionSystem::onMouseLeftClick() {
        auto hovered_unit = mRegistry.ctx().get<entt::entity&>("hovered_unit"_hs);
        if (hovered_unit == entt::null || !mRegistry.valid(hovered_unit)) return false;
        // 如果鼠标悬浮单位是玩家，则选中单位，并清除之前选中的单位
        if (auto player = mRegistry.try_get<game::component::PlayerComponent>(hovered_unit); player) {
            clearCurrentSelection();
            mRegistry.ctx().get<entt::entity&>("selected_unit"_hs) = hovered_unit;
            // 添加范围显示标签
            mRegistry.emplace_or_replace<game::defs::ShowRangeTag>(hovered_unit);
            return true;
        }
        return false;
    }

    bool SelectionSystem::onMouseRightClick() {
        clearCurrentSelection();
        return false;   // 让鼠标右键可以穿透
    }

}   // namespace game::system
