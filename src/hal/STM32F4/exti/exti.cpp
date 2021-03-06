#include <stddef.h>

#include "common/assert.h"
#include "exti.hpp"
#include "gpio/gpio.hpp"
#include "CMSIS/Device/STM32F4xx/Include/stm32f4xx.h"
#include "CMSIS/Include/core_cm4.h"

using namespace hal;

#define IRQ_PRIORITY 3
#define GPIO_AF_15_EVENTOUT 0x0F

static IRQn_Type const irq_list[PIN_QTY] =
{
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn,
	EXTI3_IRQn, EXTI4_IRQn, EXTI9_5_IRQn,
	EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
	EXTI9_5_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn
};

static GPIO_TypeDef *const gpio_list[PORT_QTY] =
{
	GPIOA, GPIOB, GPIOC,
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	GPIOD, GPIOE,
#else
	NULL, NULL,
#endif
#if defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	GPIOF, GPIOG,
#else
	NULL, NULL,
#endif
	GPIOH,
#if defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F415xx) || \
	defined(STM32F417xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	GPIOI,
#else
	NULL,
#endif
#if defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	GPIOJ, GPIOK
#else
	NULL, NULL
#endif
};

static uint32_t const port_list[PORT_QTY] =
{
	SYSCFG_EXTICR1_EXTI0_PA, SYSCFG_EXTICR1_EXTI0_PB, SYSCFG_EXTICR1_EXTI0_PC,
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	SYSCFG_EXTICR1_EXTI0_PD, SYSCFG_EXTICR1_EXTI0_PE,
#else
	0, 0,
#endif
#if defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	SYSCFG_EXTICR1_EXTI0_PF, SYSCFG_EXTICR1_EXTI0_PG,
#else
	0, 0,
#endif
	SYSCFG_EXTICR1_EXTI0_PH,
#if defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F415xx) || \
	defined(STM32F417xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	SYSCFG_EXTICR1_EXTI0_PI,
#else
	0,
#endif
#if defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	SYSCFG_EXTICR1_EXTI0_PJ, SYSCFG_EXTICR1_EXTI0_PK
#else
	0, 0
#endif
};

static uint8_t const src_offset_list[PIN_QTY] =
{
	0, 4, 8,
	0, 4, 8,
	0, 4, 8,
	0, 4, 8
};

static exti *obj_list[PIN_QTY];

static void gpio_af_init(gpio &gpio);

exti::exti(gpio &gpio, trigger_t trigger):
	_gpio(gpio),
	_trigger(trigger),
	_ctx(NULL),
	_cb(NULL)
{
	ASSERT(_trigger <= TRIGGER_BOTH);
	ASSERT(_gpio.mode() == gpio::MODE_DI);
	
	gpio_af_init(_gpio);
	
	/* Enable clock */
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	
	/* Setup EXTI line source */
	uint8_t pin = _gpio.pin();
	uint8_t exti_src = src_offset_list[pin];
	SYSCFG->EXTICR[pin / 2] &= ~(0x0F << exti_src);
	SYSCFG->EXTICR[pin / 2] |= port_list[_gpio.port()] << exti_src;
	
	uint32_t line_bit = 1 << pin;
	/* Setup EXTI mask regs */
	EXTI->EMR &= ~line_bit;
	EXTI->IMR &= ~line_bit;
	
	/* Setup EXTI trigger */
	EXTI->RTSR &= ~line_bit;
	EXTI->FTSR &= ~line_bit;
	if(_trigger == TRIGGER_RISING)
		EXTI->RTSR |= line_bit;
	else if(_trigger == TRIGGER_FALLING)
		EXTI->FTSR |= line_bit;
	else
	{
		EXTI->RTSR |= line_bit;
		EXTI->FTSR |= line_bit;
	}
	
	obj_list[pin] = this;
	
	NVIC_ClearPendingIRQ(irq_list[pin]);
	NVIC_SetPriority(irq_list[pin], IRQ_PRIORITY);
	NVIC_EnableIRQ(irq_list[pin]);
}

exti::~exti()
{
	
}

void exti::cb(cb_t cb, void *ctx)
{
	_cb = cb;
	_ctx = ctx;
}

void exti::on()
{
	ASSERT(_cb);
	
	uint8_t pin = _gpio.pin();
	uint32_t line_bit = 1 << pin;
	/* Clear EXTI line pending bits */
	EXTI->PR |= line_bit;
	/* Setup EXTI line configuration */
	EXTI->IMR |= line_bit;
	
	NVIC_ClearPendingIRQ(irq_list[pin]);
}

void exti::off()
{
	/* Clear EXTI line configuration */
	EXTI->IMR &= ~(1 << _gpio.pin());
}

void exti::trigger(trigger_t trigger)
{
	ASSERT(trigger <= TRIGGER_BOTH);
	
	_trigger = trigger;
	uint32_t line_bit = 1 << _gpio.pin();
	
	if(_trigger == TRIGGER_RISING)
		EXTI->RTSR |= line_bit;
	else if(_trigger == TRIGGER_FALLING)
		EXTI->FTSR |= line_bit;
	else
	{
		EXTI->RTSR |= line_bit;
		EXTI->FTSR |= line_bit;
	}
}

static void gpio_af_init(gpio &gpio)
{
	GPIO_TypeDef *gpio_reg = gpio_list[gpio.port()];
	
	uint8_t pin = gpio.pin();
	if(pin < 8)
	{
		gpio_reg->AFR[0] &= ~(0x0F << (pin * 4));
		gpio_reg->AFR[0] |= GPIO_AF_15_EVENTOUT << (pin * 4);
	}
	else
	{
		gpio_reg->AFR[1] &= ~(0x0F << ((pin - 8) * 4));
		gpio_reg->AFR[1] |= GPIO_AF_15_EVENTOUT << ((pin - 8) * 4);
	}
}

extern "C" void exti_irq_hndlr(hal::exti *obj)
{
	/* Clear EXTI line pending bits */
	EXTI->PR = 1 << obj->_gpio.pin();
	
	if(obj->_cb)
		obj->_cb(obj, obj->_ctx);
}

extern "C" void EXTI0_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[0]);
}

extern "C" void EXTI1_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[1]);
}

extern "C" void EXTI2_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[2]);
}

extern "C" void EXTI3_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[3]);
}

extern "C" void EXTI4_IRQHandler(void)
{
	exti_irq_hndlr(obj_list[4]);
}

extern "C" void EXTI9_5_IRQHandler(void)
{
	uint32_t pending_bit = EXTI->PR;
	
	for(uint8_t i = 5; i <= 9; i++)
	{
		if(pending_bit & (1 << i))
		{
			exti_irq_hndlr(obj_list[i]);
			break;
		}
	}
}

extern "C" void EXTI15_10_IRQHandler(void)
{
	uint32_t pending_bit = EXTI->PR;
	
	for(uint8_t i = 10; i <= 15; i++)
	{
		if(pending_bit & (1 << i))
		{
			exti_irq_hndlr(obj_list[i]);
			break;
		}
	}
}
