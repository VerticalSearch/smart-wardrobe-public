from __future__ import annotations

from pathlib import Path
import sys

import uvicorn

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "src"))

from wardrobe_gateway.config import GatewayConfig


def main() -> None:
    config = GatewayConfig.from_env()
    uvicorn.run("wardrobe_gateway.app:app", host=config.host, port=config.port, reload=False)


if __name__ == "__main__":
    main()
