/*****************************************************************//**
 * @file   place_unit_system.h
 * @brief  放置单位系统
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.19
 *********************************************************************/

#pragma once
#ifndef PLACE_UNIT_SYSTEM_H
#define PLACE_UNIT_SYSTEM_H

#include "../defs/events.h"
#include "../defs/constants.h"
#include <entt/entity/fwd.hpp>
#include <entt/signal/fwd.hpp>
#include <glm/vec2.hpp>

namespace engine::core {
    class Context;
}

namespace game::factory {
    class EntityFactory;
}

namespace game::system {

    /**
     * @brief 放置单位系统
     * @note 处理玩家角色的“准备单位”与“放置地图单位”操作。
     *       - 注意不要混淆“准备单位”（跟随鼠标的待放置单位）与“地图单位”（实际放置在地图上的战斗单位）
     */
    class PlaceUnitSystem {
    private:
        entt::registry& mRegistry;
        game::factory::EntityFactory& mEntityFactory;
        engine::core::Context& mContext;

        /// @brief 目标放置位置实体（地图上的近战/远程放置点），玩家角色只能放置在这些位置上
        entt::entity mTargetPlaceEntity{ entt::null };

    public:
        PlaceUnitSystem(entt::registry& registry,
            game::factory::EntityFactory& entity_factory,
            engine::core::Context& context);
        ~PlaceUnitSystem();

        void update(float delta_time);

    private:
        /**
         * @brief 检查位置是否为有效放置点
         * @note 如果有效，则 mTargetPlaceEntity 指向该实体，否则为 null
         * @param position 要检查的位置
         * @param player_type 单位类型
         */
        void checkTargetPlace(const glm::vec2& position, game::defs::PlayerType player_type);

        // 事件回调函数
        void onPrepUnitEvent(const game::defs::PrepUnitEvent& event);           ///< @brief 准备单位事件
        void onRemoveUnitEvent(const game::defs::RemovePlayerUnitEvent& event); ///< @brief 移除(地图上)玩家单位事件
        void onCancelPlacementEvent(const game::defs::CancelPlacementEvent& event); ///< @brief 取消放置事件

        // 输入控制回调函数
        bool onPlaceUnit();         ///< @brief 放置单位（鼠标左键点击）
        bool onCancelPrepUnit();    ///< @brief 取消放置准备（鼠标右键点击）
    };

}   // namespace game::system

#endif // PLACE_UNIT_SYSTEM_H
