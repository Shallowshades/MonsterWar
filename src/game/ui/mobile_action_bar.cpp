/*****************************************************************//**
 * @file   mobile_action_bar.cpp
 * @brief  移动端操作栏实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.09.01
 *********************************************************************/

#include "mobile_action_bar.h"
#include "../data/ui_config.h"
#include "../data/session_data.h"
#include "../data/game_stats.h"
#include "../factory/blueprint_manager.h"
#include "../component/player_component.h"
#include "../component/skill_component.h"
#include "../defs/tags.h"
#include "../defs/events.h"
#include "../../engine/core/context.h"
#include "../../engine/core/game_state.h"
#include "../../engine/core/time.h"
#include "../../engine/input/input_manager.h"
#include "../../engine/ui/ui_panel.h"
#include "../../engine/ui/ui_button.h"
#include "../../engine/ui/ui_label.h"
#include "../../engine/ui/ui_manager.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>
#include <cmath>
#include <utility>
#include <functional>

using namespace entt::literals;

namespace game::ui {

    MobileActionBar::MobileActionBar(entt::registry& registry,
        engine::ui::UIManager& ui_manager,
        engine::core::Context& context)
        : mRegistry(registry), mUIManager(ui_manager), mContext(context) {
        createUI();
        spdlog::trace("MobileActionBar 构造完成。");
    }

    MobileActionBar::~MobileActionBar() = default;

    void MobileActionBar::update(float) {
        refreshVisibility();
    }

    void MobileActionBar::createUI() {
        if (!mUIManager.init(mContext.getGameState().getLogicalSize())) return;

        auto ui_config = mRegistry.ctx().get<std::shared_ptr<game::data::UIConfig>>();
        const auto window_size = mContext.getGameState().getLogicalSize();
        const auto frame_size = ui_config->getUnitPanelFrameSize();
        const float padding = 12.0f;
        const float button_size = 72.0f;

        // 操作栏放在肖像栏上方
        float panel_height = button_size + padding * 2.0f;
        float panel_width = 4.0f * (button_size + padding) + padding;
        glm::vec2 panel_pos{
            (window_size.x - panel_width) * 0.5f,
            window_size.y - frame_size.y - 2.0f * ui_config->getUnitPanelPadding() - panel_height - 8.0f
        };

        auto anchor = std::make_unique<engine::ui::UIPanel>(panel_pos, glm::vec2(panel_width, panel_height));
        anchor->setId("mobile_action_bar"_hs);
        anchor->setBackgroundColor(engine::utils::FColor(0.0f, 0.0f, 0.0f, 0.35f));

        // 使用通用肖像框作为按钮底图（移动端先保证可点，后续可换正式按钮资源）
        auto& btn_image = ui_config->getPortraitFrame(1);

        auto make_button = [&](std::string_view text, const glm::vec2& pos, std::function<void()> callback)
            -> std::pair<std::unique_ptr<engine::ui::UIButton>, engine::ui::UILabel*> {
            auto button = std::make_unique<engine::ui::UIButton>(mContext,
                btn_image, btn_image, btn_image,
                pos,
                glm::vec2(button_size, button_size),
                std::move(callback));
            button->setId(entt::hashed_string(text.data()));

            auto label = std::make_unique<engine::ui::UILabel>(mContext.getTextRenderer(),
                text,
                ui_config->getUnitPanelFontPath(),
                18,
                engine::utils::FColor::white(),
                glm::vec2(4.0f, button_size - 26.0f));
            auto* label_ptr = label.get();
            button->addChild(std::move(label));

            return { std::move(button), label_ptr };
        };

        float x = padding;
        const float step = button_size + padding;

        // 升级
        {
            auto [button, label] = make_button("升级", glm::vec2(x, padding), [this]() {
                auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
                if (entity == entt::null || !mRegistry.valid(entity)) return;
                auto player = mRegistry.try_get<game::component::PlayerComponent>(entity);
                if (!player) return;
                auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
                if (game_stats.mCost < player->mCost) return;
                mContext.getDispatcher().enqueue(game::defs::UpgradeUnitEvent{ entity, player->mCost });
                mContext.getInputManager().consumeNextClick();
            });
            mUpgradeButton = button.get();
            mUpgradeLabel = label;
            anchor->addChild(std::move(button));
        }
        x += step;

        // 撤退
        {
            auto [button, label] = make_button("撤退", glm::vec2(x, padding), [this]() {
                auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
                if (entity == entt::null || !mRegistry.valid(entity)) return;
                auto player = mRegistry.try_get<game::component::PlayerComponent>(entity);
                if (!player) return;
                auto return_cost = static_cast<int>(player->mCost * 0.5f);
                mContext.getDispatcher().enqueue(game::defs::RetreatEvent{ entity, return_cost });
                mContext.getInputManager().consumeNextClick();
            });
            mRetreatButton = button.get();
            mRetreatLabel = label;
            anchor->addChild(std::move(button));
        }
        x += step;

        // 技能
        {
            auto [button, label] = make_button("技能", glm::vec2(x, padding), [this]() {
                auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
                if (entity == entt::null || !mRegistry.valid(entity)) return;
                if (mRegistry.all_of<game::defs::SkillReadyTag>(entity)) {
                    mContext.getDispatcher().enqueue(game::defs::SkillActiveEvent{ entity });
                    mContext.getInputManager().consumeNextClick();
                }
            });
            mSkillButton = button.get();
            mSkillLabel = label;
            anchor->addChild(std::move(button));
        }
        x += step;

        // 取消
        anchor->addChild(make_button("取消", glm::vec2(x, padding), [this]() {
            auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
            if (entity != entt::null && mRegistry.valid(entity)) {
                mRegistry.remove<game::defs::ShowRangeTag>(entity);
            }
            mRegistry.ctx().get<entt::entity&>("selected_unit"_hs) = entt::null;
            mContext.getInputManager().consumeNextClick();
        }).first);

        mUIManager.addElement(std::move(anchor));
        mAnchorPanel = static_cast<engine::ui::UIPanel*>(mUIManager.getRootElement()->getChildById("mobile_action_bar"_hs));

        // 放置模式取消按钮（独立于操作栏，放置中显示）
        auto cancel_place = make_button("取消放置", glm::vec2((window_size.x - 120.0f) * 0.5f, window_size.y - frame_size.y - 2.0f * ui_config->getUnitPanelPadding() - 56.0f), [this]() {
            mContext.getDispatcher().enqueue(game::defs::CancelPlacementEvent{});
            mContext.getInputManager().consumeNextClick();
        });
        mCancelPlaceButton = cancel_place.first.get();
        mUIManager.addElement(std::move(cancel_place.first));
        mCancelPlaceButton->setVisible(false);

        // 移动端全局 HUD：暂停/倍速（仅在触摸设备显示）
        if (mContext.getInputManager().isTouchDevice()) {
            auto hud_button_size = glm::vec2(56.0f, 56.0f);
            float hud_x = window_size.x - hud_button_size.x - 12.0f;
            float hud_y = 12.0f;

            auto make_hud_button = [&](std::string_view text, const glm::vec2& pos, std::function<void()> callback)
                -> std::pair<std::unique_ptr<engine::ui::UIButton>, engine::ui::UILabel*> {
                auto button = std::make_unique<engine::ui::UIButton>(mContext,
                    btn_image, btn_image, btn_image,
                    pos,
                    hud_button_size,
                    std::move(callback));
                button->setId(entt::hashed_string(text.data()));
                auto label = std::make_unique<engine::ui::UILabel>(mContext.getTextRenderer(),
                    text,
                    ui_config->getUnitPanelFontPath(),
                    16,
                    engine::utils::FColor::white(),
                    glm::vec2(2.0f, hud_button_size.y * 0.5f - 12.0f));
                auto* label_ptr = label.get();
                button->addChild(std::move(label));
                return { std::move(button), label_ptr };
            };

            // 暂停/继续
            {
                auto [button, label] = make_hud_button("⏸", glm::vec2(hud_x, hud_y), [this]() {
                    auto& game_state = mContext.getGameState();
                    if (game_state.isPaused()) {
                        game_state.setState(engine::core::State::Playing);
                    } else {
                        game_state.setState(engine::core::State::Paused);
                    }
                    mContext.getInputManager().consumeNextClick();
                });
                mPauseButton = button.get();
                mPauseLabel = label;
                mUIManager.addElement(std::move(button));
            }

            // 倍速 1x/2x
            {
                auto [button, label] = make_hud_button("1x", glm::vec2(hud_x - hud_button_size.x - 8.0f, hud_y), [this]() {
                    auto& time = mContext.getTime();
                    float scale = time.getTimeScale();
                    scale = (scale >= 2.0f) ? 1.0f : 2.0f;
                    time.setTimeScale(scale);
                    if (mSpeedLabel) {
                        mSpeedLabel->setText(scale >= 2.0f ? "2x" : "1x");
                    }
                    mContext.getInputManager().consumeNextClick();
                });
                mSpeedButton = button.get();
                mSpeedLabel = label;
                mUIManager.addElement(std::move(button));
            }
        }
        refreshVisibility();
    }

