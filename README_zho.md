<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-PNP-HEAD banner" width="100%">
</p>

# 🛠️ EH3D-NODE-PNP-HEAD

<p align="center"><a href="README.md">🇺🇸 English</a> | <a href="README_spa.md">🇪🇸 Español</a> | <a href="README_fra.md">🇫🇷 Français</a> | <a href="README_ita.md">🇮🇹 Italiano</a> | <a href="README_deu.md">🇩🇪 Deutsch</a> | 🇨🇳 <b>中文</b> | <a href="README_jpn.md">🇯🇵 日本語</a> | <a href="README_por.md">🇵🇹 Português</a></p>

### EH3D 贴片机的贴装头节点：吸嘴、真空、吹气、照明和联锁，已编写并用模拟器测试

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20Python-blue.svg" alt="C++17 and Python">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Tests-98-2EA043.svg" alt="Tests">
  <img src="https://img.shields.io/badge/Version-0.0.3-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-functional-4CAF50.svg" alt="Maturity">
</p>

---

**诚实性核对 - 今天真正能运行的内容：** 这是一个可在模拟中运行的节点：逻辑真实且经过测试，没有固件，也没有硬件。节点的规则在计算机上对模拟器运行（98 个测试通过），但这里的内容从未编译成固件、烧录，也从未连接到泵、阀门、压力传感器或轴。

---

> **安全（safety）与信息安全现状：** 该节点不是安全系统。它的联锁和安全状态只是机器及其控制器的急停、限位和联锁之后的第二道防线，绝不取代它们。

## 📋 速览

| | |
|---|---|
| 版本 | `0.0.3` |
| 成熟度 | 可用 |
| 测试 | 98 |
| 技术栈 | C++17 (host tests), Python 3.10+ |
| 真实硬件 | 尚未在硬件上运行 |

