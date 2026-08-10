/*****************************************************************//**
 * @file   session_data.h
 * @brief  会话数据，用于场景间传递关卡数据
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.10
 *********************************************************************/

#pragma once
#ifndef SESSION_DATA_H
#define SESSION_DATA_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <entt/entity/entity.hpp>

namespace game::data {

    /// @brief 角色数据，包含角色名称、职业、等级、稀有度
    struct UnitData {
        entt::id_type mNameId{ entt::null };   ///< @brief 角色名哈希
        entt::id_type mClassId{ entt::null };  ///< @brief 职业名哈希
        std::string mName;                     ///< @brief 角色名
        std::string mClass;                    ///< @brief 职业名
        int mLevel{ 1 };                       ///< @brief 等级
        int mRarity{ 1 };                      ///< @brief 稀有度
    };

    /**
     * @brief 会话数据，场景间（例如通关时）传递的关卡数据。
     * @note 包含当前关卡、积分、是否通关以及玩家角色池等进度信息。
     *       数据实例很可能同时被多个场景使用，因此通常由 shared_ptr 持有。
     */
    class SessionData {
    private:
        int mLevelNumber{ 1 };      ///< @brief 当前关卡
        int mPoint{ 0 };            ///< @brief 积分
        bool mLevelClear{ false };  ///< @brief 是否通关
        std::unordered_map<entt::id_type, UnitData> mUnitMap;   ///< @brief 玩家角色池（角色名哈希 : 角色数据）

    public:
        SessionData() = default;
        ~SessionData() = default;

        bool loadDefaultData(std::string_view path = "assets/data/default_session_data.json");  ///< @brief 从默认数据文件加载，成功返回true
        bool saveToFile(std::string_view path);                                                 ///< @brief 保存到存档文件，成功返回true

        void addUnit(std::string_view name, std::string_view class_str, int level, int rarity); ///< @brief 添加角色
        void removeUnit(entt::id_type name_id);                                                 ///< @brief 删除角色
        void addUnitLevel(entt::id_type name_id, int add_level = 1);                            ///< @brief 增加角色等级
        void addUnitRarity(entt::id_type name_id, int add_rarity = 1);                          ///< @brief 增加角色稀有度
        void clearUnits();                                                                      ///< @brief 清空角色列表
        void clear();                                                                           ///< @brief 清空所有数据

        void addPoint(int add_point) { mPoint += add_point; }                                   ///< @brief 增加积分
        int addOneLevel() { return ++mLevelNumber; }                                            ///< @brief 进入下一关，返回新的关卡号
        void setLevelClear(bool clear) { mLevelClear = clear; }                                 ///< @brief 设置是否通关

        // --- getters ---
        [[nodiscard]] std::unordered_map<entt::id_type, UnitData>& getUnitMap() { return mUnitMap; }
        [[nodiscard]] int getLevelNumber() const { return mLevelNumber; }
        [[nodiscard]] int getPoint() const { return mPoint; }
        [[nodiscard]] bool isLevelClear() const { return mLevelClear; }
    };

}   // namespace game::data

#endif // SESSION_DATA_H
