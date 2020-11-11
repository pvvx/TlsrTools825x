
// use static inline, because, spi flash code must reside in memory..
// these code may be embedd in flash code
#if USE_EXT_FLASH

_attribute_ram_code_ static inline void mspi_high(void){
	BM_SET(reg_gpio_out(CS_EXT_FLASH), (unsigned char)(CS_EXT_FLASH & 0xff));
}

_attribute_ram_code_ static inline void mspi_low(void){
	BM_CLR(reg_gpio_out(CS_EXT_FLASH), (unsigned char)(CS_EXT_FLASH & 0xff));
}

_attribute_ram_code_ static inline u8 mspi_get(void){
	return reg_spi_data;
}

_attribute_ram_code_ static inline void mspi_write(u8 c){
	reg_spi_data = c;
}

_attribute_ram_code_ static inline void mspi_wait(void){
	while(reg_spi_ctrl & FLD_SPI_BUSY);
}

#else

_attribute_ram_code_ static inline void mspi_wait(void){
	while(reg_mspi_ctrl & FLD_MSPI_BUSY)
		;
}

_attribute_ram_code_ static inline void mspi_high(void){
	reg_mspi_ctrl = FLD_MSPI_CS;
}

_attribute_ram_code_ static inline void mspi_low(void){
	reg_mspi_ctrl = 0;
}

_attribute_ram_code_ static inline u8 mspi_get(void){
	return reg_mspi_data;
}

_attribute_ram_code_ static inline void mspi_write(u8 c){
	reg_mspi_data = c;
}

_attribute_ram_code_ static inline void mspi_ctrl_write(u8 c){
	reg_mspi_ctrl = c;
}

_attribute_ram_code_ static inline u8 mspi_read(void){
	mspi_write(0);		// dummy, issue clock 
	mspi_wait();
	return mspi_get();
}

#endif
