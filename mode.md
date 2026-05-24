# Air Minecraft - 第一步修改记录：基础飞行器移动系统

## 修改概述

在 Luanti (Minetest) 引擎中实现了飞行器模式的核心移动和视角系统。用户可以通过按键切换进入无人机模式，在三维空间中自由飞行，并使用 5 个方向的视角观察环境。

---

## 按键绑定

| 按键 | 功能 | 默认绑定 |
|------|------|----------|
| V | 切换无人机模式开/关 | `SYSTEM_SCANCODE_25` (KEY_V) |
| C | 在无人机 5 个视角间循环 | 复用原有相机切换键 |
| ] | 下一个无人机视角 | `SYSTEM_SCANCODE_47` |
| [ | 上一个无人机视角 | `SYSTEM_SCANCODE_46` |
| W/A/S/D | 水平移动 | 复用原有移动键 |
| Space | 上升 | 复用原有跳跃键 |
| Shift | 下降 | 复用原有潜行键 |
| E | 精确/慢速模式 | 复用原有辅助键 |

## 无人机视角说明

| 视角 | CameraMode 枚举值 | 说明 |
|------|-------------------|------|
| Front | `CAMERA_MODE_DRONE_FRONT` | 默认前视 |
| Back | `CAMERA_MODE_DRONE_BACK` | 后视（水平翻转 180°） |
| Left | `CAMERA_MODE_DRONE_LEFT` | 左视（逆时针 90°） |
| Right | `CAMERA_MODE_DRONE_RIGHT` | 右视（顺时针 90°） |
| Bottom | `CAMERA_MODE_DRONE_BOTTOM` | 下视（俯瞰） |

---

## 修改文件清单

### 1. `src/player.h` - CameraMode 枚举扩展

在 `CameraMode` 枚举中新增 5 个无人机视角模式：

```cpp
enum CameraMode : int {
    CAMERA_MODE_ANY = 0,
    CAMERA_MODE_FIRST,
    CAMERA_MODE_THIRD,
    CAMERA_MODE_THIRD_FRONT,

    // Drone/Aircraft camera modes
    CAMERA_MODE_DRONE_FRONT,
    CAMERA_MODE_DRONE_BACK,
    CAMERA_MODE_DRONE_LEFT,
    CAMERA_MODE_DRONE_RIGHT,
    CAMERA_MODE_DRONE_BOTTOM,

    CameraMode_END
};
```

### 2. `src/player.cpp` - 枚举字符串映射

在 `es_CameraMode` 表中添加字符串映射，用于 Lua API 和序列化：

```cpp
{CAMERA_MODE_DRONE_FRONT, "drone_front"},
{CAMERA_MODE_DRONE_BACK, "drone_back"},
{CAMERA_MODE_DRONE_LEFT, "drone_left"},
{CAMERA_MODE_DRONE_RIGHT, "drone_right"},
{CAMERA_MODE_DRONE_BOTTOM, "drone_bottom"},
```

### 3. `src/client/keys.h` - 按键类型定义

新增 3 个按键类型：

```cpp
DRONE_TOGGLE,        // 切换无人机模式
DRONE_VIEW_NEXT,     // 下一个视角
DRONE_VIEW_PREV,     // 上一个视角
```

### 4. `src/client/inputhandler.cpp` - 按键绑定注册

在 `reloadKeybindings()` 中注册新按键：

```cpp
keybindings[KeyType::DRONE_TOGGLE] = getKeySetting("keymap_drone_toggle");
keybindings[KeyType::DRONE_VIEW_NEXT] = getKeySetting("keymap_drone_view_next");
keybindings[KeyType::DRONE_VIEW_PREV] = getKeySetting("keymap_drone_view_prev");
```

### 5. `src/defaultsettings.cpp` - 默认按键设置

```cpp
settings->setDefault("keymap_drone_toggle", "SYSTEM_SCANCODE_25"); // V
settings->setDefault("keymap_drone_view_next", "SYSTEM_SCANCODE_47"); // ]
settings->setDefault("keymap_drone_view_prev", "SYSTEM_SCANCODE_46"); // [
```

### 6. `src/client/localplayer.h` - 无人机模式标志

`PlayerSettings` 结构体新增：
```cpp
bool drone_mode = false;
```

`LocalPlayer` 类新增：
```cpp
int drone_view_index = 0;  // 0=front, 1=back, 2=left, 3=right, 4=bottom
```

### 7. `src/client/localplayer.cpp` - 无人机移动逻辑

**applyControl() 方法** - 在 parent check 之后添加无人机模式分支：
- 无重力影响
- 使用 `movement_speed_fast` 作为默认速度
- 按住 E 键切换为 `movement_speed_walk` 精确模式
- Space 上升，Shift 下降
- 使用高加速度实现快速响应

**move() 方法** - 在重力计算处添加：
```cpp
if (player_settings.drone_mode) {
    accel_f = v3f(0, 0, 0); // 无人机无重力
}
```

### 8. `src/client/camera.h` - 相机方法声明

新增方法：
```cpp
void cycleDroneView(bool reverse = false);
void enterDroneMode();
void exitDroneMode();
bool isDroneMode() const;
```

### 9. `src/client/camera.cpp` - 相机视角实现

**eye_offset 处理** - 无人机模式使用轻微上移偏移：
```cpp
case CAMERA_MODE_DRONE_FRONT ... CAMERA_MODE_DRONE_BOTTOM:
    eye_offset += v3f(0, 0.2f * BS, 0);
    break;
```

**视角方向旋转** - 根据无人机视角模式旋转相机方向：
- FRONT: 无旋转
- BACK: X/Z 取反（180°）
- LEFT: X=-Z, Z=X（逆时针 90°）
- RIGHT: X=Z, Z=-X（顺时针 90°）
- BOTTOM: 方向设为 (0,-1,0)，up 向量调整

**toggleCameraMode()** - 无人机模式下循环 5 个视角

**cycleDroneView()** - 实现 FRONT→BACK→LEFT→RIGHT→BOTTOM 循环

### 10. `src/client/game_internal.h` - Game 类声明

新增方法声明：
```cpp
void toggleDroneMode();
```

### 11. `src/client/game.cpp` - 游戏循环集成

**按键处理** - 在 `processUserInput()` 中添加：
- DRONE_TOGGLE → toggleDroneMode()
- DRONE_VIEW_NEXT/PREV → cycleDroneView() + 状态文本显示

**toggleDroneMode() 实现**：
- 切换 PlayerSettings.drone_mode
- 启用时：设置 free_move=true, noclip=true, 进入 DRONE_FRONT 视角
- 禁用时：恢复 CAMERA_MODE_FIRST 视角

**shootline 处理** - 无人机模式使用相机位置作为射击起点

**鼠标反转** - DRONE_BACK 视角下启用鼠标 Y 轴反转

---

## 编译状态

编译成功，无错误。可执行文件位于 `/Users/wansuishan/Documents/code/AirMinecraft/bin/luanti`

---

## 已知限制

1. 进入无人机模式时自动启用飞行和穿墙，退出时不自动恢复（需手动切换）
2. 无人机视角下的武器显示已禁用（draw_wield_tool 仅在 FIRST 模式下生效）
3. 触摸屏的 shootline 在无人机模式下不生效

---

# 第二步修改记录：UI 集成

## 修改概述

完成了飞行器模式的 UI 集成，包括主菜单标题修改、新的无人机模式标签页、设置对话框和暂停菜单增强。

## 修改文件清单

### 1. `src/client/clientlauncher.cpp` - 窗口标题

将主菜单窗口标题从 `PROJECT_NAME_C` 改为 `"Air Minecraft"`。

### 2. `src/client/game.cpp` - 游戏窗口标题

将游戏内窗口标题从 `PROJECT_NAME_C` 改为 `"Air Minecraft"`。

### 3. `builtin/mainmenu/init.lua` - 主菜单注册

- 加载 `dlg_drone_settings.lua` 设置对话框
- 在 tabview 中添加 `drone_mode` 标签页（位于 "Start Game" 之后）

### 4. `builtin/mainmenu/tab_drone.lua` - 无人机模式标签页（新建）

新建的标签页包含：
- 世界选择列表
- 无人机模式选项（默认启用、高速模式、显示 HUD）
- 控制说明面板
- "Drone Settings" 按钮打开设置对话框
- "Start Drone Flight" 启动按钮

### 5. `builtin/mainmenu/dlg_drone_settings.lua` - 无人机设置对话框（新建）

设置项包括：
- 飞行速度（blocks/sec）
- 视野角度（FOV）
- 显示无人机 HUD
- 启用碰撞检测
- 启动时自动进入无人机模式
- 默认相机视角选择

### 6. `src/gui/mainmenumanager.h` - 游戏回调接口

- `IGameCallback` 新增 `virtual void toggleDroneMode() = 0`
- `MainGameCallback` 新增 `toggleDroneMode()` 实现和 `drone_toggle_requested` 标志

### 7. `src/client/game_formspec.cpp` - 暂停菜单

- 标题改为 "Air Minecraft"
- 新增 "Toggle Drone Mode" 按钮
- 处理按钮点击回调

### 8. `src/client/game.cpp` - 回调处理

在游戏主循环中处理 `drone_toggle_requested` 回调。

## 编译状态

编译成功，无错误。

## 使用说明

1. 启动程序后，主菜单窗口标题显示 "Air Minecraft"
2. 点击 "Drone Mode" 标签页进入无人机模式设置
3. 选择世界后点击 "Start Drone Flight" 开始飞行
4. 游戏中按 V 键切换无人机模式
5. 暂停菜单中可点击 "Toggle Drone Mode" 切换模式

---

# 第三步修改记录：HUD 与信息显示

## 修改概述

实现了飞行器模式专用的 HUD 叠加层，在玩家进入无人机模式时自动显示飞行信息面板、罗盘、视角指示器和速度条。

## HUD 显示元素

| 元素 | 位置 | 说明 |
|------|------|------|
| 信息面板 | 右上角 | 显示高度 (ALT)、速度 (SPD)、坐标 (X/Y/Z)、偏航角 (YAW)、俯仰角 (PITCH) |
| 罗盘 | 顶部居中 | 显示方向 (N/NE/E/SE/S/SW/W/NW) 和角度 |
| 视角指示器 | 底部居中 | 显示当前无人机视角 [FRONT]/[BACK]/[LEFT]/[RIGHT]/[BOTTOM] |
| 准星 | 屏幕中心 | 带间距的十字线和中心点 |
| 速度条 | 左下角 | 图形化速度显示条，高速时变黄色警告 |

## 修改文件清单

### 1. `src/client/drone_hud.h` - DroneHud 类声明（新建）

```cpp
class DroneHud
{
public:
    DroneHud(Client *client);
    void draw(const Camera *camera, const LocalPlayer *player);

private:
    void drawInfoPanel(const Camera *camera, const LocalPlayer *player);
    void drawCompass(const LocalPlayer *player);
    void drawViewIndicator(const Camera *camera);
    void drawReticle();
    void drawSpeedBar(const LocalPlayer *player);

    static const char *getCompassDirection(f32 yaw);
    static const char *getViewName(int view_index);

    Client *m_client;
    video::IVideoDriver *m_driver;
    s32 m_font_height;
};
```

### 2. `src/client/drone_hud.cpp` - DroneHud 实现（新建）

- 使用 Irrlicht 的 `driver->draw2DRectangle()` 和 `driver->draw2DLine()` 绘制 2D 图形
- 使用 `g_fontengine->getFont()` 获取字体绘制文本
- 面板背景使用半透明黑色 (`SColor(180, 0, 0, 0)`)
- 边框和文字使用绿色主题 (`SColor(200, 0, 180, 0)`)
- 罗盘方向通过偏航角计算 8 方位 (N/NE/E/SE/S/SW/W/NW)
- 速度条最大显示 30 m/s，超过 20 m/s 时变黄色警告

### 3. `src/client/render/plain.h` - DrawHUD 类修改

新增 `DroneHud` 前向声明和成员变量：
```cpp
class DrawHUD : public RenderStep
{
public:
    DrawHUD();
    ~DrawHUD() override;
    // ... existing methods ...
private:
    std::unique_ptr<DroneHud> m_drone_hud;
};
```

构造函数和析构函数在 .cpp 文件中定义，以确保 `unique_ptr<DroneHud>` 在 DroneHud 类型完整时销毁。

### 4. `src/client/render/plain.cpp` - DrawHUD 集成

新增 include：
```cpp
#include "client/drone_hud.h"
#include "client/localplayer.h"
```

在 `DrawHUD::run()` 中添加无人机 HUD 绘制：
```cpp
// Drone mode HUD overlay
Camera *camera = context.client->getCamera();
LocalPlayer *player = context.client->getEnv().getLocalPlayer();
if (camera && camera->isDroneMode() && player) {
    if (!m_drone_hud)
        m_drone_hud = std::make_unique<DroneHud>(context.client);
    m_drone_hud->draw(camera, player);
}
```

DroneHud 使用懒初始化，仅在首次进入无人机模式时创建。

### 5. `src/client/CMakeLists.txt` - 新增源文件

在 `hud.cpp` 后添加：
```cmake
${CMAKE_CURRENT_SOURCE_DIR}/drone_hud.cpp
```

## 编译状态

编译成功，无错误。可执行文件位于 `/Users/wansuishan/Documents/code/AirMinecraft/bin/luanti`

## 使用说明

1. 按 V 键进入无人机模式，HUD 自动显示
2. 右上角面板实时显示飞行数据（高度、速度、坐标、角度）
3. 顶部罗盘显示飞行方向
4. 底部显示当前视角名称
5. 左下角速度条直观显示当前速度
6. 按 ]/[ 切换视角时，底部指示器实时更新
7. 退出无人机模式后 HUD 自动隐藏
