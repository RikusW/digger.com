# Tools

All tools use only the Python 3 standard library and read `digger.com`.

| Tool | Purpose |
|---|---|
| `extract_data.py` | regenerate `data/` (raw segments, tables, font, sprites, title PNGs) |
| `compare_tables.py` | byte-compare the reconstructed C tables with `digger.com` |
| `label_functions.py` | label every CALL target in `digger.lst` and write `../FUNCTIONS.md` |

`data/` is **generated and ignored by git** (see `../.gitignore`).
Only the tools and their documentation are committed.  To reproduce:

```
python3 tools/extract_data.py
python3 tools/compare_tables.py
python3 tools/label_functions.py
```

`extract_data.py` is deterministic: running it twice produces byte-identical
output (348 files) from the same `digger.com`.
