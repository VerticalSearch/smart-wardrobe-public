"""Collect only local gateway status/logs and local file hashes. No commands or cloud proxy reads."""
from pathlib import Path
from datetime import datetime, timezone
from urllib.request import Request, build_opener, HTTPRedirectHandler
from urllib.parse import urlsplit
import argparse, hashlib, json, re

class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl): return None

def scrub(value):
    if isinstance(value, dict):
        return {k: ('<REDACTED>' if re.search(r'password|passwd|secret|token|authorization|cookie|image_base64',k,re.I) else scrub(v)) for k,v in value.items()}
    if isinstance(value, list): return [scrub(x) for x in value]
    if isinstance(value, str):
        value = re.sub(r'(?i)(Bearer\s+)\S+',r'\1<REDACTED>',value)
        value = re.sub(r'''(?i)(["']?(?:password|passwd|secret|token|authorization|cookie)["']?\s*[:=]\s*)(["'])(.*?)(\2)''',r'\1\2<REDACTED>\4',value)
    return value

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--base',default='http://127.0.0.1:8000')
    p.add_argument('--gateway-root',type=Path,help='Actual service working directory on Pi; optional file hashes/snapshot')
    p.add_argument('--out',type=Path,required=True)
    a=p.parse_args()
    u=urlsplit(a.base)
    if u.scheme not in {'http','https'} or u.path not in {'','/'} or u.username or u.password or u.query or u.fragment:
        p.error('--base must be a gateway origin without credentials/path/query')
    a.out.mkdir(parents=True,exist_ok=False)
    report={'collected_at_utc':datetime.now(timezone.utc).isoformat(),'base':a.base,'read_only':True,'requests':[]}
    opener=build_opener(NoRedirect)
    for name,endpoint in [('status','/api/status'),('inventory','/api/inventory'),('serial_logs','/api/serial/logs?limit=300')]:
        entry={'method':'GET','path':endpoint}
        try:
            with opener.open(Request(a.base.rstrip('/')+endpoint,method='GET'),timeout=8) as response:
                body=json.loads(response.read(8*1024*1024).decode('utf-8'))
                entry['http_status']=response.status
            (a.out/(name+'.json')).write_text(json.dumps(scrub(body),ensure_ascii=False,indent=2),encoding='utf-8')
        except Exception as exc: entry['error']=scrub(str(exc))
        report['requests'].append(entry)
    if a.gateway_root:
        root=a.gateway_root.resolve()
        report['gateway_root']=str(root)
        report['file_hashes']={}
        for rel in ['src/wardrobe_gateway/app.py','src/wardrobe_gateway/protocol.py','src/wardrobe_gateway/inventory.py','src/wardrobe_gateway/mqtt_bridge.py','static/cloud_display.html','run.py']:
            file=root/rel
            report['file_hashes'][rel]=hashlib.sha256(file.read_bytes()).hexdigest() if file.is_file() else 'MISSING'
        inv=root/'data/store_positions.json'
        if inv.is_file():
            (a.out/'inventory_file_snapshot.json').write_text(json.dumps(scrub(json.loads(inv.read_text(encoding='utf-8'))),ensure_ascii=False,indent=2),encoding='utf-8')
    (a.out/'collection.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(str(a.out.resolve()))

if __name__=='__main__': main()
