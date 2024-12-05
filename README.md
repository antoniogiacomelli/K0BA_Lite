# K0BA Lite 

**K0BA** (_Kernel 0 For emBedded Applications_) is a Real-Time Kernel aiming constrained MCU-based solutions. 
This is a lightweight version with no memory isolation. Currently it supports ARMv7M architecture.
A version with memory isolation will be released soon.

> [!NOTE]
> This is a work in progress. 

*Logical Architecture:*

<img width="450" alt="kernel" src="https://github.com/antoniogiacomelli/K0BA_Lite/blob/master/layeredkernel.png">

## Highlights
  - **_O(1)_** time-complexity scheduler (RMS, up to 32 priorities)  
  - Semaphores and mutexes with priority inheritance
  - "Pump-Drop" asynchronous queue 
  - Flexible mailbox and pipe
  - Periodic/One-Shot delta-timers

**Major dependencies:**
- ARM GCC 
- CMSIS HAL

