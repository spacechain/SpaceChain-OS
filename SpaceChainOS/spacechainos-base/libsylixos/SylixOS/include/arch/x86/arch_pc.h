/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: arch_pc.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 04 月 21 日
**
** 描        述: x86 PC 通用定义.
*********************************************************************************************************/

#ifndef __X86_ARCH_PC_H
#define __X86_ARCH_PC_H

/*********************************************************************************************************
  Programmable interrupt controller (PIC)
*********************************************************************************************************/
#define PIC1_BASE_ADR               0x20
#define PIC2_BASE_ADR               0xa0
#define PIC_REG_ADDR_INTERVAL       1                               /*  address diff of adjacent regs   */
#define PIC_MASTER_STRAY_INT_LVL    0x07                            /*  master PIC stray IRQ            */
#define PIC_SLAVE_STRAY_INT_LVL     0x0f                            /*  slave PIC stray IRQ             */
#define PIC_MAX_INT_LVL             0x0f                            /*  max interrupt level in PIC      */
#define N_PIC_IRQS                  16                              /*  number of PIC IRQs              */
/*********************************************************************************************************
  Serial ports (COM1,COM2,COM3,COM4)
*********************************************************************************************************/
#define COM1_BASE_ADR               0x3f8
#define COM2_BASE_ADR               0x2f8
#define COM3_BASE_ADR               0x3e8
#define COM4_BASE_ADR               0x2e8

#define COM1_INT_LVL                0x04
#define COM2_INT_LVL                0x03
#define COM3_INT_LVL                0x05
#define COM4_INT_LVL                0x09

#define UART_REG_ADDR_INTERVAL      1                               /*  address diff of adjacent regs   */
#define N_UART_CHANNELS             2
/*********************************************************************************************************
  Timer (PIT)
*********************************************************************************************************/
#define PIT_BASE_ADR                0x40
#define PIT0_INT_LVL                0x00
#define PIT_REG_ADDR_INTERVAL       1                               /*  address diff of adjacent regs   */
#define PC_PIT_CLOCK                1193180

