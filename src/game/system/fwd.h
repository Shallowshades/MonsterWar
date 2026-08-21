/*****************************************************************//**
 * @file   fwd.h
 * @brief  游戏层系统前向声明
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#pragma once
#ifndef GAME_SYSTEM_FWD_H
#define GAME_SYSTEM_FWD_H

namespace game::system {

    class FollowPathSystem;
    class RemoveDeadSystem;
    class BlockSystem;
    class SetTargetSystem;
    class AttackStarterSystem;
    class TimerSystem;
    class OrientationSystem;
    class AnimationStateSystem;
    class AnimationEventSystem;
    class CombatResolveSystem;
    class ProjectileSystem;
    class EffectSystem;
    class HealthBarSystem;
    class GameRuleSystem;
    class PlaceUnitSystem;
    class RenderRangeSystem;
    class DebugUISystem;
    class SelectionSystem;

}   // namespace game::system

#endif // GAME_SYSTEM_FWD_H
