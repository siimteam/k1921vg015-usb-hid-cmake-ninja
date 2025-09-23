/*==============================================================================
 * Пример работы с USB HID для K1921VG015
 *
 * Осуществляется эмуляция USB HID устройства - джойстик.
 * После подключения к ПК в "Панель управления\Оборудование и звук\Устройства и принтеры"
 * отобразится устройство "NIIET USB HID device".
 * Далее правой кнопкой мыши нажимаем по изображению данного джойстика и в меню выбираем
 * "Параметры игровых устройств управления" и в появившемся окне нажимаем кнопку
 * "Свойства". Далее в окне свойств на вкладке "Проверка" будут отображены 8 кнопок,
 *  которые по очереди с интервалом около 1 секунды будут переключаться в активное состояние.
 *------------------------------------------------------------------------------
 * НИИЭТ, Александр Дыхно <dykhno@niiet.ru>
 * НИИЭТ, Дмитрий Сериков <lonni@niiet.ru>
 *==============================================================================
 * ДАННОЕ ПРОГРАММНОЕ ОБЕСПЕЧЕНИЕ ПРЕДОСТАВЛЯЕТСЯ «КАК ЕСТЬ», БЕЗ КАКИХ-ЛИБО
 * ГАРАНТИЙ, ЯВНО ВЫРАЖЕННЫХ ИЛИ ПОДРАЗУМЕВАЕМЫХ, ВКЛЮЧАЯ ГАРАНТИИ ТОВАРНОЙ
 * ПРИГОДНОСТИ, СООТВЕТСТВИЯ ПО ЕГО КОНКРЕТНОМУ НАЗНАЧЕНИЮ И ОТСУТСТВИЯ
 * НАРУШЕНИЙ, НО НЕ ОГРАНИЧИВАЯСЬ ИМИ. ДАННОЕ ПРОГРАММНОЕ ОБЕСПЕЧЕНИЕ
 * ПРЕДНАЗНАЧЕНО ДЛЯ ОЗНАКОМИТЕЛЬНЫХ ЦЕЛЕЙ И НАПРАВЛЕНО ТОЛЬКО НА
 * ПРЕДОСТАВЛЕНИЕ ДОПОЛНИТЕЛЬНОЙ ИНФОРМАЦИИ О ПРОДУКТЕ, С ЦЕЛЬЮ СОХРАНИТЬ ВРЕМЯ
 * ПОТРЕБИТЕЛЮ. НИ В КАКОМ СЛУЧАЕ АВТОРЫ ИЛИ ПРАВООБЛАДАТЕЛИ НЕ НЕСУТ
 * ОТВЕТСТВЕННОСТИ ПО КАКИМ-ЛИБО ИСКАМ, ЗА ПРЯМОЙ ИЛИ КОСВЕННЫЙ УЩЕРБ, ИЛИ
 * ПО ИНЫМ ТРЕБОВАНИЯМ, ВОЗНИКШИМ ИЗ-ЗА ИСПОЛЬЗОВАНИЯ ПРОГРАММНОГО ОБЕСПЕЧЕНИЯ
 * ИЛИ ИНЫХ ДЕЙСТВИЙ С ПРОГРАММНЫМ ОБЕСПЕЧЕНИЕМ.
 *
 *                              2025 АО "НИИЭТ"
 *==============================================================================
 */

//-- Includes ------------------------------------------------------------------
#include <K1921VG015.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <system_k1921vg015.h>
#include <retarget.h>
#include "usbhid.h"

//-- Defines -------------------------------------------------------------------
// #define USE_EP1
#define USE_EP4

#define GPIOA_ALL_Msk 0xFFFF
#define GPIOB_ALL_Msk 0xFFFF

#define LEDS_MSK 0xFF00
#define LED0_MSK (1 << 8)
#define LED1_MSK (1 << 9)
#define LED2_MSK (1 << 10)
#define LED3_MSK (1 << 11)
#define LED4_MSK (1 << 12)
#define LED5_MSK (1 << 13)
#define LED6_MSK (1 << 14)
#define LED7_MSK (1 << 15)

// extern USBDev_TypeDef USBDev_0;
extern uint8_t HID_Buffer[3];
// extern volatile uint8_t hid_report_updated;
uint32_t HID_SendReport(uint8_t *hid_report);

void BSP_led_init()
{
    GPIOA->OUTENSET = LEDS_MSK;
    GPIOA->DATAOUTSET = LEDS_MSK;

    GPIOC->OUTENSET = 0x01;
    GPIOC->DATAOUTCLR = 0x01;
}

void BSP_pu_init()
{
    GPIOC->OUTENSET = 0x04;
    GPIOC->DATAOUTSET = 0x04;
}

//-- Peripheral init functions -------------------------------------------------
void periph_init()
{
    FLASH->CTRL_bit.LAT = 3;
    FLASH->CTRL_bit.CEN = 1;
    //BSP_led_init();
    BSP_pu_init();
    USB_init();
}

//--- USER FUNCTIONS ----------------------------------------------------------------------

void delay(uint32_t val)
{
    for (; val > 0; val--)
        asm("NOP");
}

//-- Main ----------------------------------------------------------------------
int main(void)
{
    uint8_t btn_num;
    SystemInit();
    periph_init();
    // allow all interrupts in machine mode
    PLIC_SetThreshold(Plic_Mach_Target, 0); // allow all interrupts in machine mode
    InterruptEnable();

    HID_Buffer[0] = 0;
    HID_Buffer[1] = 0;
    HID_Buffer[2] = 0;
    btn_num = 0;
    while (1)
    {
        volatile uint32_t cntr;
        if (cntr < 1000000)
        {
            cntr++;
        }
        else
        {
            cntr = 0;
            HID_Buffer[0] = (1 << btn_num);
            btn_num++;
            if (btn_num > 7)
                btn_num = 0;
        }
        HID_SendReport(HID_Buffer);
    };
    return 0;
}
