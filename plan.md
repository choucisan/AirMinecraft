# Air Minecraft - 飞行器模式开发计划

## 一、项目概述

### 1.1 项目身份

这是 **Luanti**（原名 Minetest），一个知名的开源体素游戏引擎，使用 **C++17** 编写，配合 **Lua** 脚本系统。版本 5.17.0-dev，使用 CMake 构建，IrrlichtMt 作为渲染引擎。

- 许可证：LGPL-2.1-or-later
- 平台：Windows / Linux / macOS / Android
- 核心特点：引擎与游戏分离，通过 Lua mod 定义游戏逻辑

### 1.2 项目目标

在 Luanti 引擎基础上增加 **飞行器模式（Drone Mode）**，使其能作为强化学习环境使用（兼容 OpenAI Gym 接口）。飞行器可以：
- 在三维空间中自由飞行（不受地面约束）
- 提供 5 个方向的视角（前、后、左、右、下）
- 通过 API 接口控制移动和观察
- 保留原有游戏的完整性和美观性

---

## 二、项目文件架构

```
AirMinecraft/
├── src/                          # C++ 引擎核心代码
│   ├── main.cpp                  # 程序入口 (line 132: main())
│   ├── client/                   # 客户端代码（最关键）
│   │   ├── game.cpp              # 主游戏循环 (line 478: Game::run())
│   │   ├── game_internal.h       # Game 类定义 (line 93)
│   │   ├── game.h                # the_game() 函数声明
│   │   ├── clientlauncher.cpp    # 菜单-游戏循环 (line 98: ClientLauncher::run())
│   │   ├── camera.cpp/h          # 相机系统 (line 312: Camera::update())
│   │   ├── localplayer.cpp/h     # 本地玩家控制 (line 534: applyControl())
│   │   ├── clientenvironment.h   # 客户端环境管理
│   │   ├── client.cpp            # 客户端主逻辑
│   │   ├── keys.h                # 按键类型定义
│   │   ├── inputhandler.h        # 输入处理
│   │   └── game_formspec.cpp/h   # 游戏内表单
│   ├── player.h                  # 玩家基类 (速度常量, CameraMode 枚举)
│   ├── constants.h               # 常量定义 (BS=10.0f, MAP_BLOCKSIZE=16)
│   ├── map.h                     # 地图基类
│   ├── mapblock.h                # 区块 (16x16x16 节点)
│   ├── gui/                      # GUI 系统
│   │   └── guiEngine.h           # 主菜单 GUI 引擎
│   ├── script/                   # Lua 脚本绑定
│   ├── mapgen/                   # 地形生成
│   └── util/                     # 工具函数
├── builtin/                      # Lua 脚本（主菜单 + 游戏逻辑）
│   ├── mainmenu/
│   │   ├── init.lua              # 主菜单入口（创建 tabview）
│   │   ├── tab_local.lua         # "开始游戏" 标签页
│   │   ├── tab_online.lua        # 联机标签页
│   │   ├── tab_content.lua       # 内容管理标签页
│   │   └── tab_about.lua         # 关于标签页
│   ├── pause_menu/
│   │   └── init.lua              # 暂停菜单
│   ├── fstk/                     # UI 框架
│   │   ├── tabview.lua           # 标签视图组件
│   │   ├── buttonbar.lua         # 按钮栏组件
│   │   ├── dialog.lua            # 对话框组件
│   │   └── ui.lua                # UI 管理器
│   └── common/                   # 公共 Lua 模块
├── irr/                          # IrrlichtMt 渲染引擎
├── lib/                          # 第三方库
├── games/devtest/                # 开发测试游戏
├── textures/                     # 纹理包
├── fonts/                        # 字体
├── doc/                          # 文档
└── CMakeLists.txt                # 顶层构建配置
```

---

## 三、核心系统分析

### 3.1 游戏主循环

入口：`main()` → `ClientLauncher::run()` → `the_game()` → `Game::run()`

`Game::run()` 每帧执行（game.cpp:515-603）：
```
1. draw_times.limit()            — 帧率限制
2. updateStats()                 — 更新运行统计
3. updateInteractTimers()        — 交互冷却
4. checkConnection()             — 连接检查
5. processQueues()               — 事件队列
6. updateCameraOffset()          — 浮点原点偏移
7. processUserInput(dtime)       — 处理键盘鼠标输入
8. updateCameraDirection()       — 更新相机方向
9. updatePlayerControl()         — 输入 → PlayerControl 结构体
10. step(dtime)                  — 推进游戏模拟
11. processClientEvents()        — 处理服务器事件
12. updateCamera()               — 更新相机位置/朝向
13. updateSound()                — 音频
14. processPlayerInteraction()   — 方块交互
15. updateFrame()                — 渲染场景
```

