import { createRequire } from "node:module";
import { pathToFileURL } from "node:url";

const require = createRequire(import.meta.url);
const artifactToolPath = require.resolve("@oai/artifact-tool");
const artifact = await import(pathToFileURL(artifactToolPath).href);

console.log(Boolean(artifact.Presentation), Boolean(artifact.PresentationFile));
