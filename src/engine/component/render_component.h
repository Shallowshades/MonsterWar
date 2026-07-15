#pragma once

namespace engine::component {

    /**
     * @brief 渲染组件, 包含图层ID和深度。
     */
    struct RenderComponent {
        int mLayer{};                           ///< @brief 图层ID，数字越小越先绘制 
        float mDepth{};                         ///< @brief 在同一图层内的深度，数字越小越先绘制
        /*  (可用于实现y-sort排序，也可设定其它渲染顺序逻辑) */
// TODO: 未来可添加其他信息，比如透明度等 ...

        RenderComponent(int layer = 0, float depth = 0.0f) : mLayer(layer), mDepth(depth) {}

        // 重载比较运算符，用于排序
        bool operator<(const RenderComponent& other) const {
            if (mLayer == other.mLayer) {       // 如果图层相同，则比较深度
                return mDepth < other.mDepth;
            }
            return mLayer < other.mLayer;       // 如果图层不同，则比较图层ID
        }
    };

}   // namespace engine::component