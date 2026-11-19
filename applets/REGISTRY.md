# SmartApplet ID block registry

Betawise maintains the canonical list of SmartApplet ID block assignments. See
[HISTORICAL.md](HISTORICAL.md) for historical IDs.

| Inclusive range | Owner |
| --- | --- |
| `0x7100`–`0x717F` | [Community NEO fonts](HISTORICAL.md#community-neo-fonts) |
| `0x9000`–`0x900F` | [CoWriter resources](HISTORICAL.md#cowriter-resources) |
| `0xA000`–`0xA11F` | [AlphaSmart legacy applets](HISTORICAL.md#alphasmart-legacy-applets) |
| `0xA120`–`0xA13F` | [Jakub Kolcar / ioma8](https://github.com/ioma8/neo-re) |
| `0xA1A0`–`0xA1FF` | [Alpaxo / Betawise](APPLETS.md) |
| `0xAF00`–`0xAFFF` | [Legacy NEO fonts](HISTORICAL.md#legacy-neo-fonts) |

## Request a block

Open a pull request to this file before using a new block. Add a row with the
requested range, maintainer, and project link. Request one or more 16-ID blocks.
Start on a 16-ID boundary and check the table and historical index for overlap.

Each maintainer manages the applet IDs within their own block. We encourage
maintainers to publish and link an index using the [APPLETS.md format](APPLETS.md).
