/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2015, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */
#include "mbed_assert.h"
#include "pinmap.h"
#include "PortNames.h"
#include "mbed_error.h"

// GPIO mode look-up table
// Warning: order must be the same as the one defined in PinNames.h !!!
static const uint32_t gpio_mode[14] = {
    0x00000000, //  0 = GPIO_MODE_INPUT
    0x00000001, //  1 = GPIO_MODE_OUTPUT_PP
    0x00000011, //  2 = GPIO_MODE_OUTPUT_OD
    0x00000002, //  3 = GPIO_MODE_AF_PP
    0x00000012, //  4 = GPIO_MODE_AF_OD
    0x00000003, //  5 = GPIO_MODE_ANALOG
    0x10110000, //  6 = GPIO_MODE_IT_RISING
    0x10210000, //  7 = GPIO_MODE_IT_FALLING
    0x10310000, //  8 = GPIO_MODE_IT_RISING_FALLING
    0x10120000, //  9 = GPIO_MODE_EVT_RISING
    0x10220000, // 10 = GPIO_MODE_EVT_FALLING
    0x10320000, // 11 = GPIO_MODE_EVT_RISING_FALLING
    0x10000000, // 12 = Reset IT and EVT (not in STM32Cube HAL)
    0x0000000B  //13 = GPIO_MODE_ANALOG_ADC_CONTROL
};

// Enable GPIO clock and return GPIO base address
uint32_t Set_GPIO_Clock(uint32_t port_idx)
{
    uint32_t gpio_add = 0;
    switch (port_idx) {
        case PortA:
            gpio_add = GPIOA_BASE;
            __HAL_RCC_GPIOA_CLK_ENABLE();
            break;
        case PortB:
            gpio_add = GPIOB_BASE;
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
        case PortC:
            gpio_add = GPIOC_BASE;
            __HAL_RCC_GPIOC_CLK_ENABLE();
            break;
#if defined(GPIOD_BASE)
        case PortD:
            gpio_add = GPIOD_BASE;
            __HAL_RCC_GPIOD_CLK_ENABLE();
            break;
#endif
#if defined(GPIOE_BASE)
        case PortE:
            gpio_add = GPIOE_BASE;
            __HAL_RCC_GPIOE_CLK_ENABLE();
            break;
#endif
        case PortH:
            gpio_add = GPIOH_BASE;
            __HAL_RCC_GPIOH_CLK_ENABLE();
            break;
        default:
            error("Pinmap error: wrong port number\n");
            break;
    }
    return gpio_add;
}

/**
 * Configure pin (mode, speed, output type and pull-up/pull-down)
 */
void pin_function(PinName pin, int data)
{
    MBED_ASSERT(pin != (PinName)NC);
    // Get the pin informations
    uint32_t mode  = STM_PIN_MODE(data);
    uint32_t pupd  = STM_PIN_PUPD(data);
    uint32_t afnum = STM_PIN_AFNUM(data);

    uint32_t port_index = STM_PORT(pin);
    uint32_t pin_index  = STM_PIN(pin);

    // Enable GPIO clock
    uint32_t gpio_add = Set_GPIO_Clock(port_index);
    GPIO_TypeDef *gpio = (GPIO_TypeDef *)gpio_add;

    // Configure GPIO
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pin       = (uint32_t)(1 << pin_index);
    GPIO_InitStructure.Mode      = gpio_mode[mode];
    GPIO_InitStructure.Pull      = pupd;
    GPIO_InitStructure.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Alternate = afnum;
    HAL_GPIO_Init(gpio, &GPIO_InitStructure);

    // [TODO] Disconnect JTAG-DP + SW-DP signals.
    // Warning: Need to reconnect under reset
    //if ((pin == PA_13) || (pin == PA_14)) {
    //
    //}
    //if ((pin == PA_15) || (pin == PB_3) || (pin == PB_4)) {
    //
    //}
}

/**
 * Configure pin pull-up/pull-down
 */
void pin_mode(PinName pin, PinMode mode)
{
    MBED_ASSERT(pin != (PinName)NC);
    uint32_t port_index = STM_PORT(pin);
    uint32_t pin_index  = STM_PIN(pin);

    // Enable GPIO clock
    uint32_t gpio_add = Set_GPIO_Clock(port_index);
    GPIO_TypeDef *gpio = (GPIO_TypeDef *)gpio_add;

    // Configure pull-up/pull-down resistors
    uint32_t pupd = (uint32_t)mode;
    if (pupd > 2) {
        pupd = 0; // Open-drain = No pull-up/No pull-down
    }
    gpio->PUPDR &= (uint32_t)(~(GPIO_PUPDR_PUPDR0 << (pin_index * 2)));
    gpio->PUPDR |= (uint32_t)(pupd << (pin_index * 2));
}

/*  Internal function for setting the gpiomode/function
 *  without changing Pull mode
 */
void pin_function_gpiomode(PinName pin, uint32_t gpiomode) {

    /* Read current pull state from HW to avoid over-write*/
    uint32_t port_index = STM_PORT(pin);
    uint32_t pin_index  = STM_PIN(pin);
    GPIO_TypeDef *gpio = (GPIO_TypeDef *) Set_GPIO_Clock(port_index);
    uint32_t temp = gpio->PUPDR;
    uint32_t pull = (temp  >> (pin_index * 2U)) & GPIO_PUPDR_PUPDR0;

    /* Then re-use global function for updating the mode part*/
    pin_function(pin, STM_PIN_DATA(gpiomode, pull, 0));
}
