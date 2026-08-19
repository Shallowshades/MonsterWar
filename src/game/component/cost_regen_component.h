/*****************************************************************//**
 * @file   cost_regen_component.h
 * @brief  费用回复组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.19
 *********************************************************************/

#pragma once
#ifndef COST_REGEN_COMPONENT_H
#define COST_REGEN_COMPONENT_H

namespace game::component {

/**
 * @brief 费用回复组件
 *
 * 拥有该组件的实体每秒额外回复指定数量的cost（如基地建筑）。
 */
struct CostRegenComponent {
    float mRate{ 0.0f };        ///< @brief 每秒回复的cost量
};

}   // namespace game::component

#endif // COST_REGEN_COMPONENT_H
