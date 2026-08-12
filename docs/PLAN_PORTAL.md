# M5 — 玩家门户站点（Player Portal）方案

> 状态：draft（2026-08-12）
> 目标：补齐 ROADMAP §2 中"面向玩家的注册 Web/桌面入口、限流/封禁/审计与密码重置仍待实现"这一缺口
> 范围：在 modern/ 与 web/ 下交付一个对玩家公开的网站，覆盖 注册 / 登录 / 商城 / 下载 / 新闻 / 服务器状态
> 边界：不动 `[CC]Header/Protocol.h`、不动 `墨香【源码】/`、不动 `墨香【源码配套资源】/PlayDH/`、不动 1:1 数据/玩法/网络 wire

---

## 1. 决策摘要（与用户对齐过的）

| 维度 | 选型 | 理由 |
|---|---|---|
| 账号体系 | **单账号**（网站 = 游戏账号） | 复用 `chr_log_info` 行，玩家一套密码；避免双账号迁移成本与登录态割裂 |
| 商城深度 | **展示型**（只读物品 + 点券余额） | 不碰钱、不接支付；schema 留好 usepoint，未来加支付/发券是叠加，不是返工 |
| 部署形态 | **单 ECS**（front + API + 静态全在 ECS，Cloudflare 前置） | 起步最快；1.3 GB 客户端 + PlayDH 走 ECS 带宽，等真有量再切 COS/OSS |
| 视觉调性 | **古风暗黑金**（深底 + 烫金 + 朱红 + 衬线中文标题） | 致敬 2008 墨香官方站调性；与 modern 客户端 UI 风格延续 |

---

## 2. 架构

```
                    ┌──────────────────────────┐
                    │ Cloudflare Tunnel        │
                    │ broker.52trz.com         │
                    │ (or portal.52trz.com)    │
                    └─────────────┬────────────┘
                                  │ HTTPS / 443
                                  ▼
                    ┌──────────────────────────┐
                    │ ECS 8443  (already used) │
                    │ ECS 8080  (NEW: portal)  │
                    └─────────────┬────────────┘
                                  │
            ┌─────────────────────┼─────────────────────┐
            ▼                     ▼                     ▼
    ┌───────────────┐     ┌───────────────┐     ┌───────────────┐
    │  Login 16001  │     │  Agent 17001  │     │  Map 18001    │
    │  (status)     │     │  (status)     │     │  (status)     │
    └───────────────┘     └───────────────┘     └───────────────┘
            ▲                     ▲                     ▲
            └─────────────────────┴─────────────────────┘
                                  │  ping 127.0.0.1
                                  │
                          ┌───────┴────────┐
                          │ modern_portal  │ ◀── modern/src/portal (C++17)
                          │   HTTP gateway │
                          │   /api/*       │
                          │   /download/*  │
                          │   /static/*    │
                          └───────┬────────┘
                                  │  reuses
                                  ▼
                    ┌──────────────────────────┐
                    │ modern/src/server/       │
                    │  account_service.{hpp,cpp}│  ← PBKDF2 已存在
                    │  IDbAdapter (sqlite/mssql)│
                    │  MLOG                     │
                    │  memory::ObjectPool       │
                    └──────────────────────────┘
```

### 2.1 现代侧（modern/）

新增 `modern/src/portal/` —— 一个独立的 HTTP 网关二进制 `mxh_portal.exe`，监听 `0.0.0.0:8080`（可配）。

**只做四件事**：
1. 复用 `mxh::server::create_account` / `verify_account_password` / `ensure_account_user_idx` 做注册与登录
2. 读 `chr_log_info` 给 `usepoint`（未来可写）
3. 周期 ping 三个游戏端口，写 `/api/status`
4. 静态分发 + 路由

**绝不做**：
- 重新实现密码哈希（直接调 `account_service`）
- 修改 `account_service` 的接口
- 触碰游戏 wire / LoginHandler 协议
- 引入新的网络栈（用 cpp-httplib，单 header，已是 mature）

### 2.2 前端（web/）

Vue 3 + Vite + TypeScript + TailwindCSS。Vite 产物落到 `deploy/portal/static/`，由 portal 进程直接静态分发。

