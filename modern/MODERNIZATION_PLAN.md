# 澧ㄩ锛圡oxian / DarkStory锛夌幇浠ｅ寲鏀归€犺鍒?


> **椤圭洰浠ｅ彿**锛歁oxian-Reborn


> **鐩爣**锛氬湪涓嶇牬鍧忕幇鏈夋父鎴忓唴瀹广€佺帺娉曘€佽祫婧愮殑鍓嶆彁涓嬶紝鎶?2003-2010 骞寸殑浠ｇ爜浣撶郴杩佺Щ鍒?2026 骞寸殑鐜颁唬杞‖浠剁幆澧冿紝骞跺鎬ц兘銆佸彲缁存姢鎬с€佽法骞冲彴鑳藉姏鍋氭渶澶у寲鎻愬崌銆?> **鏍稿績绾︽潫**锛?*娓告垙閫昏緫銆佸崗璁€佽祫婧愩€佺帺娉曞繀椤?1:1 淇濈暀**鈥斺€旇繖鏄?澶嶅埢"鑰屼笉鏄?閲嶅仛"銆?


---





## 0. 椤圭洰鐜扮姸鎽樿锛堝垎鏋愮粨璁猴級





### 0.1 椤圭洰浣撻噺





| 缁村害 | 鏁版嵁 |


|------|------|


| 瀹㈡埛绔唬鐮?| `[Client]MH/` 绾?930 涓?.h/.cpp锛寏15 MB锛岀害 35-40 涓囪 |


| 鏈嶅姟绔唬鐮?| `[Server]*/` + `[CC]*/` 绾?412 涓?.h/.cpp锛岀害 50-70 涓囪 |


| 寮曟搸浠ｇ爜 | `4Dyuchi*` 鍏ㄥ妗?+ `[Lib]dx81` 绛?~5-10 涓囪 |


| 宸ュ叿閾?| 鎵撳寘/鍦板浘/閲嶇敓/瀵煎嚭/GM/鍘嬫祴/鏇存柊鍣?~10 涓嫭绔嬪伐绋?|


| 璧勬簮锛堝凡閮ㄧ讲锛?| 1.3 GB 瀹㈡埛绔?+ 29 MB 鏈嶅姟绔?+ ~37 寮犲湴鍥?+ 涓夊簱 MSSQL 澶囦唤 |


| 宸茬紪璇戜骇鐗?| `SWorking/` 瀹屾暣鏈嶅姟绔伐浣滅洰褰曪紝`cworking/MHClient-Connect.exe` 瀹㈡埛绔?|





### 0.2 杩涚▼鎷撴墤





```


                    鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                    鈹? Monitoring Server (MS) :30001鈹? 涓ぎ鍗忚皟


                    鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                                  鈻?                                  鈹?涓婃姤/鏌ヨ


   鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?        鈹?        鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?   鈹?Distribute :6001/400鈹傗梽鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻衡攤  Agent :7001/600    鈹?   鈹? (鐧诲綍/閫夋湇)        鈹?        鈹?        鈹? (浠ｇ悊/涓栫晫鏈?       鈹?   鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?        鈹?        鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                                  鈹?               鈻测柤  鈻测柤


                                  鈹?     鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                                  鈹?     鈹? Map #N :800N         鈹?                                  鈹?     鈹? (涓€寮犲湴鍥句竴涓繘绋?    鈹?                                  鈹?     鈹? Map #M :800M         鈹?                                  鈹?     鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                                  鈻?                          瀹㈡埛绔?MHClient.exe


```





### 0.3 瀹㈡埛绔灦鏋?


```


MHClient.exe (WinMain)


鈹溾攢 MHVerInfo.ver 鈹€ 鍚姩閰嶇疆锛堢増鏈€丏istribute 鍦板潃锛?鈹溾攢 4DyuchiNET 鈹€ 鑷爺 IOCP 缃戠粶搴擄紙瀹㈡埛绔皝瑁呬负 BaseNetwork.dll锛?鈹溾攢 HackShield + nProtect 鈹€ 闊╁浗鍙嶅鎸傦紙宸插仠鏇达級


鈹溾攢 HSEL 鈹€ 鍔犲瘑鐙?鍗忚鍔犲瘑锛堢墿鐞嗙嫍宸插仠浜э級


鈹溾攢 DirectX 8.1 鈹€ 娓叉煋锛圵in11 涓嶅啀棰勮 d3dx8.dll锛?鈹溾攢 4Dyuchi 寮曟搸 鈹€ 鑷爺 3D 灏佽锛圫S3DRenderer/Geometry/ExecutiveForMuk.dll锛?鈹溾攢 MFC + 鑷爺 cWindow 鎺т欢鏍?鈹€ UI锛圵in32 GDI锛?鈹溾攢 Miles Sound System 鈹€ 3D 闊虫晥锛堝晢涓氭巿鏉冿級


鈹斺攢 FreeImage 鈹€ 鍥剧墖瑙ｇ爜


```





### 0.4 璧勬簮鏍煎紡锛堜笉鍙牬鍧忕殑 1:1 鏁版嵁锛?


| 绫诲瀷 | 鎵╁睍鍚?| 鏁伴噺绾?| 鍔犲瘑 | 宸ュ叿 |


|------|--------|------|------|------|


| 涓氬姟琛?| `.bin` | 80+ 涓?| XOR + 浣嶇Щ | `MHFileEx` (PackingTool) |


