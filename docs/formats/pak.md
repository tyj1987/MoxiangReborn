# `.pak` 包格式

`modern/tools/unpack_pak.py` 按偏移 4 的 `dwFileItemNum` 区分两种包头。条目之间都没有对齐填充。

## 条目

两种变体的单条布局相同，从该条起点 `cur` 开始：

| 偏移 | 内容 |
| --- | --- |
| 0 | `total`，uint32。下一条起点是 `cur + total` |
| 4 | `real`，uint32，紧跟在文件名 NUL 之后的数据字节数 |
| 8 | `name_len`，uint32，文件名字节数，不含结尾 NUL |
| 12 | `data_off`，uint32 |
| 16 | 16 字节，原版样本里是 0 |
| 32 | 文件名，`name_len` 字节 |
| 32 + name_len | 1 字节 NUL |
| 33 + name_len | `real` 字节数据 |

原版样本满足 `total == 32 + name_len + 1 + real`。客户端 `Map.pak` 第 0 条：`32 + 8 + 1 + 174888 = 174929 == total`。第 1 条因此在 `96 + 174929 = 175025 = 0x2AB31`，文件名是 `water.tif`。

原版文件名按 GBK 解码。worktree 路径仍按现有读取器的 latin-1 处理，不在这次改动里更换。

## 变体 A：worktree，92 字节包头

偏移 4 的 `dwFileItemNum` 非 0。PlayDH 的 `Map.pak` 这里是 4192。

| 偏移 | 内容 |
| --- | --- |
| 0 | `version`，uint32，现行读取器要求为 1 |
| 4 | `dwFileItemNum`，uint32，条目数 |
| 8 | `flag`，uint32 |
| 12 | 其余填到 92 字节 |

条目从偏移 92 开始，只走声明的条数。这一路按 `32 + name_len + 1 + real` 前进，不按 `total` 前进：已有样本的 `total` 会坏，跟着走会把游标带出文件。条数走完后文件里还剩字节也不报错。头损坏时沿用原来的 4 字节重同步。

## 变体 B：原版，96 字节包头

偏移 4 为 0。客户端 `D:\MX\client\Package\3DData\Map.pak` 属于这一路，真正的条目数在偏移 92，值为 `0x2C03 = 11267`。

| 偏移 | 内容 |
| --- | --- |
| 0 | `version`，uint32，值为 1 |
| 4 | `dwFileItemNum`，uint32，值为 0。这不是条目数 |
| 8 | `flag`，uint32 |
| 12 | 填 0 到偏移 92 |
| 92 | 条目数，uint32 |

条目从偏移 96 开始，按 `cur + total` 正好走完该条数。文件在最后一条结束处停下是正常结束。还没走完条数就遇到 EOF，报「EOF arrived early」，并带上条目序号、偏移和声明条数。条数走完后仍有剩余字节，报剩余字节数、停住的偏移和声明条数。两条错误要能区分开。

从偏移 92 当成第一条来读会整包错 4 字节。原版 `Map.pak` 上一次失败落在 `0x556F6`，`name_len = 230632910`，就是这个错位。

## C++ 读取器还没接变体 B

`PackFile::open()` 不能读变体 B，但也不是从 92 把原版包错读成条目。这次不改 C++。

- `modern/src/pack_file.cpp` 的 `PackFile::is_pak`（38–44 行）要求 `file_item_num > 0`。偏移 4 为 0 时直接判定不是 pak。
- `PackFile::open`（47–54 行）把文件交给 `open_buffer`。
- `open_buffer`（57–58 行）在 `is_pak` 失败时返回空指针，走不到后面的循环。
- 只有通过 `is_pak` 之后，游标才设成 `sizeof(PackFileHeader)`，也就是 92（65–67 行），并按包头里的 `file_item_num` 循环（73 行）。`modern/include/mxh/compat/pack_file.hpp` 把这个包头固定成 92 字节（50–58 行）。

因此原版 `Map.pak` 在 C++ 里是打开失败，不会走出 Python 以前那种 4 字节错位。要补的是：偏移 4 为 0 时不要因 `file_item_num > 0` 拒绝；改为读偏移 92 的条数，从 96 起按 `total` 前进，文件名按 GBK，提前 EOF 和尾部剩余字节都要明确失败。变体 A 的 92 字节起点和按布局前进保持不动。
