/*****************************************************************//**
 * @file   animation_component.h
 * @brief  名称组件
 * @version 1.0
 *
 * @author ShallowShades
 * @date   2026.07.14
 *********************************************************************/

#pragma once

#include <string>
#include <entt/entity/entity.hpp>

namespace engine::component {
    /**
     * @brief 名称组件，可用于标记实体名称。
     */
    struct NameComponent {
        entt::id_type mNameId{ entt::null };    ///< @brief 名称ID
        std::string mName;                      ///< @brief 名称
    };
}