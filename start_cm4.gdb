set pagination off
monitor reset halt
set {int}0xE000ED08 = 0x08720000
set $sp = 0x20002ec0
set $pc = 0x20202b79
continue