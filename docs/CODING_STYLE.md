# MonsterWar 编码风格规范

> 每次编写代码前，务必先阅读本文件以遵循项目一致的代码风格。

---

## 1. 文件头注释 (Doxygen)

每个 `.h` 和 `.cpp` 文件必须以以下格式开头：

```cpp
/*****************************************************************//**
 * @file   filename
 * @brief  brief description
 * @version 1.0
 *
 * @author Shallowshades
 * @date   YYYY.MM.DD
 *********************************************************************/
```

- 使用 `/**` 嵌入 Doxygen 注释
- `@file` 为文件名，`@brief` 为一句话描述
- `@author` 统一为 `Shallowshades`
- `@date` 格式为 `YYYY.MM.DD`

---

## 2. 头文件保护

双重保护：`#pragma once`（第一行）+ `#ifndef` / `#define` / `#endif` 宏：

```cpp
#pragma once
#ifndef FILENAME_H
#define FILENAME_H

// ... 内容 ...

#endif // FILENAME_H
```

- `#pragma once` 在文件头注释之后，独占一行
- `#endif` 后加注释重复宏名：`#endif // CONFIG_H`

---

## 3. 命名规范

| 类别 | 格式 | 示例 |
|------|------|------|
| **命名空间** | `snake_case`，两级或三级 | `engine::core`, `engine::ui::state`, `game::scene` |
| **类 / 结构体** | `PascalCase` | `GameApp`, `RenderSystem`, `SpriteComponent` |
| **函数 / 方法** | `camelCase` | `loadLevel()`, `registerSceneSetup()`, `getTexture()` |
| **成员变量** | `m` + `PascalCase` | `mWindow`, `mIsRunning`, `mResourceManager`, `mPosition` |
| **静态常量** | `m` + `PascalCase`（与成员变量一致） | `mLogTag` |
| **函数参数** | `snake_case` | `delta_time`, `file_path`, `source_rect`, `is_visible` |
| **局部变量** | `snake_case` 或 `camelCase` | `window_width`, `vsyncMode`, `logical_width` |
| **枚举类** | `PascalCase`，值全大写 | `enum class State { Title, Playing, Paused }` |
| **模板参数** | 大写单字母 | `T` |

### 特例

- 个别遗留代码使用 `is_flipped_`（后缀下划线 + snake_case），新代码统一使用 `mIsFlipped`

---

## 4. 注释风格

### Doxygen 类/结构体注释

```cpp
/**
 * @brief 一句话说明
 *
 * 详细说明（可选）。
 */
```

### Doxygen 方法注释

短声明用行尾注释：

```cpp
void update(float delta);       ///< @brief 更新场景。
```

长声明用块注释：

```cpp
/**
 * @brief 构造函数。
 * @param position 位置
 * @param scale 缩放，默认(1.0f, 1.0f)
 * @param rotation 旋转，默认0
 */
explicit TransformComponent(glm::vec2 position,
    glm::vec2 scale = glm::vec2(1.0f, 1.0f),
    float rotation = 0.0f);
```

### 行内注释

- 使用 `//` 风格
- 注释语言：**中文**
- 适当的空格：`// 注释内容`（// 后跟一个空格）

```cpp
// 遍历获取的实体，获取组件并执行相关逻辑
// 禁用拷贝和移动语义
// TODO: 未来可添加其他信息，比如透明度等 ...
```

---

## 5. 代码布局

### 类的成员顺序

`public` → `protected` → `private`（特殊情况下 `private` 可在 `public` 之前，如 Config 类）

### 构造函数的初始化列表

```cpp
ClassName(Type param1, Type param2)
    : mMember1(std::move(param1)),
      mMember2(param2) {
    // 构造函数体
}
```

### 命名空间闭合注释

```cpp
} // namespace engine::core
} // namespace engine::system
```

### 花括号风格

- **函数/方法**: Egyptian（左括号在行尾）
- **命名空间**: 左括号独占一行
- **控制流**: Egyptian

```cpp
void func() {
    if (condition) {
        // ...
    }
}
```

### 拷贝/移动语义删除

始终显式删除并加注释：

```cpp
ClassName(const ClassName&) = delete;           ///< @brief 删除拷贝构造
ClassName& operator=(const ClassName&) = delete; ///< @brief 删除拷贝赋值构造
ClassName(ClassName&&) = delete;                ///< @brief 删除移动构造
ClassName& operator=(ClassName&&) = delete;     ///< @brief 删除移动赋值构造
```

---

## 6. 代码模式惯例

### 智能指针与所有权

- **独占所有权**: `std::unique_ptr<T>`，成员变量
- **非拥有引用**: 原始指针（`T*`）或引用（`T&`）
- **SDL 对象**: 原始指针（`SDL_Window*`, `SDL_Renderer*`）
- **Context 传递**: 始终使用引用 `Context&`，不使用指针

### 空指针检查

```cpp
// 推荐
if (ptr != nullptr) { ... }

// 不推荐
if (ptr) { ... }
```

### 返回值属性

对不可忽略的返回值使用 `[[nodiscard]]`：

```cpp
[[nodiscard]] bool init();
[[nodiscard]] bool loadLevel(std::string_view level_path, Scene* scene);
```

### 日志

使用 `spdlog`，语言为中文，格式：

```cpp
spdlog::trace("{} 初始化成功", mLogTag.data());
spdlog::error("{} 初始化失败: {}", mLogTag.data(), e.what());
spdlog::warn("{} 警告信息", mLogTag.data());
spdlog::info("GameScene 构造完成");
```

### 异常处理

初始化函数中捕获异常并记录日志后返回 false，不重新抛出：

```cpp
try {
    mDispatcher = std::make_unique<entt::dispatcher>();
} catch (const std::exception& e) {
    spdlog::error("{} 初始化失败: {}", mLogTag.data(), e.what());
    return false;
}
```

### 字符串参数类型

- 只读字符串参数使用 `std::string_view`
- 成员变量中需要拥有数据时使用 `std::string`

### `explicit` 关键字

单参数构造函数必须加 `explicit`：

```cpp
explicit GameState(SDL_Window* window, SDL_Renderer* renderer, State initial_state = State::Title);
explicit Config(std::string_view filePath);
```

### `final` 关键字

不期望被继承的具体类加 `final`：

```cpp
class GameApp final { ... };
class GameState final { ... };
```

---

## 7. 项目特定架构规范

### ECS 组件 (`engine::component`)

- 使用 `struct`（公开成员）
- 数据成员使用 `m` 前缀
- 提供带参数的构造函数以方便 emplace 构造
- 参数名 `snake_case`，使用 `std::move()` 转移

### ECS 系统 (`engine::system`)

- 使用 `class`（可公开方法）
- 核心方法命名为 `update()`
- 通过参数接收 `entt::registry&` 和其他依赖

### Scene 派生

- 构造函数中创建 system 对象（`std::make_unique`）
- `init()` 中加载关卡
- `update()` 中按顺序调用各 system 的 `update()`
- `render()` 中调用渲染 system

### 模块间引用规则

- `engine/` 层**不能**引用 `game/` 层的代码
- `game/` 层引用 `engine/` 层
- 通过 `Context` 类进行依赖注入

---

## 8. 缩进与格式

- **缩进**: Tab（显示宽度 4 空格）
- **行宽**: 无严格限制，但建议合理换行
- **include 顺序**: 标准库 → 第三方库(SDL/EnTT/glm/spdlog) → 项目内部头文件
- **空行**: 类定义中方法之间使用空行分隔；cpp 文件中函数之间使用空行