页面（vue-router）：
- `/` Home —— Hero + 服务器状态条 + 最新 3 条新闻 + 注册/下载 CTA
- `/news` `/news/:slug` —— 新闻列表与详情
- `/register` `/login` —— 表单，含 i18n
- `/account` —— 登录后的 profile（账号、点券余额、最后登录 IP/时间）
- `/shop` —— 只读商城（按 category 过滤，分页）
- `/download` —— 客户端下载 + AutoPatcher manifest 链接 + SHA-256
- `/status` —— 详细服务器状态
- `/about` —— 游戏介绍、服务器规则、Q&A

i18n：vue-i18n v9，开箱支持中/英（`zh-CN` / `en-US`），右上角切换。

### 2.3 静态资源分布

| 资源 | 位置 | 来源 |
|---|---|---|
| `web/dist/`（JS/CSS/HTML/icons） | `deploy/portal/static/dist/` | Vite build 产出 |
| 客户端安装包 | `deploy/portal/static/downloads/MoxianClientSetup.zip` | 用户手动放（CI 出） |
| AutoPatcher manifest | `deploy/portal/static/downloads/manifest.json` | 启动时由 portal 读 `deploy/portal/patches/manifest.json`，按需重写 URL |
| 商城物品图 | `deploy/portal/static/shop/<idx>.webp` | 从 `墨香【源码配套资源】/PlayDH/Resource/UI/Item/` 抽帧导出（先做 12 个代表性商品） |
| 新闻封面 | `deploy/portal/static/news/<slug>.webp` | 站长手动投放 |

---

## 3. HTTP API 契约

所有 `/api/*` 返 JSON；`/download/*` 返二进制或 302；`/static/*` 静态。

### 3.1 认证

| Method | Path | Body | 200/201 | 4xx |
|---|---|---|---|---|
| POST | `/api/auth/register` | `{account, password, confirm}` | `{user_idx, account}` | 400 invalid / 400 weak / 409 exists / 429 rate-limited |
| POST | `/api/auth/login` | `{account, password}` | `{token, user_idx, account, points}` | 401 bad-creds / 429 rate-limited |
| GET  | `/api/auth/me` | Bearer | `{account, user_idx, points, registerdate, lastlogindate, lastloginip}` | 401 |
| POST | `/api/auth/logout` | Bearer | 204 | 401 |

规则：
- 账号：3-16 位 ASCII 字母/数字/下划线（沿用 `valid_account_name`）
- 密码：8-16 位可打印 ASCII，至少 1 字母 1 数字（沿用 `valid_account_password`）
- JWT：HS256，24h 过期，secret 来自 `PORTAL_JWT_SECRET` 环境变量
- 限流：每 IP 每分钟 5 次 register、10 次 login（token bucket，in-memory，多实例时切 Redis，但起步先 in-memory）
- Token 存法：默认 `Authorization: Bearer <token>`；登录页支持 "记住我" → 写 HttpOnly Secure SameSite=Lax cookie（7 天）

### 3.2 服务器状态

| Method | Path | Body | 200 |
|---|---|---|---|
| GET | `/api/status` | – | `{login: "up"|"down"|"unknown", agent: ..., map: ..., online_count: int\|null, last_check_at: ISO8601, version: "1.0.0"}` |

实现：portal 启动一个 `std::thread` 每 5s 异步 connect 三个端口，更新内存中的 `StatusSnapshot`，加锁读。

### 3.3 新闻

| Method | Path | Body | 200 |
|---|---|---|---|
| GET | `/api/news?page=1&size=10` | – | `{items: [{id, slug, title_zh, title_en, summary_zh, summary_en, hero_image, published_at}], page, total}` |
| GET | `/api/news/:slug` | – | `{..., body_zh, body_en}` |

数据源：`deploy/portal/content/news/*.md`，front-matter + 双语 body。portal 启动时扫一次缓存到内存。**M5 阶段不做管理后台**——站长直接编辑 markdown 文件 + `scp` 上去（更接近 1:1 时代的运营手感，也避免后台权限/审计/防注入的额外工程）。

### 3.4 商城

| Method | Path | Body | 200 |
|---|---|---|---|
| GET | `/api/shop/items?category=hair\|weapon\|armor\|consumable&page=1&size=24` | – | `{items: [{idx, name_zh, name_en, category, price_points, image_url, description_zh, description_en}], page, total}` |

