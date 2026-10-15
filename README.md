# Embedded Framework

基于 CMake 和 ARM GNU Toolchain 的嵌入式工程，支持 `h723_mc02` 和 `f407_cboard`。

## 开发环境

安装以下工具，并将其可执行文件所在目录加入本机 `PATH`：

- CMake 3.22 或更新版本。
- Ninja。
- ARM GNU Toolchain（`arm-none-eabi-gcc`、`arm-none-eabi-g++`）。

仓库不保存个人工具链的绝对路径。更改 `PATH` 后，重新启动编辑器使其生效。
不要复制其他电脑上的 `build/`；CMake 缓存包含本机路径，应在本机重新配置。
需要个人配置时使用已被 Git 忽略的 `CMakeUserPresets.json`。

## VS Code

1. 打开仓库根目录，安装工作区推荐的 **CMake Tools** 和 **C/C++** 扩展。
2. 执行 `C/C++: Select a Configuration`，选择 `h723_mc02` 或 `f407_cboard`。
3. 直接编辑源码，使用补全、跳转和诊断；不需要 Configure 或 Build，也不需要 `build/` 目录。

IntelliSense 使用仓库内 `.vscode/c_cpp_properties.json` 的独立配置，从 `PATH` 查找
`arm-none-eabi-g++`，查询系统头文件与内置宏。所有工程路径都相对于工作区，
不依赖本机绝对路径、CMake 配置提供者或编译数据库。
两块板卡的 wrapper、HAL、CMSIS 和 ThreadX 端口分别配置，避免同名头文件混用。
`libs/`、`bsp/`、`devices/` 下新增头文件可直接参与解析，无需先加入构建。
共享代码一次按所选板卡的 Debug 配置分析。修改板卡宏或第三方包含目录时，
需要同步更新此文件；编辑器可见不代表该文件已接入实际构建。

需要构建固件时，再通过 `CMake: Select Configure Preset` 选择同名板卡并执行构建。
CMake preset 与 IntelliSense 配置独立选择，切板时需保持两者一致。
工作区已关闭打开工程时自动运行 CMake Configure。

本仓库默认使用微软 C/C++ 扩展提供语言服务和基于 `.clang-format` 的格式化。
如果本机安装了 clangd，请在此工作区禁用 clangd 扩展，避免两套语言服务同时运行。

如果已有窗口仍沿用旧配置，重新选择上述 C/C++ 配置并执行 `Developer: Reload Window`。
若仍有异常红线，执行 `C/C++: Log Diagnostics`，检查实际采用的 ARM 编译器、宏和包含目录。
真正的语法错误仍会正常显示；不要通过关闭诊断隐藏错误。

## 命令行构建

在仓库根目录运行：

```sh
cmake --preset h723_mc02
cmake --build --preset h723_mc02
```

构建 F407 时将上述两个命令中的 preset 名称改为 `f407_cboard`。
构建产物位于 `build/<preset名称>/`，编译数据库由 CMake 自动生成，供其他工具按需使用。
