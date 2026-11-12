"""Copy current local sources into a reviewable handoff; never touch hardware."""
from pathlib import Path
import csv
import hashlib
import json
import shutil
import xml.etree.ElementTree as ET
from datetime import datetime, timezone, timedelta

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / '云端任务与硬件执行_后端排查交付包_20261023'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    OUT.mkdir(exist_ok=False)
    manifest = []
    def copy(src, rel=None):
        rel = rel or src.relative_to(ROOT)
        dst = OUT / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        digest = sha(src)
        assert sha(dst) == digest
        manifest.append({'source': str(src), 'package_path': str(rel).replace('\\', '/'), 'bytes': src.stat().st_size,
                         'source_mtime': datetime.fromtimestamp(src.stat().st_mtime, timezone(timedelta(hours=8))).isoformat(),
                         'source_sha256': digest, 'package_sha256': sha(dst), 'transformation': 'none'})
    for src in sorted((ROOT / 'raspberry_pi_gateway').rglob('*')):
        rel = src.relative_to(ROOT / 'raspberry_pi_gateway')
        if not src.is_file() or any(p in {'data','captures','__pycache__','.pytest_cache','.venv'} for p in rel.parts):
            continue
        copy(src)
    firmware_exts = {'.c','.h','.s','.uvprojx','.uvoptx','.ini','.sct','.dbgconf','.scvd','.md','.txt','.lib'}
    for board in ('主板1', '主板2'):
        base = ROOT / '代码' / '4.11' / board
        for src in sorted(base.rglob('*')):
            if src.is_file() and src.suffix.lower() in firmware_exts:
                copy(src)
        # Existing binaries/maps are historical comparison material, not new build outputs.
        for name in ('Radar_2.hex','Radar_2.map','Radar_2.4G'):
            src = base / 'OBJ' / name
            if src.exists():
                copy(src, Path('证据/既有构建产物_未重编译未确认烧录') / board / name)
    for rel in ('02_界面原型/cloud_display.html', 'AGENTS.md', '00_项目导航/改动记录.md',
                '00_项目导航/README_项目总览.md', '00_项目导航/调试快速定位.md',
                '00_项目导航/家居文件位置索引.md'):
        copy(ROOT / rel)
    for src in sorted((ROOT / '02_界面原型/k230_cloud_control').iterdir()):
        if src.is_file():
            copy(src, Path('历史参考_不作为当前部署依据/k230_cloud_control') / src.name)
    evidence = OUT / '证据'
    evidence.mkdir(exist_ok=True)
    (evidence / '复制来源清单.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding='utf-8')
    with (evidence / '复制来源清单.csv').open('w', encoding='utf-8-sig', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(manifest[0]))
        writer.writeheader(); writer.writerows(manifest)
    validation = []
    for proj in OUT.glob('代码/4.11/*/USER/*.uvprojx'):
        xml = ET.parse(proj)
        files = [p.text for p in xml.findall('.//FilePath') if p.text]
        missing = [p for p in files if not (proj.parent / p.replace('\\','/')).exists()]
        validation.append({'project': str(proj.relative_to(OUT)), 'file_references': len(files), 'missing_files': missing})
    (evidence / 'Keil工程源文件引用检查.json').write_text(json.dumps(validation, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'output': str(OUT), 'copied_files': len(manifest), 'bytes': sum(x['bytes'] for x in manifest), 'keil': validation}, ensure_ascii=False))

if __name__ == '__main__':
    main()
