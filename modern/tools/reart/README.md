# 墨香原创资产工具与批次 01

仅对应 `tyj1987/MoxiangReborn`。本工具产出原创技术美术样本，保留原资源、客户端默认配置、玩法、协议和数值。不是完整新地图或商业成品。

## 从仓库根目录构建

需要 Python 3.10+、CMake 3.20+、支持 C++20 的编译器。仅使用 Python 标准库及本仓库既有 C++ 解析器；不需要下载原游戏美术来生成样本。

```text
python -m unittest discover -s modern/tests/reart -v
python modern/tools/reart/build_samples.py --output modern/scratch/reart-batch01
cmake -S modern/tools/reart -B modern/scratch/reart-probe
cmake --build modern/scratch/reart-probe --config Release
```

Linux 探针为 `modern/scratch/reart-probe/reart_probe`；Visual Studio 多配置构建通常为 `modern/scratch/reart-probe/Release/reart_probe.exe`。运行 `--pack modern/scratch/reart-batch01` 检查真实原生格式。配置器不同则按实际输出路径调用，不重命名扩展名。

```text
python modern/tools/reart/make_review.py modern/scratch/reart-batch01 modern/scratch/reart-review.html
python modern/tools/reart/pack.py modern/scratch/reart-batch01 modern/scratch/reart-development.zip --development
```

在普通浏览器打开生成的审阅页，右键/拖动旋转，滚轮/双指缩放，可切换三件模型和试听声音。浏览器无 WebGL 时明确标注 CPU 三维投影；两者都是独立审阅，不是 DX11 游戏画面。音乐为合成草案，尚未通过审美或事件绑定验收。

输出目录/文件必须尚不存在，工具拒绝覆盖。再次构建使用新的目录，避免破坏已有工作。

## 明确的发布隔离

不加 `--development` 默认执行正式发布门槛，本批必须被拒绝。原因包括 `authored`、技术样本、缺少玩法绑定、来源评审待办、缺少视听/设备证据及全量覆盖未完成。不能为消除错误而伪造 `release_approved` 或随意填写数据 ID。

原生文件包括 MOD/STM/CHX/ANM/DDS/WAV；GLB 为静态姿态交换文件，骨骼蒙皮和动画保留在原生格式/JSON 源数据。作者工程米制、Y 向上，原生导出乘 100；DX11 实际目录缩放、左右手系与剔除仍需游戏内校准。不得把解析通过称为装备可用或地图完成。

## 只读审计

`audit_map10.py --repo 仓库根 --probe 探针绝对路径 --output 新报告.json` 调用当前 BIN 读取器核对地图身份、NPC/传送锚点和独立 HFL。可选 `--reference 已核验原包10.hfl` 记录原地形尺寸；缺少该输入则报告不可用，绝不静默替代。

本批数据审计确认 Map10=嵩山、Map17=蘭州。所有独立 HFL 与当前占位生成器吻合，原 Map.pak 内10.hfl 为513×513、51200×51200；原件仅作参考，不进入新原创包。详见 `modern/docs/art-direction/BATCH-01-STATUS.md`。

## 交付边界

3件独立三维设计、5骨剑穗而非完整人体骨架、4按钮状态而非全套UI、拖尾纹理而非全量技能、16秒动机而非整套配乐。7组清单资产不等于所有游戏资产。未激活新运行档，未合并或发布。