#define PIT_CH2                     0x42
#define PIT_MODE                    0x43
/*********************************************************************************************************
  Real time clock (RTC)
*********************************************************************************************************/
#define RTC_INDEX                   0x70
#define RTC_DATA                    0x71
#define RTC_INT_LVL                 0x08
/*********************************************************************************************************
  Floppy disk (FD) The buffer must not cross a 64KB boundary, and be in lower memory.
*********************************************************************************************************/
#define FD_INT_LVL                  0x06
#define FD_DMA_BUF_ADDR             0x2400                          /*  floppy disk DMA buffer addr     */
#define FD_DMA_BUF_SIZE             0x2000                          /*  floppy disk DMA buffer size     */
/*********************************************************************************************************
  Pcmcia (PCMCIA)
*********************************************************************************************************/
#define PCMCIA_SOCKS                0x0                             /*  number of sockets. 0=auto detect*/
#define PCMCIA_MEMBASE              0x0                             /*  mapping base address            */
#define PCIC_BASE_ADR               0x3e0                           /*  Intel 82365SL                   */
#define PCIC_INT_LVL                0x0a
#define TCIC_BASE_ADR               0x240                           /*  Databook DB86082A               */
#define TCIC_INT_LVL                0x0a
#define CIS_MEM_START               0xd0000                         /*  mapping addr for CIS tuple      */
#define CIS_MEM_STOP                0xd3fff
#define CIS_REG_START               0xd4000                         /*  mapping addr for config reg     */
#define CIS_REG_STOP                0xd4fff
#define SRAM0_MEM_START             0xd8000                         /*  mem for SRAM0                   */
#define SRAM0_MEM_STOP              0xd8fff
#define SRAM0_MEM_LENGTH            0x100000
#define SRAM1_MEM_START             0xd9000                         /*  mem for SRAM1                   */
#define SRAM1_MEM_STOP              0xd9fff
#define SRAM1_MEM_LENGTH            0x100000
#define SRAM2_MEM_START             0xda000                         /*  mem for SRAM2                   */
#define SRAM2_MEM_STOP              0xdafff
#define SRAM2_MEM_LENGTH            0x100000
#define SRAM3_MEM_START             0xdb000                         /*  mem for SRAM3                   */
#define SRAM3_MEM_STOP              0xdbfff
#define SRAM3_MEM_LENGTH            0x100000
#define ELT0_IO_START               0x260                           /*  io for ELT0                     */
#define ELT0_IO_STOP                0x26f
#define ELT0_INT_LVL                0x05
#define ELT0_NRF                    0x00
#define ELT0_CONFIG                 0                               /*  0=EEPROM 1=AUI  2=BNC  3=RJ45   */
#define ELT1_IO_START               0x280                           /*  io for ELT1                     */
#define ELT1_IO_STOP                0x28f
#define ELT1_INT_LVL                0x09
#define ELT1_NRF                    0x00
#define ELT1_CONFIG                 0                               /*  0=EEPROM 1=AUI  2=BNC  3=RJ45   */
/*********************************************************************************************************
  Parallel port (LPT)
*********************************************************************************************************/
#define LPT0_BASE_ADRS              0x3bc
#define LPT1_BASE_ADRS              0x378
#define LPT2_BASE_ADRS              0x278
#define LPT0_INT_LVL                0x07
#define LPT1_INT_LVL                0x05
#define LPT2_INT_LVL                0x09
#define LPT_INT_LVL                 0x07
#define LPT_CHANNELS                1
/*********************************************************************************************************
  ISA IO address for bspDelay720Ns ()
*********************************************************************************************************/
#define UNUSED_ISA_IO_ADDRESS       0x84
/*********************************************************************************************************
  Mouse (MSE)
*********************************************************************************************************/
#define MSE_INT_LVL                 (0x0c)                          /*  IRQ 12 assuming PS/2 mouse      */
/*********************************************************************************************************
  Key board (KBD)
*********************************************************************************************************/
#define PC_XT_83_KBD                0                               /*  83 KEY PC/PCXT/PORTABLE         */
#define PC_PS2_101_KBD              1                               /*  101 KEY PS/2                    */
#define KBD_INT_LVL                 0x01

#define COMMAND_8042                0x64
#define DATA_8042                   0x60
#define STATUS_8042                 COMMAND_8042
#define COMMAND_8048                0x61                            /*  out Port PC 61H in the 8255 PPI */
#define DATA_8048                   0x60                            /*  input port                      */
#define STATUS_8048                 COMMAND_8048

#define JAPANES_KBD                 0
#define ENGLISH_KBD                 1

#define KBD_KDIB                    0x01                            /*  Kbd data in buffer              */
#define KBD_UDIB                    0x02                            /*  User data in buffer             */
#define KBD_MDIB                    0x20                            /*  Mouse data in buffer            */

#define KBD_RESET                   0xfe                            /*  Reset CPU command               */
/*********************************************************************************************************
  Beep generator
*********************************************************************************************************/
#define DIAG_CTRL                   0x61
#define BEEP_PITCH_L                1280                            /*  932 Hz                          */
#define BEEP_PITCH_S                1208                            /*  987 Hz                          */
/*********************************************************************************************************
  Monitor definitions
*********************************************************************************************************/
#define VGA_MEM_BASE                0xb8000
#define VGA_SEL_REG                 0x3d4
#define VGA_VAL_REG                 0x3d5
#define MONO_MEM_BASE               0xb0000
#define MONO_SEL_REG                0x3b4
#define MONO_VAL_REG                0x3b5

