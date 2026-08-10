/*****************************************************************//**
 * @file   ui_config.h
 * @brief  UI配置数据，管理icon/portrait/portrait_frame/unit_panel
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.10
 *********************************************************************/

#pragma once
#ifndef UI_CONFIG_H
#define UI_CONFIG_H

#include "../../engine/render/image.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <glm/vec2.hpp>
#include <entt/entity/fwd.hpp>
#include <nlohmann/json_fwd.hpp>

namespace game::data {

/**
 * @brief 管理UI配置数据。
 * @note 包含 icon（职业图标）、portrait（角色肖像）、portrait_frame（肖像框）、unit_panel（出击面板布局）的配置数据。
 */
class UIConfig {
private:
    std::unordered_map<entt::id_type, engine::render::Image> mIconMap;             ///< @brief 职业类型icon的map（职业名哈希 : 图片）
    std::unordered_map<entt::id_type, engine::render::Image> mPortraitMap;         ///< @brief 角色肖像的map（角色名哈希 : 图片）
    std::unordered_map<int, engine::render::Image> mPortraitFrameMap;              ///< @brief 肖像框的map（稀有度 : 图片）

    // --- 单位面板的布局配置（从json读取） ---
    float mUnitPanelPadding{ 10.0f };                   ///< @brief 单位面板间隔
    glm::vec2 mUnitPanelFrameSize{ 128.0f, 128.0f };    ///< @brief 单位面板大小
    int mUnitPanelFontSize{ 40 };                       ///< @brief 单位面板字体大小
    std::string mUnitPanelFontPath;                     ///< @brief 单位面板字体路径
    glm::vec2 mUnitPanelFontOffset{ 16.0f, 72.0f };     ///< @brief 单位面板字体偏移

public:
    UIConfig() = default;
    ~UIConfig();

    bool loadFromFile(std::string_view path = "assets/data/ui_config.json");   ///< @brief 从json配置文件加载数据，成功返回true

    // --- Getters ---
    [[nodiscard]] engine::render::Image& getIcon(entt::id_type id);
    [[nodiscard]] engine::render::Image& getPortrait(entt::id_type id);
    [[nodiscard]] engine::render::Image& getPortraitFrame(int rarity);
    [[nodiscard]] float getUnitPanelPadding() const { return mUnitPanelPadding; }
    [[nodiscard]] glm::vec2 getUnitPanelFrameSize() const { return mUnitPanelFrameSize; }
    [[nodiscard]] int getUnitPanelFontSize() const { return mUnitPanelFontSize; }
    [[nodiscard]] std::string getUnitPanelFontPath() const { return mUnitPanelFontPath; }
    [[nodiscard]] glm::vec2 getUnitPanelFontOffset() const { return mUnitPanelFontOffset; }

private:
    // --- 分步骤的数据加载函数 ---
    void loadIcon(nlohmann::json& json);
    void loadPortrait(nlohmann::json& json);
    void loadPortraitFrame(nlohmann::json& json);
    void loadLayout(nlohmann::json& json);
};

}   // namespace game::data

#endif // UI_CONFIG_H
