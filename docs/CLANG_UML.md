# clang-uml 使用指南

本项目使用 clang-uml 从 C++ 源码生成类图和包图。配置文件是项目根目录下的
`.clang-uml`，编译数据库位于 `build/clang-uml/compile_commands.json`，图表输出到
`docs/diagrams/`。这个构建树与 Visual Studio 使用的 `build/windows-msvc2026` 完全独立。

## 1. 安装要求

需要以下工具：

- CMake 3.25 或更高版本
- Ninja
- Visual Studio 2026 C++ 工具链
- clang-uml
- Java 17 或更高版本（用于 PlantUML）
- PlantUML（用于输出 PNG/SVG）

检查命令：

```powershell
cmake --version
ninja --version
clang-uml --version
```

Windows 上请从 [clang-uml Releases](https://github.com/bkryza/clang-uml/releases) 下载最新的 `win64.exe` 安装程序。安装后重新打开终端，
确保 `clang-uml.exe` 所在目录已经加入 `PATH`。

## 2. 生成编译数据库

先打开 **Developer PowerShell for VS 2026**，进入项目根目录：

```powershell
Set-Location D:/project/render
cmake --preset clang-uml
```

也可以在普通 PowerShell 中先加载项目指定的 VS 2026 开发环境：

```powershell
& "D:/tool/visual studio/Common7/Tools/Launch-VsDevShell.ps1" `
    -Arch amd64 `
    -HostArch amd64 `
    -SkipAutomaticLocation

Set-Location D:/project/render
cmake --preset clang-uml
```

成功后应生成：

```text
build/clang-uml/compile_commands.json
```

这里只需要执行 CMake 配置，不需要编译项目。添加、删除源文件或者修改编译选项后，应重新执行
`cmake --preset clang-uml`，让编译数据库保持最新。

如需完全重新生成：

```powershell
cmake --preset clang-uml --fresh
```

## 3. 生成 UML

在项目根目录生成全部图表：

```powershell
clang-uml
```

查看配置中定义的图表：

```powershell
clang-uml -l
```

只生成一张图：

```powershell
clang-uml -n renderlab_classes
clang-uml -n renderlab_packages
clang-uml -n renderer_classes
```

默认输出 PlantUML 文件：

```text
docs/diagrams/renderlab_classes.puml
docs/diagrams/renderlab_packages.puml
docs/diagrams/renderer_classes.puml
```

如果希望输出 Mermaid：

```powershell
clang-uml -g mermaid
```

## 4. 当前图表的用途

### `renderlab_classes`

分析 `src` 下全部 C++ 翻译单元，显示 `renderlab` 命名空间中的类、结构体、继承和成员关系。
随着项目增长，这张图可能会变得较大。

### `renderlab_packages`

按源码目录生成包图，用于观察 `app`、`assets`、`core`、`editor`、`renderer` 和 `scene`
之间的依赖。项目代码主要位于同一个 `renderlab` 命名空间，因此这里使用目录而不是命名空间作为包。

### `renderer_classes`

只分析 `src/renderer`，用于查看 `RenderViewport`、Surface、RHI 接口和 OpenGL Backend
之间的关系。

## 5. 修改配置

`.clang-uml` 中的路径相对于配置文件所在的项目根目录。

添加新的图表时，在 `diagrams` 下增加条目。例如生成场景类图：

```yaml
scene_classes:
  type: class
  glob:
    - src/scene/**/*.cpp
  using_namespace:
    - renderlab
  include:
    namespaces:
      - renderlab
```

检查 clang-uml 实际读取到的配置：

```powershell
clang-uml --dump-config
```

## 6. 常见问题

### 找不到 `clang-uml`

重新打开 PowerShell，并确认：

```powershell
Get-Command clang-uml
```

如果仍然找不到，将 clang-uml 的安装目录加入用户 `PATH`。

### 找不到 `compile_commands.json`

确认先执行过：

```powershell
cmake --preset clang-uml
Test-Path build/clang-uml/compile_commands.json
```

### CMake 找不到编译器

Ninja 生成器需要当前终端已经加载 MSVC 开发环境。请使用 Developer PowerShell，或者先运行
`Launch-VsDevShell.ps1`，再执行 CMake preset。

### 没有匹配到翻译单元

clang-uml 的 `glob` 必须能匹配 `compile_commands.json` 中的源文件。先更新编译数据库，然后使用详细日志检查：

```powershell
clang-uml -vvv -n renderer_classes
```

### 转换为 PNG 或 SVG

本机已经安装 Java 和 PlantUML。重新打开终端后，可以批量生成 PNG：

```powershell
plantuml -charset UTF-8 -tpng "docs/diagrams/*.puml"
```

只转换一张图：

```powershell
plantuml -charset UTF-8 -tpng docs/diagrams/renderer_classes.puml
```

输出 SVG：

```powershell
plantuml -charset UTF-8 -tsvg "docs/diagrams/*.puml"
```

PNG/SVG 会生成在 `.puml` 文件所在的 `docs/diagrams` 目录。安装的命令包装器默认将
`PLANTUML_LIMIT_SIZE` 提高到 8192，避免较大的类图被 4096 像素上限限制。

## 7. 参考资料

- [clang-uml 官方文档](https://clang-uml.github.io/)
- [配置文件参考](https://clang-uml.github.io/md_docs_2configuration__file.html)
- [GitHub 项目与 Releases](https://github.com/bkryza/clang-uml)