#define VESA_BIOS_DATA_ADDRESS      (0xbfb00)                       /*  BIOS data storage               */
#define VESA_BIOS_DATA_PREFIX       (VESA_BIOS_DATA_ADDRESS - 8)
#define VESA_BIOS_DATA_SIZE         (0x500)                         /*  Vesa BIOS data size             */
#define VESA_BIOS_KEY_1             (0x534F4942)                    /*  "BIOS"                          */
#define VESA_BIOS_KEY_2             (0x41544144)                    /*  "DATA"                          */
/*********************************************************************************************************
  PCI IOAPIC defines
*********************************************************************************************************/
#define IOAPIC_PIRQA_INT_LVL        16                              /*  IOAPIC PIRQA IRQ                */
#define IOAPIC_PIRQB_INT_LVL        17                              /*  IOAPIC PIRQB IRQ                */
#define IOAPIC_PIRQC_INT_LVL        18                              /*  IOAPIC PIRQC IRQ                */
#define IOAPIC_PIRQD_INT_LVL        19                              /*  IOAPIC PIRQD IRQ                */
#define IOAPIC_PIRQE_INT_LVL        20                              /*  IOAPIC PIRQE IRQ                */
#define IOAPIC_PIRQF_INT_LVL        21                              /*  IOAPIC PIRQF IRQ                */
#define IOAPIC_PIRQG_INT_LVL        22                              /*  IOAPIC PIRQG IRQ                */
#define IOAPIC_PIRQH_INT_LVL        23                              /*  IOAPIC PIRQH IRQ                */

#define N_IOAPIC_PIRQS              8                               /*  number of IOAPIC PIRQs          */

#define IOAPIC_PXIRQ0_INT_LVL       24
#define IOAPIC_PXIRQ1_INT_LVL       25
#define IOAPIC_PXIRQ2_INT_LVL       26
#define IOAPIC_PXIRQ3_INT_LVL       27

#define N_IOAPIC_PXIRQS             4                               /*  number of IOAPIC PXIRQs         */
/*********************************************************************************************************
  Multi processor (SMP) definitions
*********************************************************************************************************/
#define MP_ANCHOR_ADRS              0x1400                          /*  Base addr for the counters      */
#define WARM_RESET_VECTOR           0x0467                          /*  BIOS: addr to hold the vector   */
#define BIOS_SHUTDOWN_STATUS        0x0f                            /*  BIOS: shutdown status           */
/*********************************************************************************************************
  Reset control register definitions
*********************************************************************************************************/
#define RST_CNT_REG                 0x0cf9                          /*  Reset Control Register          */
#define RST_CNT_SYS_RST             0x02                            /*  System Reset bit                */
#define RST_CNT_CPU_RST             0x04                            /*  Reset CPU bit                   */
#define RST_CNT_FULL_RST            0x08                            /*  Full Reset bit                  */
/*********************************************************************************************************
  BIOS ROM space
*********************************************************************************************************/
#define BIOS_ROM_START              0xef000
#define BIOS_ROM_SIZE               0x11000
#define BIOS_ROM_END                (BIOS_ROM_START + (BIOS_ROM_SIZE - 1))
/*********************************************************************************************************
  Extended BIOS Data Area
*********************************************************************************************************/
#define EBDA_START                  0x9d000
#define EBDA_SIZE                   0x03000
#define EBDA_END                    (EBDA_START + (EBDA_SIZE - 1))
/*********************************************************************************************************
  IMCR related bits
*********************************************************************************************************/
#define IMCR_ADRS                   0x22                            /*  IMCR addr reg                   */
#define IMCR_DATA                   0x23                            /*  IMCR data reg                   */
#define IMCR_REG_SEL                0x70                            /*  IMCR reg select                 */
#define IMCR_IOAPIC_ON              0x01                            /*  IMCR IOAPIC route enable        */
#define IMCR_IOAPIC_OFF             0x00                            /*  IMCR IOAPIC route disable       */

#endif                                                              /*  __X86_ARCH_PC_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
