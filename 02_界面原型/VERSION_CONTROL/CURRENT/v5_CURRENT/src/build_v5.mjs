import { createRequire } from "node:module";
import { pathToFileURL } from "node:url";
import path from "node:path";
import fs from "node:fs/promises";

const require = createRequire(import.meta.url);
const artifactToolPath = require.resolve("@oai/artifact-tool");
const artifactRequire = createRequire(pathToFileURL(artifactToolPath));
const artifact = await import(pathToFileURL(artifactToolPath).href);
const skia = await import(pathToFileURL(artifactRequire.resolve("skia-canvas")).href);

const {
  Presentation,
  PresentationFile,
  drawSlideToCtx,
  buildPresentationLayoutExport,
  row,
  column,
  grid,
  layers,
  panel,
  text,
  shape,
  rule,
  fill,
  hug,
  fixed,
  wrap,
  grow,
  fr,
  auto,
} = artifact;

const { Canvas } = skia;

const root = path.resolve("02_界面原型/serial_screen_tjc_native_ppt_v5");
const pptDir = path.join(root, "ppt");
const previewDir = path.join(root, "previews");
const reportDir = path.join(root, "reports");

const W = 800;
const H = 480;

const C = {
  bg: "#F7F8F4",
  paper: "#FFFFFF",
  ink: "#18231F",
  muted: "#66746D",
  hair: "#DCE4DE",
  soft: "#EEF3EF",
  accent: "#168A6B",
  accentDark: "#0F6652",
  accentSoft: "#DDF1E9",
  warn: "#D9892B",
  danger: "#C6544A",
  blue: "#486F8F",
  slate: "#344B5A",
  transparent: "#FFFFFF00",
};

const font = "Microsoft YaHei";

const pages = [
  {
    no: 1,
    name: "boot",
    title: "智能衣橱",
    eyebrow: "系统启动",
    status: "初始化中",
    action: "请稍候",
    nav: "home",
    hero: "01",
    tags: ["主控通信", "K230 网络", "激光测距"],
  },
  {
    no: 2,
    name: "home",
    title: "智能衣橱",
    eyebrow: "首页",
    status: "系统就绪",
    action: "选择一个操作",
    nav: "home",
    primary: ["取衣", "归还"],
    secondary: ["护理", "浏览", "网络"],
  },
  {
    no: 3,
    name: "retrieve_select",
    title: "选择衣物",
    eyebrow: "取衣",
    status: "选择目标衣物",
    action: "确认后开始",
    nav: "retrieve",
    list: true,
  },
  {
    no: 4,
    name: "retrieve_progress",
    title: "取衣中",
    eyebrow: "执行流程",
    status: "正在取衣",
    action: "请勿遮挡设备",
    nav: "retrieve",
    progress: 0.45,
    steps: ["定位", "夹取", "移出"],
  },
  {
    no: 5,
    name: "retrieve_done",
    title: "取衣完成",
    eyebrow: "完成",
    status: "衣物已送出",
    action: "请取走衣物",
    nav: "retrieve",
    done: true,
  },
  {
    no: 6,
    name: "return_prepare",
    title: "归还准备",
    eyebrow: "归还",
    status: "放置衣物",
    action: "挂到归还杆后开始",
    nav: "return",
    prompt: true,
  },
  {
    no: 7,
    name: "return_progress",
    title: "归还中",
    eyebrow: "执行流程",
    status: "正在归还",
    action: "系统自动识别",
    nav: "return",
    progress: 0.62,
    steps: ["伸出", "检测", "识别", "入库"],
  },
  {
    no: 8,
    name: "return_done",
    title: "归还完成",
    eyebrow: "完成",
    status: "已入库",
    action: "信息已更新",
    nav: "return",
    done: true,
  },
  {
    no: 9,
    name: "treat_select",
    title: "护理选择",
    eyebrow: "护理",
    status: "选择护理模式",
    action: "选择后开始护理",
    nav: "treat",
    modes: ["热风", "负离子", "组合"],
  },
  {
    no: 10,
    name: "treat_progress",
    title: "护理中",
    eyebrow: "执行流程",
    status: "正在护理",
    action: "完成后自动停止",
    nav: "treat",
    progress: 0.58,
    steps: ["预热", "护理", "收尾"],
  },
  {
    no: 11,
    name: "treat_done",
    title: "护理完成",
    eyebrow: "完成",
    status: "护理结束",
    action: "可继续操作",
    nav: "treat",
    done: true,
  },
  {
    no: 12,
    name: "browse",
    title: "衣物浏览",
    eyebrow: "云端衣物库",
    status: "浏览衣物",
    action: "查看云端衣物库",
    nav: "browse",
    list: true,
  },
  {
    no: 13,
    name: "network",
    title: "网络设置",
    eyebrow: "Wi-Fi",
    status: "选择网络",
    action: "扫描附近 Wi-Fi",
    nav: "network",
    wifi: true,
  },
  {
    no: 14,
    name: "wifi_password",
    title: "输入密码",
    eyebrow: "Wi-Fi",
    status: "输入密码",
    action: "连接选中网络",
    nav: "network",
    password: true,
  },
  {
    no: 15,
    name: "error",
    title: "异常提示",
    eyebrow: "提示",
    status: "需要确认",
    action: "处理后继续",
    nav: "home",
    error: true,
  },
];

