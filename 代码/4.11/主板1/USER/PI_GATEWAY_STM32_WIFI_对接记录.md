# Raspberry Pi Gateway / STM32 / Serial Screen WiFi Notes

Updated: 2026-05-30

Current board1 no longer uses the previous upper-computer communication module. The active USART3
upper-computer module is:

- `../HARDWARE/UART3_PI_GATEWAY/board1_uart3_pi_gateway.c`
- `../HARDWARE/UART3_PI_GATEWAY/board1_uart3_pi_gateway.h`

The Keil project compiles `board1_uart3_pi_gateway.c`, and the previous
USART3 gateway source directory has been removed from the active board1 tree.

## Connection

- Raspberry Pi CH340 `TXD` -> board1 `PD9 / USART3_RX`
- Raspberry Pi CH340 `RXD` -> board1 `PD8 / USART3_TX`
- Raspberry Pi CH340 `GND` -> board1 `GND`
- Do not connect `VCC` by default.

## Protocol Boundary

The wire protocol is intentionally kept compatible with the existing web and
mainboard workflow:

- Web/task command to board1: `@ACMD`, `@MCMD`, `@SCMD`, `@OCMD`
- Board1 capture request to gateway: binary frame `FF 01 00 00 FE`
- Gateway result/image-info/image-ready frames keep the same numeric format.
- Optional serial-screen WiFi page can still send `@WSCAN`, `@WSEL`, and
  `@WCONN` through STM32, then STM32 forwards scan/connect requests to the
  Raspberry Pi gateway.

Board2 does not talk to the Raspberry Pi directly. Board2 remains behind the
existing board1-to-board2 command link.

## Build Check

Verified with Keil command line after replacement:

```text
"..\OBJ\Radar_2.4G" - 0 Error(s), 0 Warning(s).
```
