/**
 * @module version
 * Reads package version and name from package.json.
 */

import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const pkgPath = fileURLToPath(new URL('../package.json', import.meta.url));
const pkg = JSON.parse(readFileSync(pkgPath, 'utf-8'));

/** @type {string} Semver version string */
export const VERSION = pkg.version;

/** @type {string} Package name (scoped) */
export const NAME = pkg.name;