const navItems = [
  ["home", "首页"],
  ["retrieve", "取衣"],
  ["return", "归还"],
  ["treat", "护理"],
  ["browse", "浏览"],
  ["network", "网络"],
];

function t(value, options = {}) {
  return text(value, {
    width: options.width ?? fill,
    height: options.height ?? hug,
    name: options.name,
    style: {
      fontFace: font,
      fontSize: options.size ?? 20,
      bold: options.bold ?? false,
      color: options.color ?? C.ink,
      horizontalAlignment: options.align,
      verticalAlignment: options.valign,
    },
  });
}

function pill(label, opts = {}) {
  return panel(
    {
      width: opts.width ?? hug,
      height: opts.height ?? fixed(32),
      padding: { x: opts.px ?? 14, y: 5 },
      fill: opts.fill ?? C.soft,
      line: opts.line ?? { color: C.hair, weight: 1 },
      borderRadius: "rounded-full",
      align: "center",
      justify: "center",
      name: opts.name,
    },
    t(label, {
      width: opts.width ? fill : hug,
      size: opts.size ?? 14,
      bold: opts.bold ?? true,
      color: opts.color ?? C.muted,
      align: "center",
    }),
  );
}

function button(label, opts = {}) {
  return panel(
    {
      width: opts.width ?? fill,
      height: opts.height ?? fixed(72),
      padding: { x: 20, y: 14 },
      fill: opts.primary ? C.accent : C.paper,
      line: { color: opts.primary ? C.accent : C.hair, weight: 1.2 },
      borderRadius: 10,
      align: "center",
      justify: "center",
      name: opts.name,
    },
    t(label, {
      size: opts.primary ? 30 : 22,
      bold: true,
      color: opts.primary ? "#FFFFFF" : C.ink,
      align: "center",
    }),
  );
}

function nav(active) {
  return row(
    {
      name: "bottom-nav",
      width: fill,
      height: fixed(42),
      gap: 4,
      padding: { x: 10, y: 0 },
      align: "center",
      justify: "center",
    },
    navItems.map(([key, label]) =>
      panel(
        {
          name: `nav-${key}`,
          width: fixed(104),
          height: fixed(34),
          padding: { x: 0, y: 6 },
          fill: key === active ? C.accentSoft : C.transparent,
          borderRadius: "rounded-full",
          align: "center",
          justify: "center",
        },
        t(label, {
          size: 14,
          bold: key === active,
          color: key === active ? C.accentDark : C.muted,
          align: "center",
        }),
      ),
    ),
  );
}

