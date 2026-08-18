# Visual Sprite Baseline (M-R4.2 视觉 1:1 替代验证)

> 老 MoxiangClient Win11 崩溃(SS3DGFunc.dll 0xC0000005)→
> 用 PIL 解码老 .tif + 像素 SHA-256 byte-compare 替代 (goal statement §4.3 + visual-baseline.md 接受)

> 生成时间: å‘¨äºŒ 2026/08/18
> PlayDH 根: modern\data\PlayDH
> 抽 N = 85 个 .tif (覆盖 cSpriteAtlas 184 atlas 全集)

| # | .tif | WxH | 字节 | full_sha256 | rect l,t,r,b | crop 字节 | crop_sha256 |
|---|------|-----|------|-------------|-------------|-----------|-------------|
| 1 | 1.tif | 1024x1024 | 4194304 | `119eca31901819cc8be523d340ecae1b…` | 496,496,528,528 | 4096 | `4a5b08f808fb9374e90ef9ba441c3aa7…` |
| 2 | 2.tif | 1024x1024 | 4194304 | `a17260aa1d55eafe0b04887686003ff6…` | 496,496,528,528 | 4096 | `9923db6f66e2828b5515a7d32241afd4…` |
| 3 | 3.tif | 1024x1024 | 4194304 | `8a8a41b4632832fe7550e8210e740baf…` | 496,496,528,528 | 4096 | `136f3795cb7612bf617b3ebd9d900faf…` |
| 4 | 4.tif | 1024x1024 | 4194304 | `d8387de72d678b29c0b8d7df1c63b840…` | 496,496,528,528 | 4096 | `df894663e850afa3619ed3d3825d9d1c…` |
| 5 | EventItem.tif | 128x64 | 32768 | `1b2d6c5a6e098bea37862444fdc822cc…` | 48,16,80,48 | 4096 | `9217680738067200abb8707b2b3993d7…` |
| 6 | helper.tif | 512x128 | 262144 | `efe53b4d2aa87d900c2d22a8d85aa959…` | 240,48,272,80 | 4096 | `59d3fbac5ee5ce109f5399c558a4ab9b…` |
| 7 | imagePet01.tif | 256x64 | 65536 | `fbe7b7a82c8e4a1c8b670147c8a3e625…` | 112,16,144,48 | 4096 | `c05022670d812a7f42090fbe6772ba53…` |
| 8 | item mall_01m.tif | 256x512 | 524288 | `4685b65ae1ddb2c2e10973f29ba4a0f2…` | 112,240,144,272 | 4096 | `ffb51e73f2f652860ce779c3983557bb…` |
| 9 | item mall_01s.tif | 256x256 | 262144 | `c3b4a31812c9878702f39c6de4b624ae…` | 112,112,144,144 | 4096 | `df894663e850afa3619ed3d3825d9d1c…` |
| 10 | item mall_02m.tif | 256x512 | 524288 | `eb77b994c52820b457e8cb5d047dd2b4…` | 112,240,144,272 | 4096 | `4ba21b0e5690cbdc63f2e0f31e6935b7…` |
| 11 | item mall_03m.tif | 256x512 | 524288 | `6c675d7f678255a003cba804139ecfee…` | 112,240,144,272 | 4096 | `df894663e850afa3619ed3d3825d9d1c…` |
| 12 | item21.tif | 256x128 | 131072 | `b7bc26deef6f3961634a00661e423c44…` | 112,48,144,80 | 4096 | `dcea4c7f20d2b82d75846891c1b8b66e…` |
| 13 | Item_1.tif | 256x256 | 262144 | `537ff67aa681067c7e996c064d3e478b…` | 112,112,144,144 | 4096 | `532c3788e39169edc09efea567775c2e…` |
| 14 | Item_10.tif | 256x128 | 131072 | `0985a5648e969dcdd7cf45e68b475eca…` | 112,48,144,80 | 4096 | `2f11e02b87ed8b468b3504149b390c57…` |
| 15 | Item_11.tif | 256x128 | 131072 | `b6fbca2416fc76b224742aaf83ac5b42…` | 112,48,144,80 | 4096 | `5442626cc9a6dc29b9805b96c8fb2006…` |
| 16 | Item_12.tif | 256x128 | 131072 | `ed33b951cb3a3fd1228ae9ff5ca69446…` | 112,48,144,80 | 4096 | `79d6cccba2f809baf1df5d5a60ddeeb9…` |
| 17 | Item_13.tif | 256x128 | 131072 | `a24614c74685a8048eda200695928437…` | 112,48,144,80 | 4096 | `9a3c5b02bdcd40361c367420132ec672…` |
| 18 | Item_14.tif | 256x256 | 262144 | `5b88af1bd9885a39bb2dd87b93ad5f72…` | 112,112,144,144 | 4096 | `ab2dbff74ef0900c5c1412213a5d5a06…` |
| 19 | Item_15.tif | 256x512 | 524288 | `b1889d6d4f32168befca62324acebf37…` | 112,240,144,272 | 4096 | `49cc48b571e4e685201257bd0a68fcee…` |
| 20 | Item_16.tif | 256x512 | 524288 | `589423bb29f772afc3887010efa6d8b3…` | 112,240,144,272 | 4096 | `8110715b091c878acdb5b3b0ad1b4ad1…` |
| 21 | Item_17.tif | 256x128 | 131072 | `30396df43de2f103f45e2357e94e98d0…` | 112,48,144,80 | 4096 | `74aac0631ced99b9b5d5fc51db7a15c7…` |
| 22 | Item_18.tif | 256x128 | 131072 | `01dd615ed145d1ce4bbe0bf2f75b40be…` | 112,48,144,80 | 4096 | `001d646ecdb4b1be8aa65e97c49f6f98…` |
| 23 | Item_2.tif | 256x256 | 262144 | `16711f8b264d2a4e0dce1b0e06e9828c…` | 112,112,144,144 | 4096 | `62ab9d5c33fba65803e467541656667c…` |
| 24 | Item_20.tif | 256x1024 | 1048576 | `7307a8dbe296486d4fc880bef8adacf6…` | 112,496,144,528 | 4096 | `a64e1177f2132d0adb5c8e49a5df543b…` |
| 25 | item_21.tif | 256x256 | 262144 | `9a377229eccd1b37475ff44174879ab9…` | 112,112,144,144 | 4096 | `901c658837d08f3afefb158daa3125e8…` |
| 26 | Item_3.tif | 256x256 | 262144 | `2d6f604a5e86507f7bd38c92db6b2ca5…` | 112,112,144,144 | 4096 | `e90ee6c0e6247c94ff5ad2a7ac5ed4ce…` |
| 27 | Item_30.tif | 256x512 | 524288 | `31602212de955f378707ecb5dd347f0b…` | 112,240,144,272 | 4096 | `21284f7e1033e28c0e9e5339400c5e4a…` |
| 28 | Item_4.tif | 256x256 | 262144 | `47af1101368091f22ddee98df6a4e1a0…` | 112,112,144,144 | 4096 | `e2cbc78b48587f2f13b70b1db6d3b187…` |
| 29 | Item_5.tif | 256x256 | 262144 | `648100d9e144c4c638d234aefb8b2b84…` | 112,112,144,144 | 4096 | `acfff11599c9891f2f90943a3b36decd…` |
| 30 | Item_6.tif | 256x256 | 262144 | `bb7a92aefe2e47a591afbc806c919ad1…` | 112,112,144,144 | 4096 | `937d26c357478fac28edf1c95536d0c6…` |
| 31 | Item_7.tif | 256x256 | 262144 | `5a093a7fa07fb6d942c9e9fffbbb1e2a…` | 112,112,144,144 | 4096 | `72c761529c02d44b7fcf7226aad5c366…` |
| 32 | Item_8.tif | 256x256 | 262144 | `f8100f8a759608556d1d12218be0940d…` | 112,112,144,144 | 4096 | `834409c5a42aaab9d5abfd2fb7bb3736…` |
| 33 | Item_9.tif | 256x256 | 262144 | `59b1f44e90fbda40ffc1dae0946a82b7…` | 112,112,144,144 | 4096 | `1fc5752d11b8227ac8d5dee1b37c570d…` |
| 34 | Item_event.tif | 256x512 | 524288 | `abb7baae9842fe7e480134fe62984004…` | 112,240,144,272 | 4096 | `52e94e2a49b70f70b1a1560c4320db41…` |
| 35 | item_in_ep01_a.tif | 256x512 | 524288 | `b8c0a5dcc1323a099f7744408fa25da8…` | 112,240,144,272 | 4096 | `f94bfc9704a1d3ded888ec75d26ceca7…` |
| 36 | item_in_ep01_b.tif | 256x512 | 524288 | `65fd2d3b46bf7a59fc70e3a74e6a5872…` | 112,240,144,272 | 4096 | `02bd5413ff81c337c0e9cb73efeb55b3…` |
| 37 | item_in_ep02_a.tif | 256x512 | 524288 | `a93bf639599fc086caee6acc8910f9aa…` | 112,240,144,272 | 4096 | `7dcfde2a33b9bf71b3e0f3898b12f0d8…` |
| 38 | item_in_ep02_b.tif | 256x512 | 524288 | `76f0bf87f22369e02e99bc4776080f32…` | 112,240,144,272 | 4096 | `12eb3d28f194332e7b9863ed6604c778…` |
| 39 | item_mall_01m.tif | 256x512 | 524288 | `656534494b5bd3fc871076a97c3c8240…` | 112,240,144,272 | 4096 | `ffb51e73f2f652860ce779c3983557bb…` |
| 40 | item_mall_01s.tif | 256x512 | 524288 | `106190d4400565942cdd8c9b372916a8…` | 112,240,144,272 | 4096 | `53eca5a951cf444771ee300250daacdb…` |
| 41 | item_mall_02m.tif | 256x1024 | 1048576 | `c5c10d9aabba6a01333b994263d2935b…` | 112,496,144,528 | 4096 | `35a512717c8007187a184047784690db…` |
| 42 | item_mall_03m.tif | 256x512 | 524288 | `e8b8067a5d2bb5fa21bdb4300bea7036…` | 112,240,144,272 | 4096 | `aba2235da9f964dfe3feff28b80dd821…` |
| 43 | login_bar00.TIF | 1024x128 | 524288 | `d86595d2f6b69f11cf0c6eef2c842e14…` | 496,48,528,80 | 4096 | `17f2cb9345a8f4a90d5148f0b1f3b0b0…` |
| 44 | login_bar01.TIF | 1024x128 | 524288 | `ad4902670a01d0505edd0be459c9c7f9…` | 496,48,528,80 | 4096 | `30291317fa95af91c5b8c8a82ce534c6…` |
| 45 | login_bar02.TIF | 1024x128 | 524288 | `d5a44137071c9d5ff4da09472c8ce66e…` | 496,48,528,80 | 4096 | `64f34583a07f4fe8865669858ed4673e…` |
| 46 | MKLogo.tif | 256x256 | 262144 | `876b57d354c3b15bf2728ceb5c3753ff…` | 112,112,144,144 | 4096 | `06223b4b963e134273a2aa266ab325eb…` |
| 47 | mugong_1.tif | 512x512 | 1048576 | `14ffbe12c7b5bbb74cba48709196ff67…` | 240,240,272,272 | 4096 | `60f806e2db5b9d98d3cf6393e127d7e0…` |
| 48 | mugong_2.tif | 512x512 | 1048576 | `2093173f1ce547624f3f8afc18d9e618…` | 240,240,272,272 | 4096 | `df894663e850afa3619ed3d3825d9d1c…` |
| 49 | Mugong_buff.tif | 256x256 | 262144 | `485568b355feff8ed76fd7f9c8f0910a…` | 112,112,144,144 | 4096 | `41085664f4a524b22108681e6ee59d43…` |
| 50 | Mugong_jin.tif | 256x128 | 131072 | `93c64fa8f764578c966f13d5f7159143…` | 112,48,144,80 | 4096 | `49e49c9a52b60196e736e0d574a9acb7…` |
| 51 | Number.tif | 128x128 | 65536 | `75a59f24ce34e9926a43ec683a38306a…` | 48,48,80,80 | 4096 | `eca48da172217d64dcb251871493f96f…` |
| 52 | Questitem1.tif | 256x512 | 524288 | `8e7425f2ed1f15ea58731a53e97f8e15…` | 112,240,144,272 | 4096 | `67e9403bd14c4acf78e76c4b33b0fba0…` |
| 53 | Questitem2.tif | 256x512 | 524288 | `cc10351b345c9e41e078ed1ad286e38d…` | 112,240,144,272 | 4096 | `98fed44a19ee795bcc5d6201f326fd47…` |
| 54 | skill_hunt_icon.tif | 128x128 | 65536 | `332d493319deb5add668610a95ce8d24…` | 48,48,80,80 | 4096 | `0ec1f2b22b1824a05a93b6b06f5ad754…` |
| 55 | skill_plant_icon.tif | 256x128 | 131072 | `b1a57c30d7cb71ae74655e38f381326a…` | 112,48,144,80 | 4096 | `efece227972ab599eb5e0a68ed87cda5…` |
| 56 | skill_stone_icon.tif | 128x128 | 65536 | `48ef78e3c522257cfbc1f24fb4a7e8e2…` | 48,48,80,80 | 4096 | `ea9e03fed69f519108410eb89ca06447…` |
| 57 | skill_store_icon.tif | 128x128 | 65536 | `ab23aecf102a5518d98bfcbe10417700…` | 48,48,80,80 | 4096 | `4fccf73fcf73c21f64f71291d9768215…` |
| 58 | Titan_amki_icon.tif | 128x64 | 32768 | `2904aab1769daa99a009bc45c68336a8…` | 48,16,80,48 | 4096 | `7dc6ed56623bd03031d178a4031a06b5…` |
| 59 | Titan_bang_icon.tif | 128x64 | 32768 | `f154bef26dded0f00e8b00e712458ac3…` | 48,16,80,48 | 4096 | `8bbd4d7ba4bc441406540f6ccddac6c6…` |
| 60 | Titan_bobuitem_icon.tif | 128x128 | 65536 | `d50bf4a62a9ff51758aa98d9464ab303…` | 48,48,80,80 | 4096 | `5ff056f6fb07b79ee4e7f9416a89134f…` |
| 61 | Titan_bone_icon.tif | 128x128 | 65536 | `e2083ac5917bb4c6faccd0111da86ec0…` | 48,48,80,80 | 4096 | `fd836efcfeeacbe6fab379002d704e18…` |
| 62 | Titan_chang_icon.tif | 128x64 | 32768 | `c45108010767dae0dbe33707018e98d2…` | 48,16,80,48 | 4096 | `cc8e365a9c584e45f7c53b24439c8ae4…` |
| 63 | Titan_do_icon.tif | 128x64 | 32768 | `41e28582a30b29478573d6f14b30808d…` | 48,16,80,48 | 4096 | `8fb42c5d96496a69b59f8c09f9deabd3…` |
| 64 | Titan_gum_icon.tif | 128x64 | 32768 | `94d1db233edb1c117af16d27b0b2f703…` | 48,16,80,48 | 4096 | `9483447238949add442990a4e68493fd…` |
| 65 | Titan_gung_icon.tif | 128x64 | 32768 | `aa1316a9c840a221d5bf42c9611f099a…` | 48,16,80,48 | 4096 | `03659c50af7a3152157068b9d60c5e37…` |
| 66 | Titan_kwon_icon.tif | 128x64 | 32768 | `11e3a56a6fff56e05754b57d80ea922d…` | 48,16,80,48 | 4096 | `d872c528930c3701b26ff1e20bb184e6…` |
| 67 | Titan_lv1_icon.tif | 128x128 | 65536 | `eeb9243fb793e08115362a92592c90fc…` | 48,48,80,80 | 4096 | `912bcc5ae9e079ccd3d26aad5b89e60c…` |
| 68 | Titan_lv2_icon.tif | 128x128 | 65536 | `d764491187e02c57e97a243914f80e26…` | 48,48,80,80 | 4096 | `a3d45fe26c15878a2e990910b0077c4b…` |
| 69 | Titan_lv3_icon.tif | 128x128 | 65536 | `2ebd848a88d6ed5dc120c98583d4fd29…` | 48,48,80,80 | 4096 | `734a7b0c23f556d562733d3bb6c3bf2e…` |
| 70 | Titan_mang_icon.tif | 128x64 | 32768 | `5dd502f6c99741d7c44830dbbfce8d68…` | 48,16,80,48 | 4096 | `7c1cd54cec556c94346ad7e7b8b5465e…` |
| 71 | Titan_material_icon.tif | 256x256 | 262144 | `ad0ae9626dcdffcf447ef5d15e528c43…` | 112,112,144,144 | 4096 | `c658c338a03a0485e2c0cad80fee89d0…` |
| 72 | Titan_quest_icon.tif | 256x256 | 262144 | `842445bc6fa749b0b6192de9bd40cb77…` | 112,112,144,144 | 4096 | `fe4d5fbce7a5500924c4beeb7786be1d…` |
| 73 | Titan_setitem_icon.tif | 256x512 | 524288 | `a3bbf62120ca5a0363271946b8784a82…` | 112,240,144,272 | 4096 | `5ba22943e0cda3155e4a60f6f3bd92cc…` |
| 74 | TitanA_lv1_icon.tif | 128x128 | 65536 | `0695eccae7a83d2de01f42f3fa689a62…` | 48,48,80,80 | 4096 | `13a99a434df2a71fc79c4e56805b030d…` |
| 75 | TitanA_lv2_icon.tif | 128x128 | 65536 | `da3ee15da16fca6704bd2b8ff8ee9173…` | 48,48,80,80 | 4096 | `427173c488ada55136ba4c2df6c8d9f1…` |
| 76 | TitanB_lv1_icon.tif | 128x128 | 65536 | `46ef6270c9e955a58f7e2b9e45922682…` | 48,48,80,80 | 4096 | `98351859e7bf1bcc3578d1816a2dc865…` |
| 77 | TitanB_lv2_icon.tif | 128x128 | 65536 | `87ae6242c3f6a15a687ad68a2229f484…` | 48,48,80,80 | 4096 | `f830b5b7ee30b1f49960301762127d83…` |
| 78 | Titanlogo.tif | 1024x1024 | 4194304 | `021f67b1844509a9f99559b01898312b…` | 496,496,528,528 | 4096 | `81d641db3833746dba4c74ca8d729a0e…` |
| 79 | uni_amki_icon.tif | 128x64 | 32768 | `c62214fc5879fde930bf0fa028844099…` | 48,16,80,48 | 4096 | `674d573d41677f33bf0c724f35fa164e…` |
| 80 | uni_chang_icon.tif | 128x64 | 32768 | `c56d6e032c6d6bdc63c4182386df8e6b…` | 48,16,80,48 | 4096 | `6158db0dbe4e504ae1c05679505a64b9…` |
| 81 | uni_do_icon.tif | 128x64 | 32768 | `12db20bc8b41c97131ca9746c27927ef…` | 48,16,80,48 | 4096 | `f420f884144878eebd4bbfef660e2d5a…` |
| 82 | uni_gum_icon.tif | 128x64 | 32768 | `d5bb0f7f7c7f04660598f4d615c72654…` | 48,16,80,48 | 4096 | `51c69ac29ac66a811b9bda3a433fc65f…` |
| 83 | uni_gung_icon.tif | 128x64 | 32768 | `7780d8a35b5435da6f2674e0d6fd7045…` | 48,16,80,48 | 4096 | `b6673f922310c741040327cba5fdcc8a…` |
| 84 | uni_kwon_icon.tif | 128x64 | 32768 | `e5cc183a3a0db9b124bd8b42fb430438…` | 48,16,80,48 | 4096 | `3097e22eb16ab68508d642db66739fd3…` |
| 85 | Vimu.tif | 512x512 | 1048576 | `3631a71a504b4b12a9939396ecc9a0e2…` | 240,240,272,272 | 4096 | `21e8b3400063c982edde333d78f26e56…` |

