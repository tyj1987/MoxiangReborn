# 资源来源与许可台账

核查：2026-10-09。选型顺序为已有原创成果、合适的免费商用资源、补充原创。原版 `playdh-current` 继续按原始字节使用；本页外部候选只面向 Unity，不能修饰为原版 1:1 完成。

## 已有成果

- 武馆、刀剑枪及 Blender/导出/烘焙输入继续保留。武馆 PBR 清单记录原 `.blend`、FBX、贴图、脚本 SHA 和导出约定；清单存在不等于质量或商用验收通过。
- `original-assets-foundation` 的四个独有提交已在恢复 bundle 保全。`3dc89bc7` 的工具本轮重新生成剑、剑鞘、山门、UI、特效和音频样本：7 组、35 项哈希文件。工具与8份可编辑输入已由 `ad0e096b` 独立落位 Unity 的 `artsource/ProceduralStudies/moxiang-reart-v1`，原manifest作为来源快照保存；22/22原始Git blob核对通过。后续 `7eda291d` 接入当前GitLab单测、更新工具说明并退役旧CI入口，其原字节仍在Git历史。样本的 `rights.json` 尚待补齐作者、权利人和许可凭证，目前仅私有保全/审查，不进入Player或公开镜像；既有原资源和武馆/刀剑枪未被覆盖。
- 原游戏资源继续遵守仓库 NOTICE；代码或新增素材的许可不扩大原游戏资源权利。

## 已核查的公开来源

| 来源 | 本次许可核查 | 裸资源公开再分发 | 使用范围 |
| --- | --- | --- | --- |
| [Poly Haven](https://polyhaven.com/license) | 资产 CC0，允许商用，无强制署名 | 允许资产再分发；网站文字、Logo、示例渲染另论 | 通用 PBR 材质、环境 |
| [ambientCG](https://docs.ambientcg.com/license/) | 下载资产和材质预览为 CC0 1.0，允许商用，无强制署名 | 允许复制、修改和分发原文件 | 地表、石材、木材 |
| [Kenney](https://kenney.nl/support) | Asset pages 中游戏资源为 CC0，逐包核查附带许可 | 按 CC0；不使用其 Logo 冒充官方产品 | 通用 UI 与音效 |
| [Quaternius](https://quaternius.com/license.html) | 当前 QAL v1.0，2026-08-28 更新；免费商用，不强制署名 | 禁止以资产形式重新打包/分发；产品内使用与裸资源分发分开 | 仅保留备选；不能默认上传公开镜像 |

Quaternius 旧包须查当时取得版本与包内许可证；其现行条款说明不追溯已在旧许可下取得的资产，不能用当前站点摘要抹掉取得记录。

## 有明确出处的候选

| 候选 | 出处/版本 | 可考虑的用途 | 尚未完成 |
| --- | --- | --- | --- |
| [Weathered Planks](https://polyhaven.com/a/weathered_planks) | Dario Barresi / Dimitrios Savva，CC0；页面无固定语义版本 | 武馆外围旧木构件，按 2 米材质尺度评估 | 包下载、SHA、材质适配、画面与性能验收 |
| [Interface Sounds](https://kenney.nl/assets/interface-sounds) | Kenney，1.0（2020），100个OGG，包内CC0已核对 | Unity按钮/确认反馈候选；已下载到独立审查目录 | canonical包准入、响度/风格试听及事件绑定 |
| [Impact Sounds](https://kenney.nl/assets/impact-sounds) | Kenney，1.0（2019），130 个音效，CC0 | 脚步和通用碰撞备选 | 包内许可、SHA、风格试听及事件绑定 |

Interface Sounds原ZIP已从官方精确下载地址取得：834536字节，SHA-256 `f2193d072726d6758a5f7871b2dcc54dcce0d5c35c6f0a62f92549b327c81232`。包内103个文件逐项读取/哈希核对，其中100个OGG及1份许可；许可、来源、版本、HTTP元数据、完整文件清单和零修改记录保存在本轮`asset-candidates/kenney-interface-sounds-1.0/candidate-receipt.json`。CC0允许商用和资源再分发，无强制署名；Kenney Logo不包含在授权用途内。原ZIP仅保存在审查目录，未覆盖现有原创或原版资源，未导入Unity、未试听，也未通过canonical包准入或Player验收。

其余条目仍是来源候选，不能填造包哈希或宣称已采用。ambientCG的整体许可已核查，具体PavingStones036页面本轮无法访问，暂不计作已验证素材包。当前没有使用付费API。

采用时逐包锁定原下载地址、作者、取得日期/版本、许可正文、SHA-256、文件清单、修改记录、署名要求和允许分发的范围。原包、Unity 派生文件和原创源文件分开；通过包完整性后还需 Editor/Player 导入、视觉/音频及性能验收。重制风格、人物比例、动作和玩法绑定不满足时保持候选，不能只因 CC0 就自动入场。
