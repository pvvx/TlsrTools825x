/*
 * Poject TlsrTool FLOADER
 * pvvx 09/2019
 */
#include "common.h"
#include "analog.h"
#include "anareg_8886.h"
#include "spi_i.h"
#include "flash.h"

// Some place holder for the firmware version etc.

typedef struct {
	volatile u32 faddr;
	volatile u32 pbuf;
	volatile u16 count;
	volatile u16 cmd;
	volatile u16 iack;
	volatile u16 oack;
} sext;

sext ext;

u8	buff[8192+4096];


_attribute_ram_code_ void flash_write_sector(u32 addr, u32 len, u8 *buf) {
	u32 sz = 256;
	while(len) {
		if (len < sz) sz = len;
		flash_write_page(addr, sz, buf);
		addr += sz;
		buf += sz;
		len -= sz;
	}
}

_attribute_ram_code_ int main (void) {

	reg_irq_en = 0; // irq_disable (); //#include "irq_i.h"
	REG_ADDR32(0x60) = 0xff000000;
	//	REG_ADDR32(0x6f) = 0x81;
#if 1
	// Open clk for MCU running
	reg_clk_en0 = 0 // 0x63
	| FLD_CLK0_SPI_EN
	| FLD_CLK0_I2C_EN
//	| FLD_CLK0_RS232_EN
//	| FLD_CLK0_USB_EN
//	| FLD_CLK0_PWM_EN
//	| FLD_CLK0_QDEC_EN
	| FLD_CLK0_SWIRE_EN
	;
	reg_clk_en1 = 0	// 0x64
	| FLD_CLK1_ZB_EN
	| FLD_CLK1_SYS_TIMER_EN
	| FLD_CLK1_DMA_EN
	| FLD_CLK1_ALGM_EN
//	| FLD_CLK1_AES_EN
	;
	reg_clk_en2 = 0 // 0x65
//	| FLD_CLK2_AIF_EN
//	| FLD_CLK2_AUD_EN
//	| FLD_CLK2_DFIFO_EN
	| FLD_CLK2_MC_EN	//?
	| FLD_CLK2_MCIC_EN  //?
	;
	reg_clk_sel = 0 // 0x66
    // Выбор FHS:
//	| (0 << (7))	  	//0x66[7] = 0: 0x70[0] = 0 -> Crystal x 2, 0x70[0] = 1 ->  Crystal
	| (1 << (7))	  	//0x66[7] = 1: 0x70[0] = 0 -> 24 MHz RC, 0x70[0] = 1 ->  Crystal
	// Выбор Sys_clk:
	| (0 << (5)) 	//0x66[6:5] 00: 24MHz clock from RC
//	| (1 << (5)) 	//0x66[6:5] 01: FHS
//	| (2 << (5)) 	//0x66[6:5] 10: FHS divider (0x66[4:0])
//	| (3 << (5)) 	//0x66[6:5] 11: 24MHz Crystal Оscillator * 2 * 2/3 = 32MHz	(2/3 Divider)
	// FHS Divider, при 0x66[6:5] = 2
	| (6) 	  	// reg_clk_sel CLOCK_TYPE_PLL
	;
	reg_fhs_sel = 0 // 0x70
	| 0 // 0x70[0] = 0: Crystal x 2  или 24MHz RC
//	| 1 // 0x70[0] = 1: Crystal
	;
#endif
	// Chip specific init
	analog_write(0x82,0x64);	//areg_clk_setting
	analog_write(0x52,0x80);
	analog_write(0x0b,0x38);
	analog_write(0x8c,0x02);
	analog_write(0x02,0xa2);	//rega_vol_ldo_ctrl
#if 0 		// GPIO wake up disable
	analog_write(0x27,0x00);	//PA wake up disable, rega_wakeup_en_val0
	analog_write(0x28,0x00);	//PB wake up disable, rega_wakeup_en_val1
	analog_write(0x29,0x00);	//PC wake up disable, rega_wakeup_en_val2
	analog_write(0x2a,0x00);	//PD wake up disable, rega_wakeup_en_val3
			// DMA channels disable
	REG_ADDR16(0xc20) = 0x0100;	// reg_dma_chn_en = 0, reg_dma_chn_irq_msk = FLD_DMA_CHN_UART_RX
			// Set DMA buffers hi address
	REG_ADDR32(0xc40) = 0x04040404; // reg_dma0_addrHi, reg_dma1_addrHi, reg_dma2_addrHi, reg_dma3_addrHi = 0x04
	REG_ADDR32(0xc44) = 0x04040404; // reg_dma4_addrHi, reg_dma5_addrHi, reg_dma_ta_addrHi, reg_dma_a3_addrHi = 0x04
	REG_ADDR8(0xc48) = 0x04;	// reg_dma7_addrHi = 0x04

	REG_ADDR16(0x750) = 0x1F40;
	analog_write(0x01,((REG_ADDR8(0x7D) == 1)? 0x3c : 0x4c)); // rega_xtal_ctrl
//	reg_gpio_wakeup_irq |= FLD_GPIO_WAKEUP_EN | FLD_GPIO_INTERRUPT_EN; // [0x5b5]|=0x0c
#else
	REG_ADDR16(0xc20) = 0x0100;	// reg_dma_chn_en = 0, reg_dma_chn_irq_msk = FLD_DMA_CHN_UART_RX
#endif
	// select 24M as the system clock source.
	// clock.c : rc_24m_cal
    analog_write(0xc8, 0x80);
    analog_write(0x30, analog_read(0x30) | BIT(7));
    analog_write(0xc7, 0x0e);
    analog_write(0xc7, 0x0f);
    while((analog_read(0xcf) & 0x80) == 0);
    analog_write(0x33, analog_read(0xcb));		//write 24m cap into manual register
    analog_write(0x30, analog_read(0x30) & (~BIT(7)));
    analog_write(0xc7, 0x0e);

#if 0 // не уменьшает power
    analog_write(rega_pwdn_setting1,0
//    | FLDA_32K_RC_PWDN
    | FLDA_32K_XTAL_PWDN
//    | FLDA_32M_RC_PWDN
    | FLDA_32M_XTAL_PWDN
//    | FLDA_LDO_PWDN
//    | FLDA_BGIREF_3V_PWDN
    | FLDA_COMP_PWDN
    | FLDA_TEMPSEN_PWDN
    );
#endif
	// enable system tick ( clock_time() )
	reg_system_tick_ctrl = FLD_SYSTEM_TICK_START; //	REG_ADDR8(0x74f) = 0x01;
	/////////////////////////// app floader /////////////////////////////
	ext.faddr = 0;
	ext.pbuf = (u32) buff;
	ext.count = sizeof(buff);
	ext.cmd = FLASH_GET_JEDEC_ID;
#if USE_EXT_FLASH
	ext.iack = 0x1004; // Version, in BCD 0x1234 = 1.2.3.4
#else
	ext.iack = 0x0005; // Version, in BCD 0x1234 = 1.2.3.4
#endif
	ext.oack = 0;
	volatile u16 ack = 0xffff;
	while(1) {
#if MODULE_WATCHDOG_ENABLE
		WATCHDOG_CLEAR;  //in case of watchdog timeout
#endif
		while(ack == ext.iack);
		ack = ext.iack;
		switch(ext.cmd) {
			case FLASH_READ_CMD:
				flash_read_page(ext.faddr, ext.count, (u8 *)ext.pbuf);
				break;
			case FLASH_WRITE_CMD:
				flash_write_sector(ext.faddr, ext.count, (u8 *)ext.pbuf);
				break;
			case FLASH_SECT_ERASE_CMD:
				flash_erase_sector(ext.faddr);
				break;
			case FLASH_GET_JEDEC_ID:
				ext.faddr = flash_get_jedec_id();
				break;
/*
			case 0xF3:
				analog_write_blk((u8)ext.faddr, (u8 *)ext.pbuf, ext.count);
				break;
			case 0xF4:
				analog_read_blk((u8)ext.faddr, (u8 *)ext.pbuf, ext.count);
				break;
*/
		}
//		SET_FLD(reg_tmr_ctrl, FLD_CLR_WD); // wd_clear
		ext.oack = ack;
	}

	REG_ADDR8(0x6f) = 0x20;   //mcu reboot
	while (1);
}