function header(page) {
  return row(
    {
      name: "header",
      width: fill,
      height: fixed(64),
      align: "center",
      justify: "between",
      gap: 20,
    },
    [
      column({ width: fill, height: hug, gap: 5 }, [
        t(page.eyebrow, {
          name: "eyebrow",
          width: fill,
          size: 13,
          bold: true,
          color: C.accent,
        }),
        t(page.title, {
          name: "slide-title",
          width: fill,
          size: 32,
          bold: true,
          color: C.ink,
        }),
      ]),
      pill(page.no.toString().padStart(2, "0"), {
        width: fixed(46),
        height: fixed(28),
        fill: C.soft,
        color: C.muted,
        line: { color: C.hair, weight: 1 },
      }),
    ],
  );
}

function summary(page) {
  return column(
    {
      name: "summary",
      width: fill,
      height: hug,
      gap: 4,
    },
    [
      t(page.status, {
        name: "status-line",
        width: fill,
        size: 37,
        bold: true,
      }),
      t(page.action, {
        name: "action-line",
        width: fill,
        size: 16,
        color: C.muted,
      }),
    ],
  );
}

function progressBlock(page) {
  const progress = page.progress ?? 0.5;
  return column(
    {
      name: "progress-block",
      width: fill,
      height: fill,
      gap: 24,
      justify: "center",
    },
    [
      t(`${Math.round(progress * 100)}%`, {
        name: "progress-number",
        width: fill,
        size: 54,
        bold: true,
        color: C.accentDark,
        align: "center",
      }),
      panel(
        {
          width: fill,
          height: fixed(20),
          fill: C.soft,
          borderRadius: "rounded-full",
          align: "start",
          justify: "center",
        },
        shape({
          name: "progress-fill-placeholder",
          width: fixed(Math.round(700 * progress)),
          height: fixed(20),
          fill: C.accent,
          borderRadius: "rounded-full",
        }),
      ),
      row(
        {
          width: fill,
          height: fixed(40),
          gap: 8,
          align: "center",
        },
        (page.steps ?? []).map((step, idx) =>
          panel(
            {
              width: fill,
              height: fixed(36),
              padding: { x: 8, y: 8 },
              fill: idx === 0 ? C.accentSoft : C.paper,
              line: { color: C.hair, weight: 1 },
              borderRadius: "rounded-full",
              align: "center",
              justify: "center",
            },
            t(step, {
              size: 15,
              bold: true,
              color: idx === 0 ? C.accentDark : C.muted,
              align: "center",
            }),
          ),
        ),
      ),
    ],
  );
}

function listBlock(page) {
  return column(
    {
      name: "list-block",
      width: fill,
      height: fill,
      gap: 12,
    },
    [0, 1, 2].map((idx) =>
      row(
        {
          width: fill,
          height: fixed(66),
          gap: 16,
          padding: { x: 0, y: 0 },
          align: "center",
        },
        [
          shape({
            name: `thumb-placeholder-${idx + 1}`,
            width: fixed(66),
            height: fixed(66),
            fill: idx === 0 ? C.accentSoft : C.paper,
            line: { color: C.hair, weight: 1 },
            borderRadius: 12,
          }),
          column({ width: fill, height: hug, gap: 2 }, [
            t(idx === 0 ? "衣物名称" : "待加载", {
              name: `item-title-${idx + 1}`,
              size: 18,
              bold: true,
              color: idx === 0 ? C.ink : C.muted,
            }),
            t(idx === 0 ? "类别 / 挂位 / 状态" : "云端数据", {
              name: `item-meta-${idx + 1}`,
              size: 12,
              color: C.muted,
            }),
          ]),
        ],
      ),
    ),
  );
}

