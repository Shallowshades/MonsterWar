#pragma once

#include "../utils/math.h"

namespace engine::component {

    /**
     * @brief 渲染组件, 包含图层ID、深度和颜色调整参数。
     */
    struct RenderComponent {
        static constexpr int MAIN_LAYER{ 10 };    ///< @brief 主图层ID，默认为10

        int mLayer{};                           ///< @brief 图层ID，数字越小越先绘制
        float mDepth{};                         ///< @brief 在同一图层内的深度，数字越小越先绘制
        /*  (可用于实现y-sort排序，也可设定其它渲染顺序逻辑) */
        engine::utils::FColor mColor{ engine::utils::FColor::white() };  ///< @brief 颜色调整参数（最终颜色 = 原始颜色 * 调整颜色）

        /**
         * @brief 构造函数
         * @param layer 图层ID，数字越小越先绘制（默认MAIN_LAYER）
         * @param depth 同一图层内的深度，数字越小越先绘制（默认0）
         * @param color 颜色调整参数（默认白色，即不调整）
         */
        RenderComponent(int layer = MAIN_LAYER, float depth = 0.0f,
            engine::utils::FColor color = engine::utils::FColor::white())
            : mLayer(layer), mDepth(depth), mColor(color) {}

        // 重载比较运算符，用于排序
        bool operator<(const RenderComponent& other) const {
            if (mLayer == other.mLayer) {       // 如果图层相同，则比较深度
                return mDepth < other.mDepth;
            }
            return mLayer < other.mLayer;       // 如果图层不同，则比较图层ID
        }
    };

}   // namespace engine::component