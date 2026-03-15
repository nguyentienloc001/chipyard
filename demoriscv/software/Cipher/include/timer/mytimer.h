#include "mmio.h"

#define MyTimer_control       0x00007000     // Bit high to low: reset-we-cs
#define MyTimer_count         0x00007004

// MSB to LSB => {reset, clear, pause, start)

#define CTRL_MyTimer_IDLE     0x00
#define CTRL_MyTimer_RESET    0x08
#define CTRL_MyTimer_CLEAR    0x04
#define CTRL_MyTimer_PAUSE    0x02
#define CTRL_MyTimer_START    0x01


void mytimer_soft_reset() {
  reg_write32(MyTimer_control, CTRL_MyTimer_RESET);
  reg_write32(MyTimer_control, CTRL_MyTimer_IDLE);
}


void mytimer_start() {
  reg_write32(MyTimer_control, CTRL_MyTimer_START);
  reg_write32(MyTimer_control, CTRL_MyTimer_IDLE);
}


void mytimer_clear() {
  reg_write32(MyTimer_control, CTRL_MyTimer_CLEAR);
  reg_write32(MyTimer_control, CTRL_MyTimer_IDLE);
}


uint32_t mytimer_pause_and_return() {
  reg_write32(MyTimer_control, CTRL_MyTimer_PAUSE);
  reg_write32(MyTimer_control, CTRL_MyTimer_IDLE);
  return reg_read32(MyTimer_count);
}