数据源：`modern/data/shop/catalog.json` —— 一份手动维护的精选目录（M5 选 24-36 件代表性商品），`price_points` 对齐原版 `chr_log_info.usepoint` 充值点。展示用，无下单接口。

### 3.5 下载

| Method | Path | 行为 |
|---|---|---|
| GET | `/download/client` | 302 → `/static/downloads/MoxianClientSetup-<version>.zip`，附 `X-Content-Sha256` |
| GET | `/download/manifest.json` | 直接返回 `deploy/portal/patches/manifest.json`（AutoPatcher 协议） |
| GET | `/download/checksums.txt` | `sha256sum` 格式的文件清单 |

---

## 4. 视觉系统

### 4.1 调色板

| Token | Hex | 用途 |
|---|---|---|
| `--ink-void` | `#0a0807` | 主背景 |
| `--ink-deep` | `#13100d` | 二级背景（卡片） |
| `--ink-edge` | `#1f1a14` | 分隔线 |
| `--gold-base` | `#c9a76a` | 烫金主色 |
| `--gold-deep` | `#8a6d3a` | 烫金深色（边框/阴影） |
| `--gold-glow` | `#e8c984` | 烫金高光（hover/focus） |
| `--crimson` | `#a8324a` | 朱红强调（CTA、错误、徽章） |
| `--parchment` | `#e8d9b3` | 正文文字 |
| `--parchment-dim` | `#a89a7a` | 次要文字 |

### 4.2 字体

- 中文标题：`Noto Serif SC`（思源宋体 Google Fonts CDN，subset weights 400/700）
- 中文正文：`Noto Sans SC`（subset 400/500）
- 英文/数字：`Cinzel`（标题，serif, 古风雕刻感）+ `Inter`（正文）
- 全部 Google Fonts 加载 + `font-display: swap`，不引入 30MB 全字库

### 4.3 组件基调

- **GoldButton**：2px `--gold-deep` 描边 + `--gold-base` 文字 + hover `--gold-glow` 描边 + 朱红光晕 0 8px 24px
- **OrnateDivider**：SVG 卷草纹，宽度 200px，金色渐变
- **ServerStatusBar**：顶部 32px 暗金条，三圆点（绿/红/灰）标识 Login/Agent/Map
- **HeroBanner**：全宽 1920×720，左侧衬线大标题 + 副标题 + 双 CTA，右侧角色立绘（从 `墨香【源码配套资源】/PlayDH/Resource/UI/StartImage/` 抽 3 张做轮播，懒加载）

### 4.4 动效

- 全站慢节奏：hover transition 200ms ease-out，背景粒子金粉（CSS 动画 4s 缓动）
- 不用爆炸/弹跳/3D 翻页
- 新闻列表滚动 reveal 用 `IntersectionObserver`，一次性播放

---

## 5. 仓库落点

