/*****************************************************************//**
 * @file   audio_component.h
 * @brief  音频组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.07.14
 *********************************************************************/

#pragma once

#ifndef AUDIO_COMPONENT_H
#define AUDIO_COMPONENT_H

#include <entt/entity/fwd.hpp>
#include <unordered_map>

namespace engine::component {
    /**
     * @brief 音频组件，包含音效集合。
     */
    struct AudioComponent {
        std::unordered_map<entt::id_type, entt::id_type> mSounds;   ///< @brief 音效集合，名称(哈希) -> 音效ID
    };
}

#endif // AUDIO_COMPONENT_H