### 3.2 玩家移动系统

**类继承链**：`Player` (player.h) → `LocalPlayer` (localplayer.h:47)

**移动流程**：
1. `Game::processUserInput()` — 读取按键状态，切换 fly/fast/noclip 模式
2. `Game::updatePlayerControl()` — 将按键+相机朝向组装成 `PlayerControl` 结构体
3. `LocalPlayer::applyControl()` (localplayer.cpp:534) — 将 PlayerControl 转换为目标速度
4. `LocalPlayer::move()` (localplayer.cpp:210) — 物理/碰撞步进

**飞行模式**（已存在但简陋）：
- `free_move` 标志位控制飞行模式
- `applyControl()` 中：jump 上升, sneak 下降
- 速度由 `movement_speed_walk` / `movement_speed_fast` 决定
- 无碰撞检测中的重力影响（飞行时 gravity 被忽略）

**移动速度常量** (player.h:179-191)：
- `movement_speed_walk = 4 * BS` (40 单位/秒)
- `movement_speed_fast = 20 * BS` (200 单位/秒)
- `movement_acceleration_default = 2.4 * BS`
- `movement_acceleration_air = 1.2 * BS`

**PlayerControl 结构体** (player.h:42-96)：
```cpp
struct PlayerControl {
    u8 direction_keys;  // 编码的方向键
    bool jump, aux1, sneak, zoom, dig, place;
    float pitch, yaw;          // 相机朝向
    float movement_speed;      // 移动速度 (0-1)
    float movement_direction;  // 移动方向 (弧度)
};
```

**物理覆盖** (player.h:98-124)：
```cpp
struct PlayerPhysicsOverride {
    float speed, jump, gravity;
    float speed_walk, speed_fast, speed_crouch, speed_climb;
    float acceleration_default, acceleration_air, acceleration_fast;
    // ... 等等
};
```

### 3.3 相机系统

**Camera 类** (camera.h:60, camera.cpp)：
- 三个场景节点：`m_playernode` → `m_headnode` → `m_cameranode`
- 当前支持 3 种模式：FIRST / THIRD / THIRD_FRONT
- 通过 `toggleCameraMode()` (camera.cpp:647) 切换，绑定 'C' 键

**Camera::update()** (camera.cpp:312) 流程：
1. 获取玩家位置，平滑楼梯移动
2. 设置玩家节点旋转（yaw）
3. 计算 eye_offset（根据模式选择 first/third/third_front）
4. 应用受伤倾斜动画
5. 应用行走晃动 (view bobbing)
6. 第三人称模式：射线检测避免穿墙
7. 设置 FOV（支持平滑过渡/服务器覆盖/缩放）
8. 更新武器惯性
9. 更新武器位置和动画
10. 更新渲染距离

**CameraMode 枚举** (player.h:127-135)：
```cpp
enum CameraMode : int {
    CAMERA_MODE_ANY = 0,
    CAMERA_MODE_FIRST,
    CAMERA_MODE_THIRD,
    CAMERA_MODE_THIRD_FRONT,
    CameraMode_END
};
```

**视图晃动逻辑** (camera.cpp:556-573)：
- 当玩家在地面行走、游泳或攀爬时启用
- 飞行模式 (`free_move`) 下禁用 — 这对无人机模式是好消息

### 3.4 输入系统

**按键定义** (keys.h)：`KeyType::T` 枚举，包含所有游戏操作

**输入流程**：
1. OS 事件 → `MyEventReceiver::OnEvent()`
2. `Game::processUserInput()` — 读取按键状态，分发动作
3. `Game::updateCameraDirection()` — 鼠标/触摸/手柄 → yaw/pitch
4. `Game::updatePlayerControl()` — 按键 + 相机 → `PlayerControl`
5. `LocalPlayer::applyControl()` — PlayerControl → 实际速度

### 3.5 主菜单系统

**入口**：`builtin/mainmenu/init.lua`

使用 `tabview_create()` 创建标签视图，当前有 4 个标签：
- `local_game` — 开始单人游戏
- `play_online` — 服务器浏览器
- `content` — 内容管理
- `about` — 关于

每个标签是一个 Lua 表，包含 `name`, `caption`, `cbf_formspec`, `cbf_button_handler`, `on_change`。

---

## 四、飞行器模式设计方案

### 4.1 设计原则