function homeBlock(page) {
  return column(
    { name: "home-actions", width: fill, height: fill, gap: 10, justify: "center" },
    [
      row({ width: fill, height: fixed(100), gap: 18 }, [
        button(page.primary[0], { primary: true, height: fixed(98), name: "btn-retrieve" }),
        button(page.primary[1], { primary: true, height: fixed(98), name: "btn-return" }),
      ]),
      row(
        { width: fill, height: fixed(56), gap: 10 },
        page.secondary.map((item) => button(item, { height: fixed(52), name: `btn-${item}` })),
      ),
      row({ width: fill, height: fixed(34), gap: 8 }, [
        pill("K230", { width: fill, height: fixed(30), fill: C.soft }),
        pill("主控", { width: fill, height: fixed(30), fill: C.soft }),
        pill("-- mm", { width: fill, height: fixed(30), fill: C.soft }),
      ]),
    ],
  );
}

function wifiBlock() {
  return column(
    { name: "wifi-list", width: fill, height: fill, gap: 14, justify: "center" },
    ["Wi-Fi 名称", "信号强度", "连接状态"].map((item, idx) =>
      row(
        {
          width: fill,
          height: fixed(56),
          gap: 12,
          align: "center",
        },
        [
          pill(idx === 0 ? "当前" : "可选", {
            width: fixed(62),
            fill: idx === 0 ? C.accentSoft : C.paper,
            color: idx === 0 ? C.accentDark : C.muted,
          }),
          t(item, { size: 18, bold: idx === 0, color: C.ink }),
        ],
      ),
    ),
  );
}

function passwordBlock() {
  return column(
    { name: "password-block", width: fill, height: fill, gap: 16, justify: "center" },
    [
      panel(
        {
          width: fill,
          height: fixed(58),
          fill: C.paper,
          line: { color: C.hair, weight: 1 },
          borderRadius: 10,
          padding: { x: 18, y: 15 },
        },
        t("SSID", { size: 20, bold: true }),
      ),
      panel(
        {
          width: fill,
          height: fixed(58),
          fill: C.paper,
          line: { color: C.hair, weight: 1 },
          borderRadius: 10,
          padding: { x: 18, y: 15 },
        },
        t("请输入密码", { size: 20, color: C.muted }),
      ),
      row({ width: fill, height: fixed(56), gap: 12 }, [
        button("连接", { primary: true, height: fixed(54) }),
        button("取消", { height: fixed(54) }),
      ]),
    ],
  );
}

function modeBlock(page) {
  return row(
    { name: "mode-block", width: fill, height: fixed(96), gap: 14 },
    page.modes.map((mode, idx) =>
      panel(
        {
          width: fill,
          height: fixed(92),
          fill: idx === 0 ? C.accent : C.paper,
          line: { color: idx === 0 ? C.accent : C.hair, weight: 1 },
          borderRadius: 12,
          align: "center",
          justify: "center",
          padding: { x: 12, y: 16 },
        },
        t(mode, {
          size: 24,
          bold: true,
          color: idx === 0 ? "#FFFFFF" : C.ink,
          align: "center",
        }),
      ),
    ),
  );
}

function doneBlock(page) {
  const color = page.error ? C.danger : C.accent;
  return row(
    { name: "done-block", width: fill, height: fill, gap: 28, align: "center", justify: "center" },
    [
      panel(
        {
          width: fixed(118),
          height: fixed(118),
          fill: page.error ? "#F8E6E3" : C.accentSoft,
          borderRadius: "rounded-full",
          align: "center",
          justify: "center",
        },
        t(page.error ? "!" : "✓", {
          width: fill,
          size: 58,
          bold: true,
          color,
          align: "center",
        }),
      ),
      column({ width: fill, height: hug, gap: 8 }, [
        t(page.error ? "等待确认" : "已完成", {
          size: 42,
          bold: true,
          color,
        }),
        t(page.error ? "查看原因并处理" : "结果已同步", {
          size: 20,
          color: C.muted,
          width: wrap(360),
        }),
      ]),
    ],
  );
}

