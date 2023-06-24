/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "riscv_csr_encoding.h"
#include "soc.h"
#include "uart.h"
#include "irq_ctrl.h"

#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */

/*-----------------------------------------------------------*/
uint8_t val;

extern int main(void);
extern void freertos_risc_v_trap_handler(void);
extern void vSystemIrqHandler(uint32_t mcause);
/*
 * Prototypes for the standard FreeRTOS callback/hook functions implemented
 * within this file.  See https://www.freertos.org/a00016.html
 */
void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationTickHook(void);

/* Prepare hardware to run the demo. */
static void prvSetupHardware(void);

void setup(void) {
	prvSetupHardware();

  dbg("\n\rFreeRTOS %s running\n\n", tskKERNEL_VERSION_NUMBER);
  main();
}

/* Handle specific interrupts */
void freertos_risc_v_application_interrupt_handler(void) {
  uint32_t asynctrap_cause = 0;
  // acknowledge/clear ALL pending interrupt sources here - adapt this for your setup
  /*write_csr(mip, (0 << IRQ_M_SOFT));*/
  /*write_csr(mip, (0 << IRQ_M_TIMER));*/
  // debug output - Use the value from the mcause CSR to call interrupt-specific handlers
  //dbg("\n%c\n",getchar());
  asm volatile("csrr %0,mcause" : "=r"(asynctrap_cause));
  /*dbg("\n<IRQ> mcause = 0x%x </IRQ>\n", asynctrap_cause);*/
  vSystemIrqHandler(asynctrap_cause);
  write_csr(mip, (0 << IRQ_M_EXT));
}

/* Handle specific exceptions */
void freertos_risc_v_application_exception_handler(void) {
  uint32_t synctrap_cause = 0;
  // debug output - Use the value from the mcause CSR to call exception-specific handlers
  asm volatile("csrr %0,mcause" : "=r"(synctrap_cause));
  dbg("\n<EXC> mcause = 0x%x </EXC>\n", synctrap_cause);
}

static void prvSetupHardware(void) {
  vIRQSetMask(irqMASK_ALL); // Mask all IRQs for now

  // install the freeRTOS trap handler
  asm volatile("csrw mtvec, %0":: "r"((uint8_t *)(&freertos_risc_v_trap_handler)));

  // Disable SW IRQ (UART) - Used by bootloader
  // MIE CSR = 0x304
  // MIE VAL = 0x1080 => (EXT EN, TIMER EN)
  write_csr(mie,0x1080);
  // init UART at default baud rate, no parity bits, ho hw flow control
  vUartSetup(uartBR_UART);
}

void vApplicationMallocFailedHook(void) {
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
  dbg("FreeRTOS_FAULT: vApplicationMallocFailedHook (solution: increase 'configTOTAL_HEAP_SIZE' in FreeRTOSConfig.h)\n");
	__asm volatile( "ebreak" );
	for( ;; );
}

void vApplicationIdleHook(void) {
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */
  vCpuSleep();
}

/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName) {
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
  dbg("FreeRTOS_FAULT: vApplicationStackOverflowHook\n");
	__asm volatile( "ebreak" );
	for( ;; );
}

void vApplicationTickHook(void) {
  /* The tests in the full demo expect some interaction with interrupts. */
  //extern void vFullDemoTickHook( void );
  //vFullDemoTickHook();
}
