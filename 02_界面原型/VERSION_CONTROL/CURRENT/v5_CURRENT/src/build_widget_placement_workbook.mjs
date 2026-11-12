import fs from "node:fs/promises";
import path from "node:path";
import { createRequire } from "node:module";
import { pathToFileURL } from "node:url";

const require = createRequire(import.meta.url);
const artifactToolPath = require.resolve("@oai/artifact-tool");
const artifact = await import(pathToFileURL(artifactToolPath).href);
const { Workbook, SpreadsheetFile } = artifact;

const root = path.resolve("02_界面原型/VERSION_CONTROL/CURRENT/v5_CURRENT");
const refDir = path.join(root, "reference_tables");
const csvPath = path.join(refDir, "widget_placement_list_v5.csv");
const outPath = path.join(refDir, "widget_placement_list_v5.xlsx");

const csvText = await fs.readFile(csvPath, "utf8");
const workbook = await Workbook.fromCSV(csvText, { sheetName: "widget_placement" });
const output = await SpreadsheetFile.exportXlsx(workbook);
await output.save(outPath);

console.log(JSON.stringify({ outPath }, null, 2));