function promptBlock() {
  return panel(
    {
      name: "return-prompt",
      width: fill,
      height: fill,
      fill: C.paper,
      line: { color: C.hair, weight: 1 },
      borderRadius: 16,
      padding: { x: 28, y: 28 },
      align: "center",
      justify: "center",
    },
    row({ width: fill, height: fill, gap: 24, align: "center" }, [
      shape({
        width: fixed(92),
        height: fixed(92),
        fill: C.accentSoft,
        borderRadius: "rounded-full",
      }),
      column({ width: fill, height: hug, gap: 6 }, [
        t("归还杆就绪", { size: 34, bold: true }),
        t("挂好衣物后点击开始", { size: 20, color: C.muted }),
      ]),
    ]),
  );
}

function bootBlock(page) {
  return row(
    { name: "boot-block", width: fill, height: fill, gap: 26, align: "center", justify: "center" },
    [
      panel(
        {
          width: fixed(132),
          height: fixed(132),
          fill: C.accent,
          borderRadius: 24,
          align: "center",
          justify: "center",
        },
        t(page.hero, { size: 48, bold: true, color: "#FFFFFF", align: "center" }),
      ),
      column({ width: fill, height: hug, gap: 12 }, page.tags.map((tag) => pill(tag, { height: fixed(36), fill: C.paper }))),
    ],
  );
}

function content(page) {
  if (page.name === "boot") return bootBlock(page);
  if (page.name === "home") return homeBlock(page);
  if (page.list) return listBlock(page);
  if (page.wifi) return wifiBlock();
  if (page.password) return passwordBlock();
  if (page.modes) return modeBlock(page);
  if (page.progress) return progressBlock(page);
  if (page.done || page.error) return doneBlock(page);
  if (page.prompt) return promptBlock();
  return doneBlock(page);
}

function bgMarks() {
  return layers({ name: "background-marks", width: fill, height: fill }, [
    shape({
      name: "bg",
      width: fixed(W),
      height: fixed(H),
      fill: C.bg,
    }),
    shape({
      name: "accent-strip",
      width: fixed(8),
      height: fixed(H),
      fill: C.accent,
    }),
  ]);
}

function baseSlide(page) {
  return layers(
    { name: "slide-layers", width: fill, height: fill },
    [
      bgMarks(),
      column(
        {
          name: "screen-root",
          width: fill,
          height: fill,
          padding: { x: 34, y: 20 },
          gap: 12,
        },
        [
          header(page),
          row(
            {
              name: "status-row",
              width: fill,
              height: fixed(72),
              align: "center",
              justify: "between",
              gap: 18,
            },
            [
              summary(page),
              rule({ width: fixed(132), stroke: C.accent, weight: 4 }),
            ],
          ),
          panel(
            {
              name: "main-stage",
              width: fill,
              height: grow(1),
              fill: C.transparent,
              padding: { x: 0, y: 0 },
            },
            content(page),
          ),
          nav(page.nav),
        ],
      ),
    ],
  );
}

function controlsOverlay(page) {
  const labels = [
    ["title", 34, 20, 732, 64],
    ["status", 34, 96, 732, 72],
    ["runtime", 34, 180, 732, 226],
    ["nav/touch", 34, 418, 732, 42],
  ];
  const spacer = (width, height, name) =>
    shape({
      name,
      width: typeof width === "number" ? fixed(width) : width,
      height: typeof height === "number" ? fixed(height) : height,
      fill: C.transparent,
      line: { color: C.transparent, weight: 0 },
    });
  const controlBox = ([label, x, y, w, h]) =>
    column({ name: `ctl-layer-${label}`, width: fill, height: fill, gap: 0 }, [
      spacer(fill, y, `ctl-top-space-${label}`),
      row({ width: fill, height: fixed(h), gap: 0 }, [
        spacer(x, h, `ctl-left-space-${label}`),
        panel(
          {
            name: `ctl-${label}`,
            width: fixed(w),
            height: fixed(h),
            fill: "#FFFFFF10",
            line: { color: C.blue, weight: 1 },
            borderRadius: 6,
            padding: { x: 6, y: 3 },
          },
          t(`${page.name}:${label}`, {
            size: 10,
            bold: true,
            color: C.blue,
            width: fill,
          }),
        ),
        spacer(grow(1), h, `ctl-right-space-${label}`),
      ]),
      spacer(fill, grow(1), `ctl-bottom-space-${label}`),
    ]);
  return layers(
    { name: "controls-overlay", width: fill, height: fill },
    labels.map(controlBox),
  );
}

