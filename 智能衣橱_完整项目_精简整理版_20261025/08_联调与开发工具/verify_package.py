"""Read-only verification, Python standard library only."""
from pathlib import Path
import hashlib, json, sys
root = Path(__file__).resolve().parents[1]
manifest = root / '09_整理校验/文件校验清单.json'
rows = json.loads(manifest.read_text(encoding='utf-8'))
errors = []
for row in rows:
    p = root / row['path']
    if not p.is_file():
        errors.append({'path': row['path'], 'error': 'missing'})
        continue
    with p.open('rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest()
    if p.stat().st_size != row['bytes'] or digest != row['sha256']:
        errors.append({'path': row['path'], 'error': 'changed'})
expected = {x['path'] for x in rows} | {'09_整理校验/文件校验清单.json'}
actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
extra = sorted(actual - expected)
print(json.dumps({'checked': len(rows), 'errors': errors, 'extra_files': extra, 'ok': not errors and not extra}, ensure_ascii=False))
sys.exit(1 if errors or extra else 0)