| 璧勬簮鍖?| `.pak` | 7 涓牳蹇?| 鍚?| `4DyuchiFileStorage` |


| 鍦板浘 | `.bmhm` + `.ttb` | 37-207 寮?| 鍚?| `4DyuchiGXMapEditor` |


| 妯″瀷 | `.chl` / `.chx` / `.chr` / `.mon` / `.wpn` / `.hat` 绛?| 鏁扮櫨 | 鍚?| `MAXEXP / anmexp / MtlExp` (3ds Max Biped 鎻掍欢) |


| 璐村浘 | `.tga` / `.dds` / `.bmp` / `.jpg` | 鏁板崈 | 鍚?| 鈥?|


| 鎶€鑳藉尯 | `.bsad` | 50+ 妯℃澘 | 鍚?| 鈥?|


| 鏁版嵁搴?| `MHCMEMBER/MHGAME/MHLOG.bak` | 3 搴?MSSQL | 鈥?| SQL Server 2008 R2 |





**鍏抽敭鍒ゆ柇**锛?- `.bin` 鍔犲瘑绠楁硶鏋佸急锛坄data[i] -= (char)i`锛孋RC 宸茶娉ㄩ噴锛夛紝浣?*鏍煎紡蹇呴』淇濈暀**鎵嶈兘璇诲彇鑰佽祫婧?- `.pak` 鍖呮牸寮忥紙32 瀛楄妭 header锛夋槸杩愯鏃朵富鏍煎紡锛?*涓嶅彲鐮村潖**


- 妯″瀷 `.chl/.anm` 鏄?3ds Max Biped/Physique 瀵煎嚭锛?ds Max 2018+ 宸茬Щ闄ゆ妯″潡 鈫?蹇呴』**淇濈暀鎴栬浆鎹?*





---





## 1. 鐜颁唬鍖栬矾绾挎€昏





### 1.1 鎴樼暐閫夋嫨





| 鏂规 | 椋庨櫓 | 宸ヤ綔閲?| 澶嶅埢搴?| 璺ㄥ钩鍙?| 鎺ㄨ崘搴?|


|------|------|--------|--------|--------|------|


| A. 鍏ㄩ儴閲嶅啓锛圲nity + Rust锛?| 鏋侀珮 | 鏋佸ぇ閲忥紙2 浜哄勾+锛?| 闅句繚璇?1:1 | 鉁?| 鉁?|


| B. 寮曟搸鏇挎崲 + 鍗忚淇濈暀 | 涓?| 澶э紙6-12 鏈堬級 | 楂?| 鉁?| 鉁?|


| **C. 娓愯繘寮忕幇浠ｅ寲锛堥粯璁わ級** | **浣?* | **涓紙3-6 鏈堬級** | **鏋侀珮** | 閮ㄥ垎 | **鈽呪槄鈽呪槄鈽?* |


| D. 浠呮枃妗?瀹瑰櫒鍖?| 鏋佷綆 | 灏忥紙2-4 鍛級 | 100% | 鉁?| 鉁?|





**閲囩敤 C锛氭笎杩涘紡鐜颁唬鍖?*





### 1.2 娓愯繘寮忕幇浠ｅ寲鐨勪笁澶у師鍒?


1. **鍏煎鎬т紭鍏?*锛氫换浣曟ā鍧楁浛鎹㈤兘涓嶇牬鍧忕幇鏈夊崗璁€佽祫婧愩€佺帺娉?2. **鎺ュ彛绋冲畾**锛氭娊璞″眰淇濇寔涓庡師浠ｇ爜鍚屽舰锛堝 `MHFile::Open()` 鍦ㄦ柊鏃у疄鐜颁笅閮借兘璺戯級


3. **鍙洖閫€**锛氭瘡涓€姝ユ敼閫犻兘鑳藉垏鍥炲師鐗堣繍琛岋紱涓嶄竴娆℃€?鐮哥儌閲嶅仛"





##### Phase 5 当前状态摘要（2026-07-15 更新）



**已完成**：75 个 I4DyuchiGXRenderer 方法 + Device + PrimitiveDrawer + SpriteObject + TGA + MeshObject + FontObject + HeightField + EffectShaderPalette + MaterialSystem + ShadowMap + CaptureScreen + DynamicLightSystem + TriBufferPipeline + 自研 HLSL + ctest 集成 + 3D solid shader pair + DDS writer + HFieldObject (IDIHFieldObject 11 接口) + IB pool (lod/posMask) + Lock/Unlock + LoadTilePalette + ReplaceTile + UpdateAlphaMap 数据通路。Debug 测试 **142/142 PASS**。



**Phase 5.7 完成内容**：

- Dynamic Light System ✅ (CreateDynamicLight / DeleteDynamicLight, 8-slot light array, LIGHT_FLAG_DIRECTIONAL/POINT)

- DynamicLight struct ✅ (bActive, bDirectional, dwColor, v3Dir/v3Pos, fAmbient/fDiffuse, fAttenuation*)

- LightCB expanded ✅ (base directional + fog + 8 dynamic light slots for future multi-light shader)

- buildLightCB helper ✅ (populates base directional + extended light slots from pDynList)

- RenderMeshObject wired ✅ (pDynList/dwLightNum params now active, buildLightCB used)

- TriBuffer System ✅ (AllocRenderTriBuffer / EnableRenderTriBuffer / DisableRenderTriBuffer / FreeRenderTriBuffer)

- TriBuffer struct ✅ (magic validation, D3D11 VB+IB, indexed/non-indexed path)

- color_to_float4 ✅ (0xAABBGGRR → {R,G,B,A} engine color format)

- 13 new unit tests ✅ (DynamicLightDefaults, LightCBInit, ColorConversion, LightIndexDesc, TriBufferMagic)



**Phase 5.3-5.6 完成内容**：

- Effect Shader Palette ✅ (IDIEffect, wave/spheremap texture matrix)

- HeightField ✅ (LOD tile system, bilinear height sampling)

- CreateMaterialSet / CreateMaterial / DeleteMaterial ✅

- DX11 Shadow Map pipeline ✅ (beginShadowPass/endShadowPass, 2048x2048 D24S8)

- UpdateWindowSize ✅ (ResizeBuffers + render target recreate)

- CaptureScreen ✅ (back buffer → TGA type-2 writer)

- Math helpers ✅ (MatrixOrthographicLH, MatrixLookAtLH)

- WIN32_LEAN_AND_MEAN guard ✅



**Phase 5.8 完成内容**：

- RenderTriVector3 / RenderTriIvertex ✅（3D solid shader pair + 临时 D3D11 VB）

- GetD3DDevice 扩展 ✅（IID_ID3D11Device / IID_ID3D11DeviceContext）

- ConvertCompressedTexture 实装 ✅（TGA → uncompressed DDS 容器；BC 压缩需要 DirectXTex）

- DDS writer ✅（texture_loader.cpp::saveDDS, legacy DDS_HEADER + 原始 BGRA8）

- 6 new unit tests ✅（TextureLoaderTGA/DDS/AutoDetect 字节级 round-trip）



**Phase 5.9 完成内容**：
- 5.9a HFieldObject ✅（IDIHFieldObject 11 接口全实现 + 13 测试）
- 5.9b HeightField 真实 IB 池 ✅（(lod,posMask)→D3D11 dynamic IB; InitiallizeIndexBufferPool/CreateIndexBuffer/Lock/Unlcok 全实装） + LoadTilePalette/ReplaceTile ✅
- 5.9c alpha-blend 数据通路 ✅（m_alphaMap 4 字节/顶点；tile-blend PS 消费留待后续）

**已知限制 (stubs / deferred)**：



| Stub | 接口 | 状态 | 备注 |

|------|------|------|------|


| Dynamic Light (real accumulation) | multi-light shader | Phase 6 | LightCB slots 已备，shader 待升级 |



**测试统计**：

| 套件 | 数量 | 内容 |

|------|------|------|

| TgaLoader | 7 | TGA uncompressed / RLE / bottom-up-flip / RGBA32 |

| MeshGeometryTest | 4 | MESH_DESC + FACE_DESC 合约 |

| MatrixMathTest | 3 | ortho/look-at/identity matrix |

| FontObjectGlyph | 2 | GlyphEntry 字段 + CHAR_CODE_TYPE 枚举值 |

| FontObjectAtlas | 4 | row-packing：水平->回行->溢出复位->行高跟踪最大字 |

| MhFileEx | 6 | .bin XOR/位移加解密 + CRC 校验 |

| PackFile | 5 | .pak 头解析 + 实资源回环 |

| BsadArea | 4 | .bsad 技能区域 |

| ChxModelRealResource | 4 | 真实 .chx TAB 分隔文本 |

| DbAdapter | 4 | IDbAdapter 工厂 + 配置 |

| SqliteAdapter | 5 | SQLite 后端（事务/BLOB/文件持久化） |

| RealResource | 2 | MonsterList.bin + Effect.pak 真实资源 |

| HeightFieldTest | 9 | LOD tiles, bilinear interp, bounds clamping |

| EffectShaderTest | 15 | palette build, sphere/wave matrix, effect dispatch |

| MaterialDataTest | 3 | struct field defaults/settable/texture fields |

| MaterialSetTest | 2 | entry ownership |

| MaterialContractTest | 8 | MATERIAL getter methods, table contract |

| MaterialSetContractTest | 1 | multiple-entry material table |

| TcpServerTest | 9 | start/stop/connect/message/broadcast |

| TcpClientTest | 2 | connect/disconnect lifecycle |

| NetErrorTest | 1 | to_string non-null |

| DynamicLightDefaults | 3 | DynamicLight struct fields and flags |

| LightCBInit | 2 | init_light_cb base + extended slots |

| ColorConversion | 4 | color_to_float4 opaque/partial/alpha |

| LightIndexDesc | 1 | LIGHT_INDEX_DESC struct layout |

| DynamicLightConstants | 1 | MAX_DYNAMIC_LIGHTS=8 |

| TriBufferMagic | 1 | TRI_BUFFER_MAGIC constant |
| HFieldObjectDefaults | 3 | default ctor / IUnknown QI / OOR IID |
| HFieldObjectRefcount | 1 | refcount increments don't self-delete at 1 |
| HFieldObjectCreate | 2 | reject null desc / zero face count |
| HFieldObjectColor | 3 | no implicit alloc + uniform / per-vertex color |
| HFieldObjectStateSetters | 1 | DetailLevel/Distance/Mask/MustUpdate noop |
| HFieldObjectAlpha | 4 | hasAlphaMap toggle + integrated-tile storage + cleanup |
| HFieldObjectMath | 1 | index count = 6 × face count |
| HeightFieldPoolCaps | 1 | kMaxPosMasks=16 / kMaxLodSlots=8 |
| HeightFieldPoolGuards | 4 | zero-size / OOR rejected |
| HeightFieldManagerUtilities | 2 | makePoolKey (lod<<32)\|posMask |
| HeightFieldTilePalette | 1 | null input rejection |

| TextureLoaderTGA | 2 | saveTGA round-trip + empty rejection |

| TextureLoaderDDS | 3 | saveDDS magic/header/pixel/BGRA + empty rejection |

| TextureLoaderAutoDetect | 1 | unknown image format returns empty |

| **合计** | **142** | Debug 全过 |





## 2. 鍏抽敭鎶€鏈€夊瀷锛圥hase 1-7 鐨勬牳蹇冨喅绛栵級





### 2.1 缂栬瘧鍣ㄤ笌鏋勫缓





| 缁村害 | 閫夊瀷 | 鐞嗙敱 |


|------|------|------|


| 缂栬瘧鍣?| **MSVC 2022** (Windows) / **Clang 17+** (璺ㄥ钩鍙? | 鍏煎鑰佷唬鐮?+ 鐜颁唬 C++20/23 |


| C++ 鏍囧噯 | **C++17** 璧锋锛?*C++20** 妯″潡鍖?| 鍏煎 2003 浠ｇ爜 + 鐜颁唬鐗规€?|


| 鏋勫缓绯荤粺 | **CMake 3.25+** + **Ninja** | 宸ヤ笟鏍囧噯锛孖DE 鍙嬪ソ |


| 鍖呯鐞?| **vcpkg** (Windows) / **Conan** (璺ㄥ钩鍙? | 鐜颁唬 C++ 鏍囬厤 |


| 娴嬭瘯 | **GoogleTest** + **Catch2** | 鑰佷唬鐮侀噸鏋勫繀澶?|


| 闈欐€佸垎鏋?| **clang-tidy** + **PVS-Studio** | 闃叉鎶€鏈€虹疮绉?|





### 2.2 鍥惧舰娓叉煋





| 缁村害 | 閫夊瀷 | 鐞嗙敱 |


|------|------|------|


| 娓叉煋 API | **DirectX 11** (榛樿) / **DirectX 12** (鍙€? / **Vulkan** (璺ㄥ钩鍙板彲閫? | DX11 鍏煎鎬ф渶浣筹紱DX12 鎬ц兘涓婇檺楂?|


| 鐫€鑹插櫒 | HLSL 5.0+ / SPIR-V (Vulkan) | DX 鏍囧噯 |


| 绐楀彛 | **SDL3** (璺ㄥ钩鍙? 鎴栧師鐢?Win32 | SDL 璺ㄥ钩鍙?+ 杈撳叆涓€浣撳寲 |


| 鏁板 | **DirectXMath** (Windows) / **glm** (璺ㄥ钩鍙? | 鐜颁唬 SIMD 浼樺寲 |


| 璐村浘 | DirectXTex (BC1-BC7 + TGA 璇诲彇) | 涓?DX11 閰嶅 |





### 2.3 缃戠粶





| 缁村害 | 閫夊瀷 | 鐞嗙敱 |


|------|------|------|


| 寮傛 I/O | **Boost.Asio** (鎴愮啛) / **standalone Asio** (杞婚噺) | C++ 鏍囧噯搴撳寲鍊惧悜 |


| 鍗忚 | 淇濈暀鍘?`[CC]Header/Protocol.h` (96 绫? + 鍙€?**FlatBuffers** (鏂板崗璁? | 鑰佸崗璁?100% 鍏煎 |


| 鍔犲瘑 | **OpenSSL 3.x** (AES-256-GCM) | 鏍囧噯鍚堣 |


| 鍘嬬缉 | **zstd** | 姣?zlib 蹇?|





### 2.4 鏁版嵁搴?


| 缁村害 | 閫夊瀷 | 鐞嗙敱 |


|------|------|------|


| 榛樿 | **MS SQL Server 2019+** | 鍘熺増鍗?MSSQL |


| 澶囬€?| **PostgreSQL 16+** | 寮€婧愩€佽法骞冲彴 |


| 鎶借薄灞?| 鑷爺 `IDbAdapter`锛堜繚鐣欏師 `eQueryType` 璺敱锛?| 鍏煎鑰佷唬鐮?|


| ORM | **涓嶅紩鍏?*锛堜繚鎸佸師鎵嬪啓 SQL 椋庢牸锛?| 閬垮厤澶ф敼 |


| 椹卞姩 | 鍘?ODBC 淇濈暀 / 鏂颁唬鐮佸彲鐢?libpqxx / ODBC | |





### 2.5 UI / 宸ュ叿





| 缁村害 | 閫夊瀷 | 鐞嗙敱 |


|------|------|------|


| 瀹㈡埛绔?UI | **淇濈暀鑷爺 cWindow**锛圖X11 鍚庣锛?/ **鍙€?ImGui**锛堣皟璇曟ā寮忥級 | 1:1 澶嶅埢蹇呴€変繚鐣?|


| 宸ュ叿 GUI | **C# Avalonia** / **Rust egui** / **Web (React/Vue)** | 鐜颁唬璺ㄥ钩鍙?|


| Web 鍚庡彴 | **FastAPI** (Python) / **ASP.NET Core** (C#) | 鐜颁唬 Web 鏍囧噯 |





### 2.6 绗笁鏂瑰簱鏇挎崲琛?


| 鍘熶緷璧?| 鏂颁緷璧?| 鏇挎崲闃舵 |


|--------|--------|---------|


| DirectX 8.1 SDK | DirectX 11/12 SDK + DirectXTex | Phase 5 |


| d3dx8.lib | (鍐呯疆浜庣幇浠?SDK) | Phase 5 |


| HackShield (AhnLab) | 鏈嶅姟绔潈濞佹牎楠?/ 绉婚櫎 | Phase 3 |


| nProtect GameGuard | 鏈嶅姟绔潈濞佹牎楠?/ 绉婚櫎 | Phase 3 |


| HSEL 鍔犲瘑鐙?| OpenSSL AES-256-GCM | Phase 3 |


| Miles Sound System | **OpenAL Soft** / FMOD / Wwise | Phase 6 |


| FreeImage | DirectXTex / stb_image | Phase 5 |


| MFC (UI) | 淇濈暀 cWindow 鑷爺 | 涓嶆浛鎹?|


| wsock32.lib | WinSock 2 (ws2_32.lib) | Phase 0 |


| vfw32.lib + wmstub.lib + amstrmid.lib | libavcodec + SDL | Phase 5 |


| 3ds Max Biped/Physique 鎻掍欢 | 淇濈暀 + FBX 涓棿鏍煎紡 | Phase 10 |


| Perforce (.vsscc) | Git | Phase 0 |


| VC6/VS2003 宸ョ▼鏂囦欢 | CMake | Phase 7 |





---





## 3. 1:1 澶嶅埢娓呭崟锛堢粷瀵逛笉鑳界牬鍧忕殑閮ㄥ垎锛?


### 3.1 鍗忚灞傦紙蹇呴』瀹屽叏淇濈暀锛?


- **`[CC]Header/Protocol.h` 3542 琛岀殑 MP_CATEGORY 鏋氫妇**


- **`[CC]Header/CommonStruct.h` 140 KB 缃戠粶鍖呯粨鏋?*


- **`[CC]Header/CommonGameDefine.h` 108 KB 甯搁噺涓庢灇涓?*


- **鏈嶅姟绔垎鍙戦€昏緫**锛?3 涓?NetworkMsgParse 鍏ュ彛锛?- **瀹㈡埛绔?OnRecv 鈫?CGameState::NetworkMsgParse 璺緞**


- **HSEL 鍔犲瘑绠楁硶**锛堝嵆浣挎崲鎴?AES锛屽姞瀵嗗悗瀛楄妭娴佸彲涓庢棫瀹㈡埛绔彙鎵嬪吋瀹癸級





### 3.2 璧勬簮鏍煎紡锛堝繀椤诲畬鍏ㄤ繚鐣欙級





| 鏍煎紡 | 瀹炵幇浣嶇疆 | 鍏抽敭 header |


|------|----------|------------|


| `.bin` | `[Tool]PackingMan/MHFileEx.cpp` | `MHFILE_HEADER { dwVersion, dwType, FileSize }` |


| `.pak` | `4DyuchiFileStorage/PackFile.cpp` | `PACK_FILE_HEADER { dwTotalSize, dwFileCount, ... }` |


| `.bmhm` / `.mhm` | `4DyuchiGXMapEditor/` | 8 瀛楄妭 magic `7E-CB-31-01-2A-00-00-00` |


| `.ttb` | `MHMap.cpp` | TileTable |


| `.chx` / `.chl` / `.chr` | `MAXEXP/` + 瀹㈡埛绔?`cCharMove` | 鑷爺鏍煎紡 |


| `.mon` | 瀹㈡埛绔?`Monster.cpp` | 鈥?|


| `.bsad` | `[CC]Skill/SkillArea` | 鎶€鑳藉尯鍩?|


| `.mhs` | `StringLoader.cpp` | 瀛楃涓茬储寮?|





### 3.3 娓告垙閫昏緫锛堝繀椤诲畬鍏ㄤ繚鐣欙級





- 鎴樻枟鍏紡锛坄AttackCalc.cpp`銆乣BattleFactory_Default.cpp`锛?- 鎶€鑳芥爲锛坄SuryunRegen`銆乣SkillManager_server.cpp`锛?- 鐗╁搧寮哄寲/鍚堟垚/娉ㄩ瓊锛坄ReinforceManager`銆乣RarenessManager`銆乣ChangeItemMgr`锛?- 宸ヤ細鎴?鏀诲煄鎴?鎹偣鎴橈紙`GuildFieldWarMgr`銆乣SiegeWarMgr`銆乣FortWarManager`锛?- 鎽嗘憡/浜ゆ槗/鎷嶅崠锛坄StreetStall`銆乣ExchangeManager`銆乣AuctionContents`锛?- 浠诲姟/鎴愬氨锛坄QuestManager`銆乣QuestUpdater`锛?- 鎬墿 AI锛坄AISystem`銆乣AIManager`锛?- 瀹犵墿/娉板潶锛坄Pet`銆乣Titan`锛?


### 3.4 鐜╂硶骞宠　鏁版嵁锛堝繀椤诲畬鍏ㄤ繚鐣欙級





- 缁忛獙鏇茬嚎


- 鐗╁搧灞炴€?- 鎶€鑳戒激瀹冲叕寮?- 瑁呭鎺夌巼


- Boss 鍒锋柊


- 鍟嗗煄鐗╁搧浠锋牸





**杩欎簺閮芥槸 `.bin` 鏂囦欢锛岃鍙栨纭嵆鍙紱涓嶈鍘?璋冩暣骞宠　"鎴?淇 bug"鈥斺€旈櫎闈炵敤鎴锋槑纭姹傘€?*





---





## 4. 椋庨櫓涓庣紦瑙?


| 椋庨櫓 | 绛夌骇 | 缂撹В鎺柦 |


|------|------|---------|


| DX11 鍚庣涓庡師 DX8 娓叉煋缁嗚妭涓嶄竴鑷?| 涓?| 1:1 鎴浘姣斿 + 鑷姩鍖栬瑙夊洖褰?|


| 鑰?.vcproj/.dsp 鏃犳硶鐩存帴杩佺Щ | 楂?| 鍏堜繚鐣欐棫宸ョ▼锛孋Make 涓庡叾鍏卞瓨 |


| `.chl/.anm` 妯″瀷鏍煎紡鏃犱汉鏂囨。鍖?| 涓?| 缂栧啓閫嗗悜鏂囨。 + 杞崲鍣?|


| HSEL 鍔犲瘑鐙楁浛鎹㈠鑷磋€佸鎴风鏃犳硶鐧诲綍 | 浣?| 淇濈暀 HSEL 鎺ュ彛鍏煎灞傦紝榛樿绂佺敤 |


| 3ds Max 2018+ 绉婚櫎 Biped 鎻掍欢 | 浣?| 淇濈暀 3ds Max 7-2017 鏃х増 + 鎻愪緵 FBX 涓棿鏍煎紡 |


| 鏁版嵁搴?1:1 鍏煎浣?MSSQL 鎬ц兘鐡堕 | 浣?| 淇濈暀 IDbAdapter 鎺ュ彛锛屽彲鎹?PostgreSQL |


| 璺ㄥ钩鍙扮紪璇戝伐浣滈噺澶?| 涓?| 鍏?Windows-only锛圖X11锛夛紝璺ㄥ钩鍙颁綔涓哄彲閫夐」 |





---





## 5. 闃舵浜や粯鐗╋紙姣忛樁娈电殑"瀹屾垚鏍囧噯"锛?


### Phase 0 浜や粯鐗?- [x] 鏈鍒掓枃妗?- [ ] `.gitignore`锛堣繃婊ゆ棫宸ョ▼鏂囦欢锛?- [ ] `AGENTS.md`锛圓I 鍔╂墜鎸囧崡锛?- [ ] `cmake_minimum.txt` 鐜颁唬宸ョ▼楠ㄦ灦


- [ ] `scripts/start-server.ps1` 涓€閿惎鍔ㄨ剼鏈?


### Phase 1 浜や粯鐗?- [x] `modern/MoxianCompat` 闈欐€佸簱锛圕MHFileEx + PackFile 閲嶅啓锛?- [x] `tools/MoxianResourceExplorer` 鍛戒护琛岃祫婧愭祻瑙堝櫒


- [x] 鍗曞厓娴嬭瘯锛氭墍鏈?`.bin/.pak/.bmhm/.bsad` 璇诲彇楠岃瘉


- [ ] 鏂囨。锛歚docs/RESOURCE_FORMATS.md`