1. **完整性**：作为 Luanti 引擎的一等公民，不是简单 hack
2. **精美性**：UI/UX 与原版风格统一
3. **通用性**：既能手动操控，也支持 API/RL 控制
4. **功能齐全**：5 个视角、自由飞行、速度控制、状态反馈

### 4.2 核心修改清单

#### 阶段一：飞行器实体与移动（C++ 核心）

**1. 新增 `DroneEntity` 类**
- 文件：`src/client/drone.h` / `src/client/drone.cpp`
- 继承自 `ClientActiveObject` 或作为 `LocalPlayer` 的特殊模式
- 独立的碰撞箱（小型，如 0.3x0.3x0.3 BS）
- 自定义物理参数（无重力、高速度、快速响应）
- 电池/电量系统（可选）

**2. 修改 `LocalPlayer` 移动逻辑**
- 文件：`src/client/localplayer.cpp` (applyControl, move)
- 新增 `drone_mode` 标志位
- 飞行器模式下：
  - 无重力
  - 无行走晃动 (view bobbing)
  - 独立的速度参数
  - 无碰撞箱与地面的交互
  - 可以自由穿越液体

**3. 扩展 CameraMode**
- 文件：`src/player.h` (CameraMode 枚举), `src/client/camera.cpp`
- 新增 5 个无人机视角模式：
  ```cpp
  CAMERA_MODE_DRONE_FRONT,     // 前视
  CAMERA_MODE_DRONE_BACK,      // 后视
  CAMERA_MODE_DRONE_LEFT,      // 左视
  CAMERA_MODE_DRONE_RIGHT,     // 右视
  CAMERA_MODE_DRONE_BOTTOM,    // 下视
  ```
- 每个视角对应不同的 eye_offset 和 camera rotation
- 可通过按键循环切换（如 'V' 键）

**4. 新增按键绑定**
- 文件：`src/client/keys.h`
- 新增：
  ```cpp
  DRONE_TOGGLE,        // 切换飞行器模式
  DRONE_VIEW_NEXT,     // 下一个视角
  DRONE_VIEW_PREV,     // 上一个视角
  DRONE_ASCEND,        // 上升
  DRONE_DESCEND,       // 下降
  DRONE_HOVER,         // 悬停
  ```

**5. 修改游戏循环**
- 文件：`src/client/game.cpp`
- `processUserInput()` — 处理飞行器按键
- `updateCameraDirection()` — 飞行器视角更新
- `updatePlayerControl()` — 飞行器控制逻辑
- `updateCamera()` — 飞行器相机更新

#### 阶段二：菜单系统（Lua UI）

**6. 修改主菜单**
- 文件：`builtin/mainmenu/init.lua`
- 修改标题为 "Air Minecraft"
- 新增 "飞行器模式" 标签页或在 "开始游戏" 中添加选项

**7. 创建飞行器设置对话框**
- 文件：`builtin/mainmenu/dlg_drone_settings.lua`（新建）
- 设置项：
  - 飞行速度
  - 视角切换模式
  - 是否显示 HUD 信息（电量、高度、坐标）
  - 控制方案选择

**8. 修改暂停菜单**
- 文件：`builtin/pause_menu/init.lua`
- 添加飞行器状态显示
- 添加飞行器设置快捷入口

#### 阶段三：HUD 与状态显示

**9. 飞行器 HUD**
- 文件：`src/client/hud.cpp` 或新建 `src/client/drone_hud.cpp`
- 显示：
  - 高度计
  - 速度计
  - 指南针/朝向
  - 电量条（可选）
  - 小地图增强（标记当前位置）

#### 阶段四：API / Gym 接口

**10. Lua API 扩展**
- 文件：`src/script/scripting_client.cpp` 等
- 暴露飞行器控制 API：
  ```lua
  drone.set_position(x, y, z)
  drone.get_position()
  drone.set_velocity(vx, vy, vz)
  drone.get_velocity()
  drone.get_observation()  -- 获取 5 个视角的图像
  drone.step(action)       -- 执行一步动作
  drone.reset()            -- 重置环境
  ```

**11. Python Gym Wrapper**（可选，独立仓库）
- 封装 Lua API 为 Python Gym 环境
- 提供标准的 `step()`, `reset()`, `observation_space`, `action_space`

---

## 五、关键文件修改列表

