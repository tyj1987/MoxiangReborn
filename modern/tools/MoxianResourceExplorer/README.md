# MoxianResourceExplorer

命令行资源浏览器 - 用现代 C++ 实现，可以读取老格式资源。

## 用法

```bash
# 列出 .bin 文件的元信息
mxh_explorer info path/to/ItemList.bin

# 解密 .bin 到原始字节（写到 .bin.dec 文件）
mxh_explorer extract path/to/MonsterList.bin -o ./out/

# 列出 .pak 中所有文件
mxh_explorer list path/to/Effect.pak

# 从 .pak 中提取单个文件
mxh_explorer extract-pak path/to/Effect.pak "Map\\Map0.bmhm" -o ./out/

# 查看 .bsad 技能区域
mxh_explorer bsad path/to/SkillArea/9x9_Blank.bsad

# 显示 .bmhm 地图头信息
mxh_explorer map path/to/Resource/Map/Map22.bmhm
```

## 构建

```powershell
cmake -S modern -B modern/build -G "Visual Studio 17 2022" -A Win32
cmake --build modern/build --config Release --target mxh_explorer
```

## 输出

- 工具位于 `modern/build/Release/mxh_explorer.exe` (或 Debug 目录)
- 帮助：`mxh_explorer --help`