### Phase 2 浜や粯鐗?- [x] `modern/MoxianDb` IDbAdapter 鎺ュ彛


- [x] MSSQL + PostgreSQL 鍙屽疄鐜?- [x] `tools/MoxianSchemaExporter` schema 瀵煎嚭


- [ ] 鏂囨。锛歚docs/DATABASE_SCHEMA.md`





### Phase 5 浜や粯鐗╋紙DX11 娓叉煋鍣級


- [x] `modern/src/render/mxh_render` 闈欐€佸簱 鈥?DX11 鍚庣


- [x] `modern/include/mxh/render/IRenderer.hpp` 鈥?1:1 绔彛锛?5 涓柟娉曠鍚嶏級


- [x] `modern/include/mxh/render/IFileStorage.hpp` 鈥?1:1 绔彛锛?7 涓柟娉曠鍚嶏級


- [x] `modern/include/mxh/render/render_typedef.hpp` 鈥?涓庡師 DX8 寮曟搸浜岃繘鍒跺吋瀹圭殑缁撴瀯浣?- [x] Device 鍒濆鍖栵紙SwapChain / RenderTarget / 榛樿鐘舵€佸璞★級


- [x] PrimitiveDrawer锛圧enderBox/Line/Point/Circle/Grid 鐨?DX11 瀹炵幇锛?- [x] SpriteObject锛圛D3D11Texture2D + SRV锛孌raw + Resize + LockRect锛?- [x] TGA 瑙ｇ爜鍣紙uncompressed + RLE锛屽惈 bottom-up 缈昏浆锛?- [x] `tools/MoxianRenderDemo` 鈥?鐑熼浘娴嬭瘯锛?D lit textured cube + wireframe grid锛?- [x] TGA 瑙ｇ爜鍣ㄥ崟鍏冩祴璇?鈥?7 涓?case 鍏ㄨ繃锛坄TgaLoader.*`锛?- [x] MeshObject DX11 瀹炵幇锛坄MeshObject::StartInitialize/EndInitialize/InsertFaceGroup`锛?  + 鑷爺 HLSL 鐫€鑹插櫒 (`kVS_Lit` / `kPS_Lit`锛孌3DCompile `vs_4_0` / `ps_4_0`)


  + 3 涓父閲忕紦鍐? `world` + `viewProj` + `light (ambient/diffuse/lightDir/cameraPos/fog)`


  + 绔嬫柟浣撶敓鎴?(`initializeCube`, 24 vert / 36 idx) + MeshDescAndFaceDesc 鍚堢害鍗曞厓娴嬭瘯


- [x] FontObject DX11 瀹炵幇锛圙DI `GetGlyphOutlineA` + 512脳512 BGRA atlas + row-packing锛?  + 8-bit 鍗曞瓧鑺傜爜鐐圭紦瀛橈紙涓庡師 MultiByte 鏋勫缓鐨?`TCHAR == char` 涓€鑷达紱CJK 鐢?.TTB 棰勫鐞嗙绾挎壙鎷咃級


  + `packGlyph` 琛屽唴鎵撳寘 + 婧㈠嚭鏃舵暣寮?atlas 澶嶄綅锛圕PU 渚х畻娉曞彲鐙珛鍗曟祴锛? case锛?  + 澶嶇敤 PrimitiveDrawer::drawTexturedQuad锛屾棤闇€鏂板 shader


- [x] ChxModel 鐪熷疄璧勬簮娴嬭瘯锛? case锛歚.chx` 鏄?TAB 鍒嗛殧鏂囨湰鍏冩暟鎹? 涓嶆槸浜岃繘鍒?mesh锛?- [x] ctest 闆嗘垚锛坓test_add_tests 鎵嬪伐鍒椾妇, 缁曞紑涓枃璺緞涓?gtest_discover_tests 鐨?JSON 杈撳嚭瓒呮椂锛?- [x] Phase 5 杩涘害鎶ュ憡锛堣涓嬫柟 "Phase 5 褰撳墠鐘舵€佹憳瑕?锛?### Phase 5 褰撳墠鐘舵€佹憳瑕侊紙2026-07-07 鏇存柊锛?