```
modern/
├── src/portal/
│   ├── CMakeLists.txt
│   ├── main.cpp                       # portal 入口
│   ├── http_server.{hpp,cpp}          # cpp-httplib 包装 + 中间件
│   ├── auth_routes.{hpp,cpp}          # /api/auth/*
│   ├── status_routes.{hpp,cpp}        # /api/status + 后台 ping 线程
│   ├── news_routes.{hpp,cpp}          # /api/news
│   ├── shop_routes.{hpp,cpp}          # /api/shop/items
│   ├── download_routes.{hpp,cpp}      # /download/*
│   ├── static_handler.{hpp,cpp}       # SPA fallback + Range
│   ├── jwt_token.{hpp,cpp}            # jwt-cpp 包装
│   ├── rate_limiter.{hpp,cpp}         # token bucket
│   ├── content_loader.{hpp,cpp}       # 扫 news/*.md + shop/catalog.json
│   ├── config.hpp                     # env -> struct
│   └── portal_log.hpp
├── tests/unit/portal/
│   ├── auth_route_test.cpp
│   ├── jwt_test.cpp
│   ├── rate_limiter_test.cpp
│   ├── status_route_test.cpp
│   ├── news_route_test.cpp
│   ├── shop_route_test.cpp
│   └── download_route_test.cpp
├── third_party/
│   ├── httplib/httplib.h              # cpp-httplib v0.15.x (MIT)
│   ├── nlohmann/json.hpp              # v3.11.x (MIT)
│   └── jwt-cpp/jwt.h                  # v0.7.x (MIT)
├── data/portal/
│   ├── content/news/*.md              # 双语新闻 markdown
│   └── shop/catalog.json              # 商城精选目录
└── tools/MoxianPortal/
    ├── CMakeLists.txt
    └── main.cpp                       # 链接 modern/src/portal → mxh_portal.exe

web/
├── package.json
├── pnpm-lock.yaml
├── vite.config.ts
├── tsconfig.json
├── tailwind.config.js
├── postcss.config.js
├── index.html
├── README.md
└── src/
    ├── main.ts
    ├── App.vue
    ├── env.d.ts
    ├── router/index.ts
    ├── stores/
    │   ├── auth.ts
    │   └── i18n.ts
    ├── api/
    │   ├── client.ts                  # ofetch 包装 + 401 自动跳登录
    │   ├── auth.ts
    │   ├── status.ts
    │   ├── news.ts
    │   └── shop.ts
    ├── views/
    │   ├── HomeView.vue
    │   ├── NewsView.vue
    │   ├── NewsDetailView.vue
    │   ├── RegisterView.vue
    │   ├── LoginView.vue
    │   ├── AccountView.vue
    │   ├── ShopView.vue
    │   ├── DownloadView.vue
    │   ├── StatusView.vue
    │   ├── AboutView.vue
    │   └── NotFoundView.vue
    ├── components/
    │   ├── SiteHeader.vue
    │   ├── SiteFooter.vue
    │   ├── ServerStatusBar.vue
    │   ├── HeroBanner.vue
    │   ├── NewsCard.vue
    │   ├── ShopItemCard.vue
    │   ├── GoldButton.vue
    │   ├── OrnateDivider.vue
    │   └── LocaleSwitcher.vue
    ├── i18n/
    │   ├── index.ts
    │   ├── zh-CN.ts
    │   └── en-US.ts
    └── assets/css/
        ├── tailwind.css
        └── theme.css

deploy/portal/
├── start_portal.ps1                   # Start-Process mxh_portal.exe
├── stop_portal.ps1
├── static/
│   ├── dist/                          # Vite 产物
│   ├── downloads/                     # 客户端 + manifest
│   ├── news/                          # 新闻封面图
│   └── shop/                          # 商城物品图
├── content/news/                      # 实际生效的新闻 md（部署侧维护）
└── shop/catalog.json                  # 实际生效的商城目录
```

---

## 6. 关键依赖（全部 MIT / Apache 2.0，无授权风险）

| 组件 | 版本 | License | 来源 |
|---|---|---|---|
| cpp-httplib | 0.15.x | MIT | https://github.com/yhirose/cpp-httplib |
| nlohmann/json | 3.11.x | MIT | https://github.com/nlohmann/json |
| jwt-cpp | 0.7.x | MIT | https://github.com/Thalhammer/jwt-cpp |
| Vue | 3.5.x | MIT | npm |
| Vite | 5.x | MIT | npm |
| TypeScript | 5.x | Apache 2.0 | npm |
| TailwindCSS | 3.x | MIT | npm |
| vue-router | 4.x | MIT | npm |
| pinia | 2.x | MIT | npm |
| vue-i18n | 9.x | MIT | npm |
| ofetch | 1.x | MIT | npm |
| vee-validate | 4.x + zod | MIT | npm |
| @headlessui/vue | 1.x | MIT | npm |

不引入：
- Element Plus / Naive UI（太"管理后台"）
- axios（用 ofetch 更轻）
- i18next / react-intl（vue-i18n 够用）
- 任何 Tailwind UI / shadcn 模板（避免 license 模糊）

---

## 7. 里程碑（M5）

按 1 commit = 1 单元拆，可单独 cherry-pick。

