# Serial Screen WiFi Protocol

The serial screen connects to STM32 USART6. The Raspberry Pi gateway connects to
STM32 USART3 through the `UART3_PI_GATEWAY` module. The screen only talks to
STM32; STM32 forwards WiFi commands to the Raspberry Pi gateway when this legacy
screen WiFi page is used.

## Screen To STM32

- Scan button: `@WSCAN\n`
- Select WiFi row 1: `@WSEL|0\n`
- Select WiFi row 2: `@WSEL|1\n`
- Select WiFi row 8: `@WSEL|7\n`
- Connect button: `@WCONN|password\n`

## STM32 Screen Components

- WiFi list text: `wifi0` to `wifi7`
- Selected network: `wifi_ssid`
- Status text: `wifi_status`

## STM32 To Raspberry Pi Gateway

- Start scan: `@WSCAN\n`
- Connect: `@WCONN|SSID|PASSWORD\n`

## Raspberry Pi Gateway To STM32

- Scan started: `@WSCAN_BEGIN\n`
- One network: `@WAP|index|rssi|auth|ssid\n`
- Scan finished: `@WSCAN_END|count\n`
- Connecting: `@WIFI_CONNECTING|ssid\n`
- Connected: `@WIFI_OK|ip\n`
- Failed: `@WIFI_FAIL|reason\n`

SSID and password must not contain `|`, carriage return, or newline.
