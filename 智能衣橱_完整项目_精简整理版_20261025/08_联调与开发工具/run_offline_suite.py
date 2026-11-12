"""Run unchanged source tests with temporary persistence and blocked network/device access."""
from pathlib import Path
import os, sys, socket, tempfile, dataclasses, ipaddress
ROOT=Path(__file__).resolve().parents[1]/'02_树莓派网关与网页'
sys.path.insert(0,str(ROOT/'raspberry_pi_gateway/src'))
sys.dont_write_bytecode=True
os.environ['PYTEST_DISABLE_PLUGIN_AUTOLOAD']='1'

def denied(*args,**kwargs): raise AssertionError('External network or serial use forbidden in offline suite')

def main():
    import pytest, serial, requests
    from wardrobe_gateway.config import GatewayConfig
    with tempfile.TemporaryDirectory(prefix='wardrobe_suite_') as folder:
        base=Path(folder)
        cfg=dataclasses.replace(GatewayConfig.from_env(),project_root=base,capture_dir=base/'captures',
            serial_dry_run=True,mqtt_enable=False,weather_enable=False,cloud_api_base='http://127.0.0.1:9',
            device_token='',mqtt_password='')
        GatewayConfig.from_env=classmethod(lambda cls:cfg)
        # Windows asyncio uses a loopback socketpair internally; permit only that transport.
        original_connect=socket.socket.connect
        def local_connect(sock,address):
            try: allowed=ipaddress.ip_address(address[0]).is_loopback
            except (ValueError,TypeError,IndexError): allowed=False
            if not allowed: return denied()
            return original_connect(sock,address)
        socket.socket.connect=local_connect
        socket.create_connection=denied
        requests.sessions.Session.request=denied
        serial.Serial=denied
        return pytest.main([str(ROOT/'raspberry_pi_gateway/tests'),'-q','-p','no:cacheprovider','--basetemp',str(base/'pytest')])
if __name__=='__main__': sys.exit(main())