| 优先级 | 文件 | 修改内容 |
|--------|------|----------|
| P0 | `src/player.h` | 新增 CameraMode 枚举值 (DRONE_*) |
| P0 | `src/client/localplayer.h` | 新增 drone_mode 标志、无人机物理参数 |
| P0 | `src/client/localplayer.cpp` | applyControl() 中添加无人机移动逻辑 |
| P0 | `src/client/camera.cpp` | update() 中添加无人机视角处理 |
| P0 | `src/client/camera.h` | 新增无人机相关成员变量和方法 |
| P0 | `src/client/keys.h` | 新增无人机按键定义 |
| P1 | `src/client/game.cpp` | processUserInput/updatePlayerControl 中添加无人机逻辑 |
| P1 | `src/client/game_internal.h` | Game 类新增无人机相关成员 |
| P1 | `builtin/mainmenu/init.lua` | 修改主菜单标题和结构 |
| P1 | `builtin/mainmenu/tab_local.lua` | 添加飞行器模式启动选项 |
| P2 | `builtin/mainmenu/dlg_drone_settings.lua` | 新建飞行器设置对话框 |
| P2 | `src/client/drone_hud.cpp` | 新建飞行器 HUD 显示 |
| P3 | `src/script/scripting_client.cpp` | 暴露飞行器 Lua API |
| P3 | `builtin/mainmenu/tab_about.lua` | 更新关于页面信息 |

---

## 六、实施顺序

### 第一步：基础飞行器移动（让无人机能飞起来）
1. 在 `player.h` 扩展 CameraMode 枚举
2. 在 `localplayer.h/cpp` 添加 drone_mode 和独立物理
3. 在 `keys.h` 添加按键
4. 在 `game.cpp` 处理输入

### 第二步：多视角系统（让无人机有 5 个眼睛）
1. 在 `camera.h/cpp` 实现 5 个视角
2. 添加视角切换逻辑
3. 调整每个视角的 eye_offset

### 第三步：UI 集成（让用户能选择和配置）
1. 修改主菜单标题和样式
2. 添加飞行器模式入口
3. 创建设置对话框
4. 暂停菜单增强

### 第四步：HUD 与信息显示（让飞行器状态可见）
1. 高度/速度/坐标显示
2. 视角指示器
3. 小地图增强

### 第五步：API 暴露（让 RL 可以控制）
1. Lua API 绑定
2. Gym wrapper（Python 侧）

---

## 七、技术要点

### 7.1 飞行器物理与原版飞行的区别

原版飞行 (`free_move`)：
- 仍然是玩家实体
- 有碰撞检测
- 速度受 `movement_speed_walk/fast` 限制
- 有 view bobbing

飞行器模式：
- 独立的物理参数（更高加速度、更快速度）
- 可选择是否启用碰撞
- 无 view bobbing
- 可以悬停
- 支持精确位移控制（RL 需要）

### 7.2 视角实现细节

5 个视角的 eye_offset 和 rotation：

| 视角 | eye_offset | pitch | yaw_offset |
|------|-----------|-------|------------|
| 前 | (0, 0, 0) | 玩家 pitch | 0° |
| 后 | (0, 0, 0) | 玩家 pitch | 180° |
| 左 | (0, 0, 0) | 玩家 pitch | -90° |
| 右 | (0, 0, 0) | 玩家 pitch | 90° |
| 下 | (0, -0.5, 0) | -90° | 0° |

或者更高级的方案：使用多个渲染目标（Render Targets），同时渲染 5 个视角到纹理，用于 RL 的 observation。

### 7.3 RL 集成考虑

- **观测空间**：5 个视角的 RGB 图像（或深度图）
- **动作空间**：6 自由度 (vx, vy, vz, roll, pitch, yaw) 或简化为 (前进/后退, 左/右, 上/下, 旋转)
- **奖励函数**：由外部定义（到达目标、收集物品等）
- **episode 管理**：reset() 恢复初始状态

---

## 八、风险与注意事项

1. **兼容性**：修改必须保持与现有 Lua mod 和多人游戏的兼容
2. **性能**：5 个视角渲染对 GPU 要求较高，需要优化（可选低分辨率/低帧率）
3. **网络协议**：飞行器模式最好作为客户端扩展，不影响服务器协议
4. **构建系统**：新增文件需更新 CMakeLists.txt
5. **测试**：需要在多个平台上测试（至少 macOS + Linux）

---

## 九、参考资料

- Luanti 文档：`doc/` 目录
- 游戏循环：`src/client/game.cpp:478`
- 玩家移动：`src/client/localplayer.cpp:534` (applyControl)
- 相机系统：`src/client/camera.cpp:312` (update)
- 主菜单：`builtin/mainmenu/init.lua`
- UI 框架：`builtin/fstk/`
- 按键定义：`src/client/keys.h`
- 常量定义：`src/constants.h`
