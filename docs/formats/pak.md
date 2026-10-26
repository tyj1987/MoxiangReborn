# `.pak` 包格式

`modern/tools/unpack_pak.py` 和 `mxh::compat::PackFile` 按偏移 4 的 `dwFileItemNum` 区分两种包头。条目之间都没有对齐填充。

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

## C++ 读取器

`PackFile::open()` / `open_buffer()`（`modern/src/pack_file.cpp`）跟 Python 用同一条规则。

偏移 4 非 0：92 字节包头那条循环保持原样。从偏移 92 开始，按 `32 + name_len + 1 + real` 前进，不按 `total` 前进。这一路头损坏时仍然提前结束并返回已经读到的条目，不改成失败关闭。

偏移 4 为 0：96 字节包头。

- 文件短于 96 字节就失败。这是截断的包头，不会退回 92 字节循环，也不会从偏移 92 把条目读错 4 字节。
- 条目数是偏移 92 的 uint32。读之前先检查边界。这个数为 0 时失败。因此版本号为 1、其余为 0 的 96 字节假包头仍然打不开，`FilesystemFileStorage` 和 `EffectCatalog` 现有的跳过测试不用改。
- 条目从偏移 96 开始。每条先检查剩余字节，再读 32 字节条目头。`total` 必须等于 `32 + name_len + 1 + real`，下一条在 `cur + total`。
- 声明条数即使按最小条目 33 字节也装不进包头之后的空间时失败。错误里带上这个条数。原版 `Map.pak` 的 `0x2C03 = 11267` 如果落在一个只有包头的文件上，走的就是这条。
- 条目头或正文越过 EOF、文件名后面不是 NUL、条数走完后还有剩余字节，都失败。不返回已经读到的半截条目，也不从 EOF 之后读。
- 失败返回空指针。调用方可以再传 `std::string* error` 拿原因；不传则行为和以前一样，只看空指针。
- 文件名保留盘上的字节。原版样本是 GBK，这里不转成 UTF-8。`PackFileHeader` 仍然是 92 字节，偏移 92 的条数不塞进这个结构体。

`file_count()` 在变体 A 仍是偏移 4。变体 B 打开成功时等于偏移 92 的条数。`header().file_item_num` 始终是盘上的偏移 4，变体 B 为 0。

单测在 `modern/tests/unit/pack_file_header_test.cpp`，目标 `mxh_pack_file_header_tests`。用合成包覆盖 92 字节包头、96 字节包头、截断包头、条目越过 EOF、条数大于文件能装下的量。合成样本自己的条目偏移写在测试里，不是原版 `Map.pak` 里测得的 `water.tif` 偏移 `0x2AB31`。不读取、不提交原版 `Map.pak`。

打不开时的日志不变：

- `FilesystemFileStorage::Initialize`（`modern/src/render/filesystem_file_storage.cpp`）对 `PackFile::open` 失败的 `.pak` 打 `MLOG_WARN`，写明路径和 `unsupported header / not a pak`，然后跳过，继续扫目录。
- `EffectCatalog::load`（`modern/src/game/effect_catalog.cpp`）失败返回，不是跳过，同样打 `MLOG_WARN`。`error` 仍是 `Effect.pak is not a valid PackFile`，函数仍返回 false。

## 输出目录

原资源目录只读。受保护的范围有两层，都会先解析成真实路径（跟随符号链接，折叠 `..`；Windows 上不区分大小写）：

1. 原版根目录。Windows 上默认是 `D:\MX`。`--original-root` 优先于环境变量 `MXH_ORIGINAL_ROOT`，环境变量再优先于这个默认值。非 Windows 没有默认根目录，只在显式设置时生效。解析后的输出若是这个根目录或其子目录，就拒绝。只是名字前缀相同不算，例如 `D:\MX2` 不在 `D:\MX` 里，`D:\MXtmp_x` 也不在里面。
2. pak 自己所在的目录及其子目录。pak 不在原版根目录里时，这一层仍然生效。

拒绝时进程退出码为 1，stderr 写明原因，并且不创建任何目录或文件。`--dry-run` 指向这两处时同样拒绝。

未指定输出目录时，按顺序用第一个解析后不在 pak 目录或原版根目录里的位置：

- 系统临时目录下的 `moxiang-unpack/<包名>`
- Windows 上接着是 `%LOCALAPPDATA%\moxiang-unpack\<包名>`。没有 `LOCALAPPDATA` 时才用当前工作目录下的同名文件夹
- 其它系统接着是 `~/.cache/moxiang-unpack/<包名>`，然后才是当前工作目录

候选落在只读范围内就跳过，所以 `%TEMP%` 或 `%LOCALAPPDATA%` 在原版根目录内时不会被用上。全部不行就拒绝，不会在 pak 旁边建 `<包名>_unpacked`。
