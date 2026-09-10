# Fcitx5 输入计数器

[English](README.md) | 简体中文

一个 Fcitx5 插件，用于统计已提交文本中的 Unicode 扩展字素簇，并将每小时的统计总数保存到本地 SQLite 数据库。

插件会在 Fcitx 状态栏添加一个“输入计数器”按钮。点击后打开 Qt 统计窗口，显示总计、今日、最近 24 小时和最近 7 天的输入量，以及最近 24 小时、7 天、30 天、12 个月或所有记录年份的柱状图。最近 7 天的图表按每 6 小时分组。自定义图表支持指定开始时间、结束时间，以及从小时到月的统计粒度。窗口也支持清空全部统计记录。界面提供简体中文翻译。

## 统计范围

- 统计 Fcitx 输入法发出的 `InputContextCommitString` 和 `InputContextCommitStringWithCursor` 事件。
- 统计 Fcitx 直接传递给应用的可打印按键，包括普通英文直输。
- 对每个提交事件的文本，按 UAX #29 定义的 Unicode 扩展字素簇计数，包括空白字符和标点。例如，`👨‍👩‍👧‍👦` 计为一个字素簇。
- 不统计控制快捷键和非文本按键。
- 每小时的统计总数保存在 `$XDG_DATA_HOME/fcitx5/input-counter/stats.db`，通常为 `~/.local/share/fcitx5/input-counter/stats.db`。
- 计数先缓存在内存中，每 60 秒、插件关闭时，以及点击统计按钮时写入数据库。

## 游戏输入过滤

游戏过滤默认开启。它**只过滤原始按键的补计**；`InputContextCommitString` 和 `InputContextCommitStringWithCursor` 提交的文本始终参与统计，包括中文、英文、标点和表情。插件不会吞掉或修改传给应用的按键事件。

- 根据 Fcitx 输入上下文提供的程序名，精确匹配已知游戏程序。切换到能识别程序名的非游戏应用后，即使游戏仍在后台运行，也会恢复正常计数。
- 如果程序名为空，则以匹配的 Steam 游戏进程是否正在运行为兜底条件，决定是否过滤。此兜底功能可以关闭。对于**非空但不在列表中**的程序名，不会自动当作游戏处理。
- 过滤期间，可打印按键的候选计数暂存至松开时再入账。如果松开前再次收到同键按下事件，或事件带有 Fcitx 的重复标记，则丢弃该轮长按的计数。独立的短按仍会计数，包括游戏中的移动短按。游戏里正常、有意的长按输入同样会被排除。
- 候选缓存只包含键标识、计数和单调时钟时间戳，不包含输入文本。最多缓存 64 个按住的键；某个键连续两秒没有事件后，其候选计数过期并被丢弃。失去焦点、输入上下文销毁或重置、切换输入法以及修改配置时，也会丢弃候选缓存。无法可靠提供松开事件的输入路径可能漏计；将自动重复表示为交替松开和按下事件的输入路径可能绕过过滤。

可以通过 Fcitx 插件配置界面修改设置，也可以编辑 `~/.config/fcitx5/conf/inputcounter.conf`（遵循 `$XDG_CONFIG_HOME` 设置）：

要识别程序，请让目标应用的输入框获得焦点，然后在以下命令的输出中查找 `focus:1`：

```sh
busctl --user call org.fcitx.Fcitx5 /controller org.fcitx.Fcitx.Controller1 DebugInfo
```
## 构建

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
```

构建依赖：Fcitx5Core（>= 5.1.2）、ICU、SQLite3、Qt6 DBus 和 Widgets，以及 Gettext。

## 安装

### Arch Linux（AUR）

使用 AUR 助手安装 [`fcitx5-input-counter`](https://aur.archlinux.org/packages/fcitx5-input-counter)，例如：

```sh
paru -S fcitx5-input-counter
```

### 从源码安装

插件库必须安装到 Fcitx 插件目录。常规系统安装命令如下：

```sh
sudo cmake --install build
fcitx5 -r
```