**宸插畬鎴?*锛?5 涓?I4DyuchiGXRenderer 鏂规硶 + Device + PrimitiveDrawer + SpriteObject + TGA + MeshObject + FontObject + 鑷爺 HLSL + ChxModel 娴嬭瘯 + ctest 闆嗘垚銆侱ebug 娴嬭瘯 **47/47 PASS**銆?


**鏈鏋勫缓淇**锛?- `chx_real_resource_test.cpp`锛氫慨姝?API 璋冪敤锛坄PackFile::open` 杩斿洖 `unique_ptr`锛宍read_mh_bin` 杩斿洖 `Result<T>`锛宍std::min<size_t>` MSVC 鐗瑰寲锛夆渽


- `FontObjectAtlas` 娴嬭瘯鏂█锛氫慨澶?`FakeAtlasPacker` 鏈熸湜鍊硷紙`kW=50` 鏃剁涓夋 glyph 纭疄瑙﹀彂鎹㈣锛夆渽


- RenderDemo `mesh_object.hpp` 鍐呴儴澶寸Щ闄や緷璧栵紝鏀逛负閫氳繃 `IDIMeshObject` 鍏紑鎺ュ彛杩愯 鉁?- 鎵€鏈?CMakeLists.txt 绉婚櫎 `gtest_discover_tests`锛堜腑鏂囪矾寰勫鑷?JSON 鍐欏叆澶辫触锛屾敼鐢ㄦ墜鍔?`add_test`锛夆渽


- MSBuild 璺緞闂淇锛歊enderDemo 涓嶄緷璧栧唴閮ㄥ懡鍚嶇┖闂达紝缁曡繃 MSBuild 涓枃璺緞宕╂簝 鉁?


**宸茬煡闄愬埗 (stubs / deferred)**锛?


| Stub | 鎺ュ彛 | 鐘舵€?| 澶囨敞 |


|------|------|------|------|


| `CreateHeightField` | LOD + alpha + chunked VB | 鏂囨。鍘熻鍒?deferred | 鏃?4Dyuchi HeightField 鏄ぇ妯″潡 |


| `CreateMaterial[Set]` | 鏉愯川琛ㄧ鐞?| Phase 5 楂樼骇 | 闇€瑕?MATERIAL鈫扴RV 缂撳瓨 |


| `CreateEffectShaderPalette*` | CUSTOM_EFFECT_DESC | Phase 5 楂樼骇 | 鐗规晥绯荤粺 |


| `RenderTri*` / `AllocRenderTriBuffer*` | TriBuffer 璺緞 | Phase 5 楂樼骇 | 鍔ㄦ€佷笁瑙掔紦鍐?|


| `CaptureScreen` | DX11 offscreen RT + 淇濆瓨 | 宸ュ叿 | 灞忓箷鎴浘 |


| `ConvertCompressedTexture` | BC1-BC7 鍘嬬缉 | Phase 5 楂樼骇 | DirectXTex |


| `BeginShadowMap` / `EndShadowMap` | shadow map pipeline | 鍚庢湡 | |


| `GetD3DDevice` (闈?IUnknown IID) | 鍏煎鑰佷唬鐮?| 鍏煎鎬?| 褰撳墠鍙敮鎸?`IUnknown` |





> **FontObject 鑼冨洿璇存槑**锛?-bit 缂撳瓨锛堥潪 Unicode CJK 绨囷級锛汣JK 搴旇蛋鑰佸紩鎿庤嚜宸辩殑 .TTB 棰勭儰瀛楀浘銆?> DirectWrite SDF / 澶氳壊 emoji 闇€瑕佸崟鐙?Phase 6+ 宸ヤ綔銆?


**娴嬭瘯缁熻**锛?| 濂椾欢 | 鏁伴噺 | 鍐呭 |


|------|------|------|


| `TgaLoader` | 7 | TGA uncompressed / RLE / bottom-up-flip / RGBA32 |


| `MeshGeometryTest` | 4 | MESH_DESC + FACE_DESC 鍚堢害 |


| `FontObjectGlyph` | 2 | GlyphEntry 瀛楁 + CHAR_CODE_TYPE 鏋氫妇鍊?|


| `FontObjectAtlas` | 4 | row-packing锛氭按骞斥啋鍥炶鈫掓孩鍑哄浣嶁啋琛岄珮璺熻釜鏈€澶у瓧 |


| `MhFileEx` | 6 | `.bin` XOR/浣嶇Щ鍔犺В瀵?+ CRC 鏍￠獙 |


| `PackFile` | 5 | `.pak` 澶磋В鏋?+ 瀹炶祫婧愬洖鐜?|


| `BsadArea` | 4 | `.bsad` 鎶€鑳藉尯鍩?|


| `ChxModelRealResource` | 4 | 鐪熷疄 `.chx` TAB 鍒嗛殧鏂囨湰 |


| `DbAdapter` | 4 | `IDbAdapter` 宸ュ巶 + 閰嶇疆 |


| `SqliteAdapter` | 5 | SQLite 鍚庣锛堜簨鍔?BLOB/鏂囦欢鎸佷箙鍖栵級 |


| `RealResource` | 2 | 鐪熷疄 `MonsterList.bin` + `Effect.pak` 璺戦€?|


| **鍚堣** | **47** | Debug 鍏ㄨ繃 |





### Phase 3-7 浜や粯鐗╋紙姣忛樁娈碉級


- [ ] 鏂版ā鍧楁簮鐮?+ 鍗曞厓娴嬭瘯


- [ ] 鍥炲綊娴嬭瘯锛氫笌鍘熸ā鍧楄涓哄姣?- [ ] 鏇存柊鏂囨。


- [ ] CMakeLists.txt 闆嗘垚





---





## 6. 涓嶅湪鏈鍒掕寖鍥村唴





涓洪伩鍏嶈寖鍥磋敁寤讹紝鏄庣‘浠ヤ笅**涓嶅仛**锛?


1. 鉂?鏀瑰彉娓告垙骞宠　锛堜激瀹炽€佺粡楠屻€佺垎鐜囩瓑锛?2. 鉂?娣诲姞鏂拌亴涓?鏂板湴鍥?鏂拌澶囷紙闄ら潪鐢ㄦ埛鏄庣‘瑕佹眰锛?3. 鉂?閲嶅啓鍗忚锛堜繚鐣欏師濮?Category/Protocol锛?4. 鉂?鏇挎崲璧勬簮鏍煎紡锛堜繚鐣?.bin/.pak 绛夛級


5. 鉂?鍙嶅悜宸ョ▼鍔犲瘑鐙楋紙HSEL锛?6. 鉂?鍟嗕笟鍖栬繍钀ョ浉鍏筹紙璁¤垂銆佸晢鍩庡悗鍙帮級





濡傞渶浠ヤ笂鍔熻兘锛屼綔涓虹嫭绔嬮」鐩璁恒€?


---





## 7. 绔嬪嵆寮€濮嬬殑鎵ц椤癸紙Phase 0锛?


1. 鉁?**宸插畬鎴?*锛氭湰璁″垝鏂囨。


2. 鈴?**杩涜涓?*锛?   - 鍒涘缓 `.gitignore`


   - 鍒涘缓 `AGENTS.md`


   - 鍒涘缓 `cmake/` 楠ㄦ灦锛堝厛涓嶅姩鍘熷伐绋嬶級


   - 鍒涘缓 `scripts/` 涓€閿惎鍔ㄨ剼鏈?   - 缂栧啓 `modern/MoxianCompat` Phase 1 璧锋浠ｇ爜





---





## 8. 闀挎湡鎰挎櫙





缁忚繃 3-6 涓湀鐨勬笎杩涘紡鐜颁唬鍖栵紝澧ㄩ灏嗭細


- 鑳藉湪 Windows 11 + 鐜颁唬纭欢 + VS2022 涓婃祦鐣呯紪璇戣繍琛?- 淇濈暀 100% 鍘熷娓告垙鍐呭锛堣祫婧愩€佺帺娉曘€侀€昏緫锛?- 鏁版嵁搴撱€佺綉缁溿€佸姞瀵嗐€乁I 鍏ㄩ儴鎺ュ彛绋冲畾锛屽唴閮ㄧ幇浠ｅ寲


- 鎬ц兘鎻愬崌锛堝绾跨▼銆丟PU 鍔犻€熴€佸唴瀛樹紭鍖栵級


- 鍙€夎法骞冲彴锛圠inux 鏈嶅姟绔?+ macOS 瀹㈡埛绔級


- 鎷ユ湁瀹屽杽鐨勬枃妗ｄ笌鏋勫缓宸ュ叿閾?- 浠讳綍璐＄尞鑰呴兘鑳藉湪 1 灏忔椂鍐呰窇璧锋潵





**鏈€缁堢洰鏍?*锛氳杩欎唤 2003 骞寸殑浠ｇ爜锛屽湪 2026 骞翠緷鐒惰兘瀹屾暣杩愯锛屽苟涓旀瘮鍘熷鐗堟湰鏇村揩銆佹洿绋冲畾銆佹洿鏄撶淮鎶ゃ€


