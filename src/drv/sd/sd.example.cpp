
#include "gpio/gpio.hpp"
#include "spi/spi.hpp"
#include "drv/sd/sd_spi.hpp"
#include "drv/di/di.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace hal;
using namespace drv;

static void sd_cb(di *di, bool state, void *ctx);

static void di_task(void *pvParameters)
{
	di *cd_di = (di *)pvParameters;
	while(1)
	{
		cd_di->poll();
		vTaskDelay(1);
	}
}

int main(void)
{
	static gpio spi1_mosi(0, 7, gpio::MODE_AF, 0);
	static gpio spi1_miso(0, 6, gpio::MODE_AF, 0);
	static gpio spi1_clk(0, 5, gpio::MODE_AF, 0);
	static gpio sd_cs(0, 4, gpio::MODE_DO, 1);
	static gpio sd_cd(0, 3, gpio::MODE_DI, 1);
	
	static dma spi1_rx_dma(dma::DMA_2, dma::STREAM_0, dma::CH_3,
		dma::DIR_PERIPH_TO_MEM, dma::INC_SIZE_8);
	static dma spi1_tx_dma(dma::DMA_2, dma::STREAM_3, dma::CH_3,
		dma::DIR_MEM_TO_PERIPH, dma::INC_SIZE_8);
	
	static spi spi1(spi::SPI_1, 1000000, spi::CPOL_0, spi::CPHA_0,
		spi::BIT_ORDER_MSB, spi1_tx_dma, spi1_rx_dma, spi1_mosi, spi1_miso,
		spi1_clk);
	
	static sd_spi sd1(spi1, sd_cs, &sd_cd);
	
	static di cd_di(sd_cd, 30, 1);
	cd_di.cb(sd_cb, &sd1);
	
	xTaskCreate(di_task, "di", configMINIMAL_STACK_SIZE * 3, &cd_di,
		tskIDLE_PRIORITY + 1, NULL);
	
	vTaskStartScheduler();
}

static void sd_cb(di *di, bool state, void *ctx)
{
	if(state)
		return;
	
	sd *sd1 = (sd *)ctx;
	
	int8_t res = sd1->init();
	if(res != sd::RES_OK)
		return;
	
	sd_csd_t csd;
	res = sd1->read_csd(&csd);
}
