/*****************************************************************//**
 * @file   mobile_action_bar.h
 * @brief  移动端操作栏
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.09.01
 *********************************************************************/

#pragma once
#ifndef MOBILE_ACTION_BAR_H
#define MOBILE_ACTION_BAR_H

#include <entt/entity/fwd.hpp>

namespace engine::core {
    class Context;
}

namespace engine::ui {
    class UIPanel;
    class UIManager;
    class UIButton;
    class UILabel;
}

namespace game::ui {

/**
 * @brief 移动端操作栏
 *
 * 选中玩家单位后显示升级/撤退/技能/取消按钮，供 Android 触屏操作。
 * 只负责表达 UI 意图，具体逻辑仍通过既有事件（UpgradeUnitEvent/RetreatEvent/SkillActiveEvent）触发。
 */
class MobileActionBar {
public:
    MobileActionBar(entt::registry& registry,
        engine::ui::UIManager& ui_manager,
        engine::core::Context& context);
    ~MobileActionBar();

    void update(float delta_time);

    engine::ui::UIPanel* getAnchorPanel() const { return mAnchorPanel; }

private:
    void createUI();
    void refreshVisibility();

    entt::registry& mRegistry;
    engine::ui::UIManager& mUIManager;
    engine::core::Context& mContext;

    engine::ui::UIPanel* mAnchorPanel{ nullptr };
    engine::ui::UIButton* mUpgradeButton{ nullptr };
    engine::ui::UIButton* mRetreatButton{ nullptr };
    engine::ui::UIButton* mSkillButton{ nullptr };
    engine::ui::UILabel* mUpgradeLabel{ nullptr };
    engine::ui::UILabel* mRetreatLabel{ nullptr };
    engine::ui::UILabel* mSkillLabel{ nullptr };
    engine::ui::UIButton* mCancelPlaceButton{ nullptr };
    engine::ui::UIButton* mPauseButton{ nullptr };
    engine::ui::UIButton* mSpeedButton{ nullptr };
    engine::ui::UILabel* mPauseLabel{ nullptr };
    engine::ui::UILabel* mSpeedLabel{ nullptr };
};

}   // namespace game::ui

#endif // MOBILE_ACTION_BAR_H