| 编号 | 内容 | 验收 |
|---|---|---|
| **M5.1** | portal 骨架：cpp-httplib 接入 + /api/healthz + /static/* + 启动日志 + CMake 集成 | `cmake --build` 0 错；ctest 新增 healthz 测试 PASS；`mxh_portal.exe` 起来后 `curl /api/healthz` 返 200 |
| **M5.2** | jwt_token + rate_limiter（含单测） | 10 个单测全过；rate limiter 5req/min 行为锁定 |
| **M5.3** | /api/auth（register/login/me/logout），复用 `account_service` | 注册 + 登录 + me 闭环 E2E 测试；PBKDF2 真实落到 `chr_log_info`；rate-limit 触发返 429；JWT 验签失败 401 |
| **M5.4** | /api/status + 后台 ping 线程 | mock Login/Agent/Map 端口测试；up/down 切换 5s 内反映 |
| **M5.5** | /api/news + content_loader 扫 markdown | 双语 front-matter 解析；不存在的 slug 返 404；3 篇示例新闻 |
| **M5.6** | /api/shop/items + 24 件示例目录 | category 过滤 + 分页；catalog.json 缺失返 503 |
| **M5.7** | /download/*（client/manifest/checksums） | manifest 字段对齐 AutoPatcher 协议；checksums SHA-256 校验通过 |
| **M5.8** | 前端骨架：Vite + Vue 3 + TS + Tailwind + vue-router + pinia + i18n + theme.css | `npm run build` 成功；`pnpm dev` 起 dev server；dist 落到 `deploy/portal/static/dist/` |
| **M5.9** | HomeView + NewsView + NewsDetailView + StatusView | 静态数据驱动；server status 实拉接口；中英切换 |
| **M5.10** | RegisterView + LoginView + AccountView | 真实调 /api/auth；表单校验；401 自动跳登录 |
| **M5.11** | ShopView + DownloadView + AboutView + 404 | 商城卡片栅格；下载页 SHA-256 展示 |
| **M5.12** | HeroBanner + 角色立绘（从 PlayDH 抽 3 张）+ 动效 | PlayDH 图片抽帧脚本 `tools/extract_hero_images.py`；视觉验收 |
| **M5.13** | ECS 部署：`deploy/portal/start_portal.ps1` + Cloudflare tunnel 路由 + 烟雾测试 | 从外网 `curl https://broker.52trz.com/portal/` 返 200；/api/status 返 3 端口 up |
| **M5.14** | 文档：`docs/PORTAL_API.md`（API 参考）+ `docs/PORTAL_DEPLOY.md`（ECS 部署） + 更新 ROADMAP.md §3 M5 | 中英双语；`scripts/commercial-smoke.ps1` 加 portal 烟雾段 |

**完成判据**：
- `cmake --build modern/build --config Debug` 0 错
- `ctest -C Debug --output-on-failure` 全过（含 M5 新增 ~50 个单测）
- `pnpm --dir web build` 成功
- `deploy/scripts/start_modern.ps1 -Mode start`（三服）+ `deploy/portal/start_portal.ps1` 起来后：
  - `curl http://127.0.0.1:8080/api/healthz` → 200
  - `curl http://127.0.0.1:8080/api/status` → 3 up
  - 浏览器 `http://127.0.0.1:8080/` → 古风暗黑金首页，hero banner 显示
  - 注册一个测试账号 → 收到 JWT → `/account` 看到自己 → `/shop` 看到余额
- ECS 部署：`https://broker.52trz.com/portal/`（或新域名）从公网访问 200
- 1.0 release tag 之前的"待办"减少一项（ROADMAP §2 注册 Web/桌面入口 行）

---

## 8. 风险与对策

| 风险 | 影响 | 对策 |
|---|---|---|
| cpp-httplib 阻塞 I/O 多连接并发差 | 高并发下延迟 | 用线程池（cpp-httplib 内置 `ThreadPool`） + 限流；峰值 <100 并发可接受 |
| ECS 1.3GB 客户端带宽成本 | 月度账单 | 起步压低客户端 zip（只带 ModernClient + 必要脚本）；等真有量再切 COS+CDN |
| Cloudflare tunnel 多个 hostname | 配置复杂度 | 复用 `broker.52trz.com` 加 path 路由 `/portal/*`；M5.14 决定 |
| 新闻 markdown 注入 | XSS | 前端 `v-text`/`{{}}` 全部走 Vue 文本插值；不接 v-html（管理员可控但用 fenced code block 即可） |
| JWT secret 泄漏 | 全账号被伪造 | secret 走环境变量（不进仓库、不进 AI 对话）；`scripts/gen-jwt-secret.ps1` 启动时在 ECS 现场生成 |
| 注册接口被脚本刷 | DB 满 | rate-limit + turnstile（hCaptcha 自部署）→ M6 加；M5 先 IP 限流 |
| Vue 3 中文/英文文案 | 双语一致 | 单一 `zh-CN.ts` / `en-US.ts` 同步走 PR review；CI 漏 key 报警（M5.14 加） |

---

## 9. 不做（明确范围外）

- ❌ 支付集成（支付宝/微信/Stripe）—— M6
- ❌ GM 工具 web 化 —— MoxianGMTool 已存在，UI 是桌面
- ❌ 后台管理（新闻/商城可视化编辑）—— 站长直接维护 markdown + JSON
- ❌ 移动端 App / 小程序 —— 网站响应式足够
- ❌ 邮件验证 / 短信验证 —— M5 注册即时通过；M6 加
- ❌ 密码重置（忘记密码）—— M6，需要邮件通道
- ❌ 角色面板（在线角色、装备、战绩）—— 玩家自己登客户端看
- ❌ 论坛 / 社区 / Discord 嵌入 —— 独立项目
- ❌ i18n 第三种语言 —— 中/英两种

---

## 10. 下一步

1. 用户 review 本方案 → 同意 / 调整
2. M5.1 portal 骨架开搞（`modern/src/portal/CMakeLists.txt` + 三个第三方 header）
3. 同步在 `ROADMAP.md` §3 新增"M5：玩家门户"段
4. 每个里程碑完成跑 `cmake --build` + `ctest` + `pnpm build`（如涉及前端）
5. M5.13 部署后做 ECS 实地烟雾测试
6. M5.14 出 release notes 进 `docs/CHANGELOG.md`

---

## 附 A. 关键代码契约（速查）

```cpp
// 复用现有 — 不改
namespace mxh::server {
    bool valid_account_name(std::string_view) noexcept;
    bool valid_account_password(std::string_view) noexcept;
    AccountCreateResult create_account(IDbAdapter&, std::string_view, std::string_view);
    bool verify_account_password(std::string_view password, std::string_view stored) noexcept;
    std::uint32_t ensure_account_user_idx(IDbAdapter&, std::string_view);
}

// 新增 — portal 侧
namespace mxh::portal {
    struct Config { std::string bind = "0.0.0.0"; uint16_t port = 8080;
                    std::string db_backend; std::string db_path;
                    std::string jwt_secret; std::string static_root;
                    std::string content_root;
                    uint16_t game_login_port = 16001;
                    uint16_t game_agent_port = 17001;
                    uint16_t game_map_port   = 18001; };
    int run(const Config&);   // blocks; returns 0 on SIGINT clean exit
}
```

```ts
// web/src/api/client.ts
export const api = ofetch.create({
  baseURL: import.meta.env.VITE_API_BASE ?? '',
  onResponse({ response }) {
    if (response.status === 401) {
      const auth = useAuthStore();
      auth.clear();
      if (router.currentRoute.value.name !== 'login') {
        router.push({ name: 'login', query: { from: router.currentRoute.value.fullPath } });
      }
    }
  },
});
```

---

## 附 B. ROADMAP 增量（待合并到 ROADMAP.md §3）

新增段：
```
### M5：玩家门户站点（Player Portal）

modern 侧 + 前端 + 单 ECS 部署，覆盖 注册 / 登录 / 商城（展示型）/ 下载 / 新闻 / 服务器状态。
- 复用 `mxh::server::account_service` 做 PBKDF2 注册登录，零密码学重复
- 引入 cpp-httplib + nlohmann/json + jwt-cpp（均 MIT）作为 portal HTTP 栈
- 前端 Vue 3 + Vite + TailwindCSS + vue-i18n（zh-CN / en-US）
- 视觉：古风暗黑金（#0a0807 底 + #c9a76a 烫金 + #a8324a 朱红）
- 单 ECS 部署 + Cloudflare tunnel 前置（路径 /portal/*）

完成判据：见 docs/PLAN_PORTAL.md §7
```

ROADMAP §2 当前状态表新增一行：
```
| 玩家门户 | M5 范围 6 页面（注册/登录/商城/下载/新闻/状态）已交付；JWT 24h；PBKDF2 复用 account_service；
Vue 3 前端 build 通过；ECS 部署烟雾测试 PASS | M6 接支付 / 密码重置 / 邮件验证 | **M5 modern 闭环完成** |
```
