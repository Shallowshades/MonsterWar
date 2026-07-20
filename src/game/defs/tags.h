/*****************************************************************//**
 * @file   tags.h
 * @brief  游戏层标签组件定义
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#pragma once
#ifndef GAME_TAGS_H
#define GAME_TAGS_H

/* 在entt中，空的结构体可以作为"标签组件"使用 */
/* 它不占用内存空间，当需要给实体添加标签时，这是最推荐的做法 */
namespace game::defs {

    struct DeadTag {};              ///< @brief 死亡标签，用于标记实体死亡并延时删除

}   // namespace game::defs

#endif // GAME_TAGS_H
