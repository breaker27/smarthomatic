
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

// Function Pototype
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

// Function Implementation
void wdt_init(void)
{
	mcusr_mirror = MCUSR;
	MCUSR = 0;
	wdt_disable();
	return;
}
