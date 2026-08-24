import { spawn } from "node:child_process";

try {
  await import("./migrate.mjs");
} catch (error) {
  console.error("Aethelgrad startup migration failed; API will not start.");
  console.error(error instanceof Error ? error.message : error);
  process.exit(1);
}

const api = spawn(process.execPath, ["src/server.mjs"], {
  env: process.env,
  stdio: "inherit"
});

api.on("error", (error) => {
  console.error("Aethelgrad API process failed to start.");
  console.error(error instanceof Error ? error.message : error);
  process.exitCode = 1;
});

api.on("exit", (code, signal) => {
  process.exitCode = signal ? 0 : (code ?? 1);
});

const shutdown = (signal) => {
  if (!api.killed) api.kill(signal);
};

process.on("SIGTERM", () => shutdown("SIGTERM"));
process.on("SIGINT", () => shutdown("SIGINT"));