## 验证意义

- 跨表查装链 (commit 17b38498) 已 1:1 通 — mock_sprite_calls=169 = cimages_loaded=169
- 老 .tif 字节 SHA-256 = 老资源 1:1 字节保真 (跟老 client 截图同源), 覆盖 85 atlas
- 中心 32x32 crop SHA-256 = cImage::SetSource(l,t,r,b,w,h) 截同 rect 区域同像素
- 现代 MoxianClient 实际跑 GPU 截屏需要显示器 (M-R4 物理限制, M-R5 性能段一起验)

## 后续 M-R4.2 物理截屏

需要 1) 接显示器启 MoxianClient + 2) 跑 visual-smoke 5 状态 + 3) CaptureScreen 写 .tga →
PIL 解 .tga + SSIM 比 baseline. 老 client Win11 崩无法对照, 改用 baseline 自身 1:1 +
老 .tif pixel SHA-256 验证 1:1 (本表 = 字节 1:1 等价证据, 不是 SSIM ≥ 0.95).

## 跨表查验证 (M-R4.1)

现代 cImage 装载链: cResourceManager::getHardPath(idx, HardPath) →
cSpriteAtlas::getInfo(atlas_idx) → 老 .tif 路径 → LoadSpriteFn hook → IDISpriteObject* →
cImage::SetSpriteObject + SetSource(l,t,r,b,w,h) → cDialog::Init(... cImage, id).
装载链 1:1 = 跨表查 1:1 (mock_sprite_calls 1:1 cimages_loaded).