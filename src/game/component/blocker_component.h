/*****************************************************************//**
 * @file   blocker_component.h
 * @brief  阻挡者组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.22
 *********************************************************************/

#pragma once
#ifndef BLOCKER_COMPONENT_H
#define BLOCKER_COMPONENT_H

namespace game::component {

    /// @brief 阻挡者组件，存储阻挡者最大阻挡数量和当前阻挡数量
    struct BlockerComponent {
        int mMaxCount{};
        int mCurrentCount{};
    };

}   // namespace game::component

#endif // BLOCKER_COMPONENT_H