function composeSlide(slide, node) {
  slide.compose(node, {
    frame: { left: 0, top: 0, width: W, height: H },
    baseUnit: 8,
  });
}

function buildDeck(kind) {
  const presentation = Presentation.create({
    slideSize: { width: W, height: H },
  });
  for (const page of pages) {
    const slide = presentation.slides.add();
    const node =
      kind === "controls"
        ? layers({ name: "controls-root", width: fill, height: fill }, [baseSlide(page), controlsOverlay(page)])
        : baseSlide(page);
    composeSlide(slide, node);
  }
  return presentation;
}

async function saveDeck(presentation, file) {
  const blob = await PresentationFile.exportPptx(presentation);
  await blob.save(file);
}

async function renderDeck(presentation, prefix) {
  const outputs = [];
  for (let i = 0; i < presentation.slides.items.length; i++) {
    const slide = presentation.slides.items[i];
    const canvas = new Canvas(W, H);
    const ctx = canvas.getContext("2d");
    await drawSlideToCtx(slide, presentation, ctx);
    const png = await canvas.toBuffer("png");
    const out = path.join(previewDir, `${prefix}_${String(i + 1).padStart(2, "0")}_${pages[i].name}.png`);
    await fs.writeFile(out, png);
    outputs.push(out);
  }
  return outputs;
}

async function writeReports(presentation, prefix) {
  const reports = [];
  for (let i = 0; i < presentation.slides.items.length; i++) {
    const slide = presentation.slides.items[i];
    const layout = buildPresentationLayoutExport(slide, []);
    const out = path.join(reportDir, `${prefix}_${String(i + 1).padStart(2, "0")}_${pages[i].name}.layout.json`);
    await fs.writeFile(out, JSON.stringify(layout, null, 2), "utf8");
    reports.push(out);
  }
  return reports;
}

async function main() {
  await fs.mkdir(pptDir, { recursive: true });
  await fs.mkdir(previewDir, { recursive: true });
  await fs.mkdir(reportDir, { recursive: true });

  const backgrounds = buildDeck("backgrounds");
  const controls = buildDeck("controls");

  const bgPpt = path.join(pptDir, "serial_screen_user_backgrounds_v5.pptx");
  const ctlPpt = path.join(pptDir, "serial_screen_user_controls_plan_v5.pptx");

  await saveDeck(backgrounds, bgPpt);
  await saveDeck(controls, ctlPpt);
  const bgPreviews = await renderDeck(backgrounds, "backgrounds");
  const ctlPreviews = await renderDeck(controls, "controls");
  await writeReports(backgrounds, "backgrounds");
  await writeReports(controls, "controls");

  const manifest = {
    version: "v5",
    slideSize: { width: W, height: H },
    decks: {
      backgrounds: bgPpt,
      controls: ctlPpt,
    },
    previews: {
      backgrounds: bgPreviews,
      controls: ctlPreviews,
    },
    pages: pages.map(({ no, name, title, nav }) => ({ no, name, title, nav })),
  };
  await fs.writeFile(path.join(root, "manifest_v5.json"), JSON.stringify(manifest, null, 2), "utf8");
  console.log(JSON.stringify(manifest, null, 2));
}

await main();
process.exit(0);
