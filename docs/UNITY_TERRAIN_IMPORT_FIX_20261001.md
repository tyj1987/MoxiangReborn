# Map10 冷导入依赖修复

基线：`9a830a98ecc9bb26a5b4966669ef4136f232142d`，同分支增量。ROG报告按heightfield→13 DDS→terrain顺序重导可恢复64 chunks，来源与hash匹配；这是协作端证据，不是云端Unity运行结果。

## 改动

`MxhTerrainImporter`版本从1升到2，使Unity重新处理既有导入结果。此前只登记源文件依赖，却通过`LoadAssetAtPath`读取heightfield和DDS的导入产物。现在：

- `GatherDependenciesFromSourceFile`只读取descriptor/palette文本，预先声明heightfield与所有纹理产物；不在该回调调用AssetDatabase。视觉override复用palette已有纹理，未增加新纹理来源。
- `OnImportAsset`保留源文件依赖，并在加载heightfield/纹理前显式声明artifact依赖，覆盖未就绪产物的后续导入。palette/override仍按源文件追踪。
- heightfield未就绪单独报告，来源不匹配、SHA不匹配和槽位校验继续拒绝输入；不放宽校验，不产生占位地形。
- 提供菜单`Moxiang/Reimport Map10 Terrain Dependencies`和`Moxiang.Editor.MxhTerrainImporter.ReimportMap10Dependencies`入口：按收集结果同步重导依赖，再导入terrain并检查GameObject。单次有限流程，失败抛错；无sleep、自动重试或Refresh循环。

使用Unity支持的[预先收集artifact依赖回调](https://docs.unity3d.com/6000.6/Documentation/ScriptReference/AssetImporters.ScriptedImporter.GatherDependenciesFromSourceFile.html)和[DependsOnArtifact](https://docs.unity3d.com/6000.6/Documentation/ScriptReference/AssetImporters.AssetImportContext.DependsOnArtifact.html)。前者帮助安排初次导入次序，后者记录未导入产物并使依赖方在产物导入后重新导入。

## 验证与限制

新增`TerrainImportDependencyTests`五个EditMode用例：原创1格地形/4×4 DDS首次统一Refresh、显式单次重导、错误sourceId拒绝、错误纹理hash拒绝、实际Map10依赖集合为1 heightfield+13 DDS。夹具写入唯一临时Assets目录并清理，不复制或改写游戏资产。负例仅在预期失败期间抑制Unity版本相关日志，并明确要求重导抛错、依赖成功导入且terrain无法加载。

云端没有Unity Editor，以上C#测试及实际冷导入尚未执行。云端已运行：

```sh
python3 scripts/check-project-governance.py
python3 -m unittest discover -s modern/tests/unit/tools -p 'test_unity*.py'
git diff --check
```

治理/diff通过，Python18/18通过。独立读取实际Map10源文件确认14个唯一依赖路径、palette/heightfield的sourceId一致、13个DDS与height sidecar的SHA全部匹配。此静态结果不代表Unity导入通过。

## ROG复测

1. 在隔离的新Library工作副本上让Unity正常首次导入，先不要手工顺序重导；运行`Moxiang.Tests.TerrainImportDependencyTests`、`TerrainPaletteTests.ActualMap10PaletteAndChunkAssetsExist`与`ImportedMap10Tests`。
2. 验证Map10为GameObject而非DefaultAsset、64个mesh chunks、13个纹理及非空材质引用；记录完整SHA与导入日志。
3. 若旧Library仍保留失败结果，运行上述菜单/executeMethod一次，保存失败或恢复证据。手动恢复成功不能替代第一步的冷导入验收。

本批未修改地形、palette、DDS、怪物资产或来源校验。ROG的228怪物、6 kind、首击Hit及受击动画结果不等同于击杀、掉落、拾取、存档重登完成；这些仍待后续真实验收探针。