**跳转到：** [概述](#-概述) · [架构](#-架构) · [快速开始](#-快速开始) · [测试](#-测试) · [仓库结构](#-仓库结构) · [相关项目](#-相关项目)

## 🚦 项目状态

每一行是本仓库的一个部分，灯号表示它今天做到了哪一步。内容依据仓库能够展示的证据（测试、构建、运行），而不是计划。

🟢 已完成并验证 · 🟡 可用，有已知限制 · 🟠 部分完成或尚未验证 · 🔴 未完成或不可用

| 领域 | 状态 | 在这里的含义 |
|---|---|---|
| 逻辑与模拟器 | 🟡 | 节点的规则对固定种子的模拟器运行；98 个测试在计算机上通过。 |
| 网络契约 | 🟡 | 节点的用语写在 docs/CONTRACT.md 中，页面与应答不一致时测试会失败；服务器的插件尚未使用该节点。 |
| 固件 | 🟠 | 电路板的固件尚未编写：只导入了共享基础中的简单辅助代码。 |
| 真实硬件 | 🔴 | 泵、阀门、压力传感器或轴上尚未运行任何东西。 |

## 🎯 概述

* **一个节点，一个贴装头：** 用每个吸嘴的真空吸取和释放元件，区分丢失的元件与从未吸到的元件，在联锁下移动 Z 轴和旋转，并在失去服务器时自行进入安全状态。
* **两种真空检测方式，加一个轴联锁：** 每个吸嘴在 `head.cfg` 中选择压力传感器（滤波、校准、带回差）或真空开关（去抖），并对元件丢失、未达到真空、传感器超范围或冻结给出报警；贴装头还会说明所有吸嘴何时已升到安全高度、机器可以在 X 和 Y 方向移动，并在此期间拒绝降低吸嘴。

## 🧩 在 EH3D 中的角色

贴片程序规划作业；连接真实机器后，该节点将在一个吸嘴上执行吸取和释放。目前它只以逻辑和模拟器的形式存在。

## 🏛️ 架构

<p align="center">
  <img src="images/architecture.svg" alt="EH3D-NODE-PNP-HEAD 的架构：谁在使用、包含什么、依赖什么" width="100%">
</p>

本仓库各部分如何协作：谁在使用它、它包含什么、它依赖什么。图片由 `docs/architecture.json` 绘制，并与代码核对。

## 🔒 安全与防护

该节点的安全规则；展示每条规则的测试已在 docs/REQUIREMENTS.md 中列出：

* 没有令牌的节点会拒绝一切移动或更改任何东西的请求；令牌从不显示、回写或记入日志。
* 听不到服务器时，节点自行进入安全状态，在有人确认之前不移动任何东西。命令等待期间（长时间移动或长时间吸取），节点继续接收请求：服务器必须持续询问，中途失去链路会使移动停止。这段时间有多长，节点无需询问即会告知：`GET /api/status` 中的 `link_lost_ms`，即其设置中的值。
* 每个限值都是带保守默认值的设置，错误的值绝不会放宽限值：它会被拒绝，默认值保留。

## 🔌 接口

服务器、程序和手机通过 HTTP 与节点通信，方式与套件中的其他节点相同（JSON 和 Bearer 令牌）。页面 [docs/CONTRACT.md](docs/CONTRACT.md) 就是契约，测试使它与应答保持一致。

```text
GET  /api/status                         the head and all its nozzles (always answered)
GET  /api/nozzle?n=N                     one nozzle
POST /api/nozzle/pick?n=N                open the valve; answer when the vacuum says if a part is held
POST /api/nozzle/place?n=N               release the part with a puff (only low enough)
POST /api/nozzle/blow?n=N                a puff alone (not while a part is held)
POST /api/z?n=N&mm=H                     move the nozzle to the height H
POST /api/rotate?n=N&deg=D               turn the nozzle (only high enough)
POST /api/light?up=P&down=Q              the two lights, in percent
POST /api/stop                           stop every move (the vacuum is not touched)
POST /api/ack[?n=N]                      clear a fault that is gone (or the safe state, without n)
POST /api/vacuum/calibrate?n=N&kind=zero|scale[&kpa=K]   if the settings allow it
POST /api/xy?moving=1|0                    the machine asks to move in X and Y (only with every nozzle raised) or says it stopped
```

## ⚙️ 配置

所有调校的数值都在 `head.cfg` 中；`head.cfg.example` 列出全部数值及其默认值和范围。超出范围的值或无人认识的键会被报告，默认值保留。

## 🚀 快速开始

需要 C++17 编译器、CMake 3.16 或更高版本以及 Python 3.10 或更高版本；无需安装其他东西。

```bash
cmake -S tests -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
python -I tools/ci_validate.py
python -I -m unittest discover -s tools -p "test_*.py"
```

## 🧪 测试

98 个测试：固定种子的 C++ 主机测试（运行模拟器和网络用语，并将契约页面与应答对比）以及仓库的 Python 测试。CI 还会用 AddressSanitizer 和 UBSan 运行 C++ 测试。

## 🏗️ 构建与发布

版本记录在 `eh3d.project.json` 中，每个版本在 [CHANGELOG.md](CHANGELOG.md) 中都有条目。目前还没有固件构建；固件出现后只会在 GitHub Actions 中构建。软件采用 GPL-3.0-or-later；节点的机械部件计划采用 CC-BY-SA，电子部分采用 CERN-OHL-S（见 [docs/HARDWARE.md](docs/HARDWARE.md)）。

## 📂 仓库结构

<p align="center">
  <img src="images/repository-structure.svg" alt="EH3D-NODE-PNP-HEAD 的仓库结构" width="100%">
</p>

<details>
<summary>目录树的文本版</summary>

```text
EH3D-NODE-PNP-HEAD/
├── core/
│   ├── head_config.hpp cfg_text.hpp                                                                                                         设置文件及其安全默认值
│   ├── head.hpp                                                                                                                             吸嘴、气泵和轴；模拟或数字真空检测；X/Y 联锁
│   ├── head_node.hpp version.hpp                                                                                                            节点在网络上的用语
│   ├── head_sim.hpp                                                                                                                         电脑上的贴装头：气泵、阀、元件、压力传感器和开关
│   └── json.hpp auth.hpp semver.hpp backoff.hpp task_health.hpp stored_load.hpp memory_budget.hpp net_text.hpp node_id.hpp civil_time.hpp   各节点共享的简单辅助代码，由工具保持一致
├── tests/
│   ├── check.hpp CMakeLists.txt                                                                                                             测试框架及其构建方式
│   ├── test_config.cpp                                                                                                                      设置文件、滤波器和校准
│   ├── test_head.cpp                                                                                                                        模拟泵和传感器上的贴装头：吸取、丢失、联锁、安全状态
│   ├── test_sensor.cpp                                                                                                                      模拟与数字真空检测、报警和 X/Y 联锁
│   └── test_node.cpp                                                                                                                        网络用语、令牌、安全状态和契约页面
├── docs/
│   ├── CONTRACT.md                                                                                                                          节点在网络（和链路）上所说的内容
│   ├── REQUIREMENTS.md                                                                                                                      每条需求及证明它的测试
│   ├── SCOPE.md STATUS.md NOT_YET.md                                                                                                        用途、进展和待办事项
│   ├── HARDWARE.md                                                                                                                          初步零件、台架测试和许可
│   └── architecture.json structure.json                                                                                                     两张图的数据
├── tools/                                                                                                                                   仓库检查、其测试以及图片工具
├── images/                                                                                                                                  横幅图以及架构和仓库结构图片
├── .github/                                                                                                                                 CI 与仪表板工作流、Issue 和 PR 模板
├── eh3d.project.json                                                                                                                        清单：版本、角色、成熟度和构建命令
├── head.cfg.example                                                                                                                         全部设置及其默认值和范围
├── VERSION                                                                                                                                  仓库的版本
├── CHANGELOG.md                                                                                                                             每个版本的变更
├── AGENTS.md                                                                                                                                修改仓库者须遵守的规则
├── test.sh test.bat                                                                                                                         运行主机测试
├── SECURITY.md SUPPORT.md                                                                                                                   如何报告漏洞及在哪里提问
└── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE                                                                                               参与贡献、行为准则和许可证
```

</details>

## 🗺️ 路线图与已知限制

**目前已知的限制**

* 固件、泵／阀门输出、压力读取和轴驱动都尚未编写；模拟的真空和噪声是虚构的。机器的控制器如何得知吸嘴已抬起，尚未决定。

**下一步**

* 基于共享基础的 ESP-IDF 固件、泵和阀门的输出以及压力传感器的读取，然后对一个吸嘴做首次台架测试。

## 🌍 语言

README 和文本提供英语、西班牙语、法语、意大利语、德语、汉语、日语和欧洲葡萄牙语版本。

## 🔗 相关项目

* [EH3D-PHOENIX-PNP](https://github.com/JuanenRac/EH3D-PHOENIX-PNP)：规划作业并描述该节点的贴片程序
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER)：其贴片机插件将位于该节点背后的服务器
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON)：该节点从中获取简单辅助代码的固件基础
* [EH3D-NODE-PNP-FEEDER](https://github.com/JuanenRac/EH3D-NODE-PNP-FEEDER)：输送元件的供料器节点

## 📚 文档

* [套件的约定](https://github.com/JuanenRac/EH3D-DOCS/blob/main/docs/CONVENTIONS.md)
* [节点的契约](docs/CONTRACT.md)
* [需求及各自的测试](docs/REQUIREMENTS.md)
* [范围](docs/SCOPE.md)、[状态](docs/STATUS.md)和[尚未完成的事项](docs/NOT_YET.md)
* [硬件：初步零件、台架测试与许可](docs/HARDWARE.md)
* [变更日志](CHANGELOG.md)

## 🤝 参与贡献与社区

* 提交 pull request 之前运行 `python -I tools/ci_validate.py` 和基础部分的测试：必须全部通过。见 [CONTRIBUTING.md](CONTRIBUTING.md) 和[行为准则](CODE_OF_CONDUCT.md)。
* README 有八种语言：修改文字时要同步修改全部八个文件，两张图片由 `tools/make_architecture.py` 和 `tools/make_structure.py` 根据 `docs/architecture.json` 和 `docs/structure.json` 绘制。
* 请私下报告漏洞：[SECURITY.md](SECURITY.md)。问题、想法和报告：electrohobby3d@gmail.com（[SUPPORT.md](SUPPORT.md)）。

## 👤 作者

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 许可证

软件：GPL-3.0-or-later。见 [LICENSE](LICENSE)。硬件：机械部件计划采用 CC-BY-SA，电子部分采用 CERN-OHL-S（见 [docs/HARDWARE.md](docs/HARDWARE.md)）。
