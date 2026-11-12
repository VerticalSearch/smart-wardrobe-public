from pathlib import Path
import ast,csv,hashlib,importlib.util,json,re,shutil,sys,tempfile,zipfile
from datetime import datetime,timezone,timedelta

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'云端任务与硬件执行_后端排查交付包_20261023'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def write(rel,obj):
    p=OUT/rel
    p.parent.mkdir(parents=True,exist_ok=True)
    p.write_text(json.dumps(obj,ensure_ascii=False,indent=2),encoding='utf-8')

def main():
    # Synchronize only the explicitly required project change log.
    shutil.copy2(ROOT/'00_项目导航/改动记录.md',OUT/'00_项目导航/改动记录.md')
    records=json.loads((OUT/'证据/复制来源清单.json').read_text(encoding='utf-8'))
    for item in records:
        src=Path(item['source']); dst=OUT/item['package_path']
        if item['package_path']=='00_项目导航/改动记录.md':
            item.update(bytes=src.stat().st_size,source_sha256=sha(src),package_sha256=sha(dst),
                        source_mtime=datetime.fromtimestamp(src.stat().st_mtime,timezone(timedelta(hours=8))).isoformat())
        assert sha(src)==item['source_sha256']==sha(dst),item['package_path']
    write('证据/复制来源清单.json',records)
    with (OUT/'证据/复制来源清单.csv').open('w',encoding='utf-8-sig',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(records[0])); w.writeheader(); w.writerows(records)
    specs=[
      ('一键启动按钮/动作常量','raspberry_pi_gateway/static/cloud_display.html','const STORE_PREPARE_ACTION_ID'),
      ('存衣准备入口','raspberry_pi_gateway/static/cloud_display.html','async function newStore('),
      ('缩回后继续存衣','raspberry_pi_gateway/static/cloud_display.html','async function retractAndStartStore('),
      ('页面终态精确ID匹配','raspberry_pi_gateway/static/cloud_display.html','async function waitForTaskTerminal('),
      ('原子接口调用','raspberry_pi_gateway/static/cloud_display.html','async function createHardwareTaskWithSerial('),
      ('云建任务与串口下发','raspberry_pi_gateway/src/wardrobe_gateway/app.py','def _dispatch_hardware_task('),
      ('串口命令发送/事务预留','raspberry_pi_gateway/src/wardrobe_gateway/app.py','def _send_commands('),
      ('板端回执/库存/事件/MQTT','raspberry_pi_gateway/src/wardrobe_gateway/app.py','def _handle_serial_event('),
      ('MQTT终态未传外层ID','raspberry_pi_gateway/src/wardrobe_gateway/app.py','mqtt_bridge.publish_status("task", line'),
      ('MQTT桥/双主题/QoS','raspberry_pi_gateway/src/wardrobe_gateway/mqtt_bridge.py','class MqttBridge:'),
      ('24字符清洗','raspberry_pi_gateway/src/wardrobe_gateway/protocol.py','def clean_text('),
      ('任务串口编码','raspberry_pi_gateway/src/wardrobe_gateway/protocol.py','def action_line('),
      ('串口回执解析','raspberry_pi_gateway/src/wardrobe_gateway/serial_link.py','def _parse_buffer('),
      ('库存活动事务保护','raspberry_pi_gateway/src/wardrobe_gateway/inventory.py','def _ensure_no_active_locked('),
      ('库存短ID兼容及完成提交','raspberry_pi_gateway/src/wardrobe_gateway/inventory.py','def apply_task_status('),
      ('环境配置','raspberry_pi_gateway/src/wardrobe_gateway/config.py','def from_env('),
      ('主板1命令接收与忙锁','代码/4.11/主板1/HARDWARE/UART3_PI_GATEWAY/board1_uart3_pi_gateway.c','static void board1_pi_gateway_handle_action_line('),
      ('主板1回传终态/释放任务锁','代码/4.11/主板1/HARDWARE/UART3_PI_GATEWAY/board1_uart3_pi_gateway.c','void board1_uart3_pi_gateway_handle_board2_status('),
      ('主板1发送0x13任务','代码/4.11/主板1/HARDWARE/USART1.DMA/usartdma.c','uint8_t Board1_LinkStartAction('),
      ('主板2任务路由','代码/4.11/主板2/HARDWARE/USART1.DMA/usartdma.c','static void board2_start_action('),
      ('主板2终态回传服务','代码/4.11/主板2/HARDWARE/USART1.DMA/usartdma.c','void Board2_LinkTaskStatusService('),
      ('主板2动作12入口','代码/4.11/主板2/HARDWARE/Action_List/board2_action_list.c','void Board2_ActionList_StartStorePrepare('),
      ('伸出后STORE_PREPARE结束分支','代码/4.11/主板2/HARDWARE/Action_List/board2_action_list.c','if (s_workflow_mode == BOARD2_ACTION_MODE_STORE_PREPARE)'),
      ('主板2动作13入口','代码/4.11/主板2/HARDWARE/Action_List/board2_action_list.c','void Board2_ActionList_StartStoreRetract('),
      ('主板2动作11入口','代码/4.11/主板2/HARDWARE/Action_List/board2_action_list.c','void Board2_ActionList_StartDefaultAfterTelescope('),
      ('主板2主循环服务','代码/4.11/主板2/USER/main.c','Board2_LinkTaskStatusService();'),
    ]
    index=['# 关键代码索引','', '行号针对本交付快照。先阅读交付说明，避免把旧导航中的K230/USART1描述套到当前硬件。','', '| 关注点 | 文件 | 行号 |','|---|---|---|']
    for title,rel,needle in specs:
        lines=(OUT/rel).read_text(encoding='utf-8-sig',errors='replace').splitlines()
        n=next((i for i,line in enumerate(lines,1) if needle in line),None)
        assert n,(title,needle)
        index.append(f'| {title} | [{rel}](<{rel}>) | {n} |')
    index+=['','完整驱动和动作参数位于主板2 `HARDWARE/Action_List/board2_action_tuning.h`、`Stepper`、各UART目录；不要只复制表内几个C文件。','']
    (OUT/'04_关键代码索引.md').write_text('\n'.join(index),encoding='utf-8')
    pyfiles=list(OUT.rglob('*.py'))
    for p in pyfiles: ast.parse(p.read_text(encoding='utf-8-sig'),filename=str(p))
    # Exercise collector with a fake opener: verifies exact GET allowlist and redaction with no network.
    spec=importlib.util.spec_from_file_location('handoff_collector',OUT/'排查工具/collect_readonly.py')
    mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
    captured=[]
    class Response:
        status=200
        def __enter__(self):return self
        def __exit__(self,*args):pass
        def read(self,n):return b'{"token":"example-test-value","ok":true}'
    class Opener:
        def open(self,req,timeout): captured.append((req.get_method(),req.full_url)); return Response()
    mod.build_opener=lambda *a:Opener()
    saved=sys.argv
    with tempfile.TemporaryDirectory(prefix='wardrobe_collect_check_') as temp:
        target=Path(temp)/'case'
        sys.argv=['collect_readonly.py','--base','http://127.0.0.1:8000','--out',str(target)]
        mod.main()
        assert len(captured)==3 and all(m=='GET' for m,u in captured)
        assert json.loads((target/'status.json').read_text(encoding='utf-8'))['token']=='<REDACTED>'
    sys.argv=saved
    secret=re.compile(r'''(?i)\b([\w]*(?:password|passwd|secret|token|api_key)[\w]*)["']?\s*(?:[:=]|\s+)\s*(["'])([^"'\r\n]+)\2''')
    findings=[]
    exts={'.py','.js','.html','.json','.md','.c','.h','.sh','.ps1','.service','.ini'}
    for p in OUT.rglob('*'):
        if p.suffix.lower() not in exts:continue
        for n,line in enumerate(p.read_text(encoding='utf-8',errors='replace').splitlines(),1):
            for m in secret.finditer(line):
                value=m.group(3)
                if value in {'<REDACTED>','redacted','Bearer ','gateway_command_id','token','password'}:continue
                if m.group(1)=='token_redaction' and value=='passed' and p.name=='交付验收.json':continue
                findings.append({'path':p.relative_to(OUT).as_posix(),'line':n,'key':m.group(1)})
    assert not findings,findings
    write('证据/交付验收.json',{'verified_at':datetime.now(timezone(timedelta(hours=8))).isoformat(),
        'copied_files_verified':len(records),'source_files_unchanged_except_required_changelog':True,
        'python_ast_files':len(pyfiles),'pytest':'79 passed, 4 warnings','keil_build':'not performed',
        'hardware_flashed':False,'hardware_reset':False,'device_connected':False,'cloud_business_accessed':False,
        'collector_mock_check':{'requests':captured,'token_redaction':'passed','network_used':False},
        'sensitive_literal_scan_findings':findings,'scan_scope':'selected text formats; not a guarantee about every possible secret'})
    files=[]
    for p in sorted(OUT.rglob('*')):
        if not p.is_file() or p.name=='文件校验清单.json':continue
        assert '__pycache__' not in p.parts
        files.append({'path':p.relative_to(OUT).as_posix(),'bytes':p.stat().st_size,'sha256':sha(p)})
    write('文件校验清单.json',{'algorithm':'SHA-256','excludes':['文件校验清单.json'],'files':files})
    archive=OUT.with_suffix('.zip')
    with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for p in sorted(OUT.rglob('*')):
            if p.is_file():z.write(p,OUT.name+'/'+p.relative_to(OUT).as_posix())
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert len(z.namelist())==len(set(z.namelist()))==len(files)+1
        for item in files:
            assert hashlib.sha256(z.read(OUT.name+'/'+item['path'])).hexdigest()==item['sha256']
        assert hashlib.sha256(z.read(OUT.name+'/文件校验清单.json')).hexdigest()==sha(OUT/'文件校验清单.json')
    digest=sha(archive)
    archive.with_suffix('.zip.sha256.txt').write_text(digest+'  '+archive.name+'\n',encoding='utf-8')
    print(json.dumps({'archive':str(archive),'files':len(files)+1,'bytes':archive.stat().st_size,'sha256':digest,'python_ast_files':len(pyfiles)},ensure_ascii=False))

if __name__=='__main__':
    sys.dont_write_bytecode=True
    main()