    void MobileActionBar::refreshVisibility() {
        if (!mAnchorPanel) return;

        // 放置模式取消按钮显示/隐藏
        if (mCancelPlaceButton) {
            auto& touch_mode = mRegistry.ctx().get<game::defs::TouchMode&>("touch_mode"_hs);
            mCancelPlaceButton->setVisible(touch_mode == game::defs::TouchMode::PLACING);
        }

        // 全局 HUD 标签刷新
        if (mPauseLabel) {
            mPauseLabel->setText(mContext.getGameState().isPaused() ? "▶" : "⏸");
        }
        if (mSpeedLabel) {
            float scale = mContext.getTime().getTimeScale();
            mSpeedLabel->setText(scale >= 2.0f ? "2x" : "1x");
        }

        auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
        bool visible = entity != entt::null && mRegistry.valid(entity) &&
            mRegistry.all_of<game::component::PlayerComponent>(entity);
        mAnchorPanel->setVisible(visible);

        if (!visible) return;

        auto player = mRegistry.get<game::component::PlayerComponent>(entity);
        auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();

        // 升级：显示费用，COST 不足时禁用
        if (mUpgradeButton && mUpgradeLabel) {
            mUpgradeLabel->setText("升级 -" + std::to_string(player.mCost));
            mUpgradeButton->setInteractive(game_stats.mCost >= player.mCost);
        }

        // 撤退：显示返还，始终可用
        if (mRetreatButton && mRetreatLabel) {
            auto return_cost = static_cast<int>(player.mCost * 0.5f);
            mRetreatLabel->setText("撤退 +" + std::to_string(return_cost));
            mRetreatButton->setInteractive(true);
        }

        // 技能：显示冷却/就绪，未就绪时禁用
        if (mSkillButton && mSkillLabel) {
            bool ready = mRegistry.all_of<game::defs::SkillReadyTag>(entity);
            if (auto skill = mRegistry.try_get<game::component::SkillComponent>(entity); skill) {
                if (ready) {
                    mSkillLabel->setText(skill->mName + " 就绪");
                } else {
                    mSkillLabel->setText(skill->mName + " CD");
                }
            } else {
                mSkillLabel->setText("技能");
            }
            mSkillButton->setInteractive(ready);
        }
    }

}   // namespace game::ui
