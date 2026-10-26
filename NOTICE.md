# NOTICE — 版权与逆向工程声明

## 项目性质

**Moxiang-Reborn**（墨香重生）是一个开源研究项目，目标是在现代 Windows
环境中 1:1 复现 2003–2010 韩国 MMORPG《墨香》的渲染管线、UI 布局、玩法
数值和网络协议。本仓库**仅包含现代 C++17/20 重写代码**，以及用于 1:1
对照测试的**老版资源字节基线**（85 个 `.tif` 像素 SHA-256 + 132 个
`InterfaceScript/*.bin` 二进制格式 baseline）。

## 资源来源

PlayDH 资源树（`modern/data/PlayDH/`，约 5135 个文件）**已提交到本仓库**，
作为 1:1 对照的 canonical 资源基线：

- 资源归原版权方所有，本项目尊重版权，仅用于研究与对照测试
- Map / Character / Effect / monster 四个超过 100 MB 的 `.pak` 走 Git LFS，
  其余文件为普通 Git blob（见 `.gitattributes`）；文件清单与哈希见
  `reference/manifests/playdh-current.sha256.json`
- PlayDH 缺失、但原版客户端 Map.pak 中存在的条目，按已批准的方案打成
  一个补充包 `modern/data/PlayDH/zz_supplement.pak`（原版 96 字节包头格式，
  条目字节原样拷贝，不含解码表或密钥）。条目名单见
  `reference/manifests/playdh-supplemental-fill.json`，由
  `modern/tools/pak_writer.py` 可复现地生成。小于 50 MB 时作为普通 blob
  提交，否则按 `.gitattributes` 走 LFS
- 仓库不包含原版客户端（`D:\MX`）的其他资源、密钥或解码后的地图文本

## 代码协议

- **本仓库代码**：MIT License（见 `LICENSE`）
- **可自由**：阅读、修改、私下使用、二次开发
- **不可商用**：将代码 + 资源的组合作为商业产品分发属侵权，本项目
  反对任何形式的商业化利用

## 免责

本项目仅供学习、研究、私下怀旧使用。**任何因使用本项目代码或资源而
产生的法律、商业或民事纠纷，由使用者自行承担。** 项目贡献者不为
任何第三方使用本项目造成的后果负责。

## 致谢

- 2003–2010 年原《墨香》研发团队（韩国 Yedang Online / Joyon）
- 早期逆向分析社区对老版 .bin / .pak 格式的解包贡献
- 现代 C++ 社区对 DX11 / Asio / SQLite / nlohmann 项目的开源贡献
