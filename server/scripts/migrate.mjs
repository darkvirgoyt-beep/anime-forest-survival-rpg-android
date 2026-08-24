import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import pg from "pg";
import { loadRuntimeConfig } from "../src/security.mjs";

const { Pool } = pg;
const here = path.dirname(fileURLToPath(import.meta.url));
const sqlDirectory = path.resolve(here, "../sql");
const config = loadRuntimeConfig();
const pool = new Pool({
  connectionString: config.databaseUrl,
  max: 1,
  ssl: config.databaseSsl ? { rejectUnauthorized: config.databaseSslVerify } : undefined
});

try {
  const client = await pool.connect();
  try {
    await client.query(`
      CREATE TABLE IF NOT EXISTS schema_migrations (
        filename TEXT PRIMARY KEY,
        applied_at TIMESTAMPTZ NOT NULL DEFAULT now()
      )
    `);

    const files = (await fs.readdir(sqlDirectory))
      .filter(filename => /^\d{3}_.+\.sql$/.test(filename))
      .sort();

    for (const filename of files) {
      const applied = await client.query("SELECT 1 FROM schema_migrations WHERE filename = $1", [filename]);
      if (applied.rowCount === 1) continue;

      const sql = await fs.readFile(path.join(sqlDirectory, filename), "utf8");
      await client.query("BEGIN");
      try {
        await client.query(sql);
        await client.query("INSERT INTO schema_migrations (filename) VALUES ($1)", [filename]);
        await client.query("COMMIT");
        console.log(`Applied migration ${filename}`);
      } catch (error) {
        await client.query("ROLLBACK");
        throw error;
      }
    }

    console.log(`Aethelgrad database migrations ready (${files.length} files).`);
  } finally {
    client.release();
  }
} finally {
  await pool.end();
}
