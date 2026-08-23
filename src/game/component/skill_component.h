/*****************************************************************//**
 * @file   skill_component.h
 * @brief  技能组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.23
 *********************************************************************/

#pragma once
#ifndef SKILL_COMPONENT_H
#define SKILL_COMPONENT_H

#include <entt/entity/entity.hpp>
#include <string>

namespace game::component {

/**
 * @brief 技能组件
 * @note 用于存储技能信息，包括技能ID、用于显示特效的实体ID、名称、描述、冷却时间、计时器等。
 */
struct SkillComponent {
    entt::id_type mSkillId{ entt::null };       ///< @brief 技能ID
    entt::entity mDisplayEntity{ entt::null };  ///< @brief 用于显示特效的实体ID
    std::string mName;                          ///< @brief 技能名称
    std::string mDescription;                   ///< @brief 技能描述
    float mCooldown{ 0.0f };                    ///< @brief 技能冷却时间
    float mDuration{ 0.0f };                    ///< @brief 技能持续时间
    float mCooldownTimer{ 0.0f };               ///< @brief 技能冷却计时器
    float mDurationTimer{ 0.0f };               ///< @brief 技能持续计时器
};

}   // namespace game::component

#endif // SKILL_COMPONENT_H
