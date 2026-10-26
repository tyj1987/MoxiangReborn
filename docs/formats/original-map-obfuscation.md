# 原版 .map 混淆格式说明

## 封装布局
- 字节 0-3：LE uint32，等于该条目总长度。
- 字节 4-23：20 字节头部。
- 字节 24 起：`plain[i] = orig[24+i] XOR K[i % 8]`，K 为每文件 8 字节密钥。
- 解码后明文以 `GX_` 开头，长度 = 原文件长度 - 24。

## 密钥 K 的来源
- 仅由头部第 2 字节（文件偏移 6）决定；头部第 3 字节与之相同；其余 18 个头部字节不影响 K。
- `K[0] = 该字节 - 4 (mod 256)`。
- 该字节取值相同 => K 完全相同。
- 验证：58 对内容相同的原版/PlayDH 文件，覆盖 48 个不同取值，全部吻合。
- K 看起来是从同一条共享密钥流中、按该字节选取的 8 字节窗口；完整生成算法尚未还原（待办，M5）。

## 识别规则
- 前 4 字节 LE uint32 == 条目长度，且文件不以 `GX_` 开头 => 原版混淆格式。

## 验证结果
- 原版 10.map 解码后与 PlayDH 10.map 逐字节一致。

## 本地解码插件钩子（resource_reference_gate）
仓库只提供可插拔钩子和合成测试，不含解码器、密钥推导、密钥字节或密钥表。结论可以入库，解码能力不入库。

- 加载方式只有两种：`derive --local-decoder <仓库外的 .py>` 或环境变量 `MXH_LOCAL_MAP_DECODER`。没有默认搜索路径；位于仓库目录树内的路径一律拒绝。CI 不传这两者；`check` 从不加载插件。
- 插件契约：模块提供 `DECODER_VERSION: str` 与 `decode_map(name: str, data: bytes) -> bytes | None`。返回明文 map 字节，或 `None`（例如没有密钥）。derive 把明文交给现有的 `parse_map_references` 与 `.hfl` / `.stm` 后续解析，与明文 map 完全相同。
- 清单标记：
  - 来自解码 map 的每条引用（含其 `.hfl` / `.stm` 子引用）带 `derivedBy: "local-decoder"`。
  - `derivation.localDecoder = {version, sourcePakSha256, decodedMaps: [{name, sha256}]}`。`sourcePakSha256` 是 `--pak` 文件的 SHA-256，`decodedMaps[].sha256` 是原版（混淆）.map 字节的 SHA-256。
  - 插件返回 `None` 的 map 留在 unparsed，reason 为 `obfuscated-map-key-unavailable`（等待 M5 密钥工作）。未用插件时仍为 `obfuscated-map`。解码后仍无节标记的记为 `local-decoder output did not contain HEIGHT_FIELD, STATIC_MODEL or GX_OBJECT`。
  - 清单不得写入明文片段、密钥字节、头部字节或任何可推出密钥的字段。
- `check`：
  - 清单没有 `localDecoder` 时输出与以前逐字相同。
  - 有 `localDecoder` 时另起一行 `local-decoder=N state=...`，报告 JSON 增加 `counts.localDecoder` 与 `localDecoder`。
  - 未给 `--pak`（CI）：state=unverified，仅列出，不失败。
  - 给了 `--pak` 且 SHA-256 一致：state=verified。
  - 给了 `--pak` 但 SHA-256 不一致：state=stale，这些结论改记为 no conclusion（not-run，stale）。
