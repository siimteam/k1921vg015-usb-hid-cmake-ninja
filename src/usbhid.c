/*
 * usbhid.c
 *
 *  Created on: 5 сент. 2024 г.
 *      Author: dav
 */

#include "usbhid.h"

uint8_t line_coding_struct[7]; // заглушка
static const uint8_t device_qualifier[10] = {
    0x0a,
    0x06,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00};

DEVICE_DSCR_TYPE device_dscr;
static const DEVICE_DSCR_TYPE device_dscr_const =
    {
        sizeof(DEVICE_DSCR_TYPE),
        USB_DESC_TYPE_DEVICE,
        {LOBYTE(USB_BCD),
         HIBYTE(USB_BCD)},
        0x00, // 0xFF,//USB_DEVICE_CLASS,//(HID)
        0x00, // USB_DEVICE_SUBCLASS,
        0x00, // 0xFF,//USB_DEVICE_PROTOCOL,
        USB_DEVICE_MAX_EP0_SIZE,
        {LOBYTE(USB_DEVICE_VENDOR_ID),
         HIBYTE(USB_DEVICE_VENDOR_ID)},
        {LOBYTE(USB_DEVICE_PRODUCT_ID),
         HIBYTE(USB_DEVICE_PRODUCT_ID)},
        {LOBYTE(USB_BCD_DEVICE),
         HIBYTE(USB_BCD_DEVICE)},
        USB_DEVICE_IDX_MFC_STR,
        USB_DEVICE_IDX_PRODUCT_STR,
        USB_DEVICE_IDX_SERIAL_STR,
        USB_DEVICE_CFG_MAX_NUM};

static const uint8_t __attribute__((aligned(4))) HID_ReportDesc[42] =
    {
        0x05, 0x01, // USAGE PAGE (generic desktop)
        0x09, 0x05, // USAGE (GamePad)
        0xA1, 0x01, // COLLECTION (Application)
        0xA1, 0x00, // COLLECTION (Physical)
        0x05, 0x09, // USAGE_PAGE (Button)
        0x19, 0x01, // USAGE_MINIMUM (Button 1)
        0x29, 0x08, // USAGE_MAXIMUM (Button 8)
        0x15, 0x00, // LOGICAL_MINIMUM (0)
        0x25, 0x01, // LOGICAL_MAXIMUM (1)
        0x95, 0x08, // REPORT_COUNT (8)
        0x75, 0x01, // REPORT_SIZE (1)
        0x81, 0x02, // INPUT (Data,Var,Abs)
        0x05, 0x01, // USAGE_PAGE (Generic desktop)
        0x09, 0x30, // USAGE (X)
        0x09, 0x31, // USAGE (Y)
        0x15, 0x81, // LOGICAL_MINIMUM (-127)
        0x25, 0x7F, // LOGICAL_MAXIMUM (127)
        0x75, 0x08, // REPORT_SIZE (8)
        0x95, 0x02, // REPORT_COUNT (2)
        0x81, 0x03, // INPUT (Data,Var,Abs)
        0xC0,       // END_COLLECTION
        0xC0        // END_COLLECTION

};

static const uint8_t __attribute__((aligned(4))) cfg_dscr[34] =
    {
        sizeof(CONFIGURATION_DSCR_TYPE),
        USB_DESC_TYPE_CONFIGURATION,
        LOBYTE(sizeof(cfg_dscr)),
        HIBYTE(sizeof(cfg_dscr)),
        0x01, // bNumInterfaces
        0x01, // bConfigurationValue,
        0x00, // iConfiguration
        0xc0, // ???
        0x32, // max 100 mA

        sizeof(INTERFACE_DSCR_TYPE),
        USB_DESC_TYPE_INTERFACE,
        0x00, // bInterfaceNumber
        0x00, // bAlternateSetting
        0x01, // bNumEndpoints
        0x03, // bInterfaceClass (HID)
        0x00, // bInterfaceSubClass (0 = no boot)
        0x00, // bInterfaceProtocol (0 = none, 1 = kbd, 2 = mouse)
        0x00, // iInterface
        // header func descr
        0x09,                // HID desc size
        HID_DESCRIPTOR_TYPE, // HID desc type
        LOBYTE(HID_CLASS_SPEC_RELEASE_NUMBER),
        HIBYTE(HID_CLASS_SPEC_RELEASE_NUMBER),
        0x00, // Target country
        0x01, // bNum HID descrs
        0x22, // bDescrType
        LOBYTE(sizeof(HID_ReportDesc)),
        HIBYTE(sizeof(HID_ReportDesc)),
        // mouse EP descr
        0x07,
        USB_DESC_TYPE_ENDPOINT,
        EP_DATA_IN, // EP address (IN)
        0x03,       // interrupt EP
        LOBYTE(HID_IN_PACKET),
        HIBYTE(HID_IN_PACKET),
        0x0A // SOF interval polling
};

USBDev_TypeDef USBDev_0;

uint8_t HID_Buffer[3];
volatile uint8_t hid_report_updated = 0;

uint32_t USBDev_CEPSendResponse(CEP_RESPONSE resp)
{
    USB->CEP_CTRL_STAT = resp;
    // NT_USBDevice->CEP_CTRL_STAT = resp;
    return 0;
}

uint32_t get_strlen(char *str)
{
    char *pstr = str;
    uint32_t i = 0;

    while ((*pstr++) != 0)
    {
        i++;
    }
    return i;
}

uint32_t get_string_to_unicode(char *desc, uint8_t *unicode)
{
    uint8_t i = 0;
    unicode[i++] = get_strlen(desc) * 2 + 2;
    unicode[i++] = USB_DESC_TYPE_STRING;

    while (*desc != 0)
    {
        unicode[i++] = *desc++;
        unicode[i++] = 0x00;
    }
    return unicode[0];
}

uint8_t __attribute__((aligned(1024))) dma_tmpbuf[1024]; // for max ISOC HIGHSPEED transaction

USBDev_State_TypeDef USBDev_GetState(USBDev_TypeDef *usbdev);
uint32_t HID_SendReport(uint8_t * /*hid_report_t* */ hid_report);

void USBPLL_init()
{
    uint32_t timeout_counter = 0;
    USB->PLLUSBCFG0 = (7 << RCU_PLLSYSCFG0_PD1B_Pos) |   // PD1B
                      (7 << RCU_PLLSYSCFG0_PD1A_Pos) |   // PD1A
                      (1 << RCU_PLLSYSCFG0_PD0B_Pos) |   // PD0B 120FPRE/(1+1) = 60FOUT
                      (7 << RCU_PLLSYSCFG0_PD0A_Pos) |   // PD0A 960FVCO/(1+7) = 120FPRE
                      (1 << RCU_PLLSYSCFG0_REFDIV_Pos) | // refdiv 16/1 = 16FREF
                      (0 << RCU_PLLSYSCFG0_FOUTEN_Pos) | // fouten
                      (0 << RCU_PLLSYSCFG0_DSMEN_Pos) |  // dsmen
                      (0 << RCU_PLLSYSCFG0_DACEN_Pos) |  // dacen
                      (3 << RCU_PLLSYSCFG0_BYP_Pos) |    // bypass
                      (1 << RCU_PLLSYSCFG0_PLLEN_Pos);   // en
    USB->PLLUSBCFG1 = 0;                                 // FRAC = 0
    USB->PLLUSBCFG2 = 59;                                // FBDIV 16FREF*60=960FVCO
    USB->PLLUSBCFG0_bit.FOUTEN = 1;                      // Fout0 Enable
    timeout_counter = 1000;
    while (timeout_counter)
        timeout_counter--;
    while ((USB->PLLUSBSTAT_bit.LOCK) != 1)
    {
    };                           // wait lock signal
    USB->PLLUSBCFG0_bit.BYP = 2; // Bypass for Fout1

    USB->PLLUSBCFG3_bit.USBCLKSEL = 0; // 0-PLLUSBClk (FOUT0); 1- SYSClk
}

void USBDev_Init(USBDev_TypeDef *usbdev)
{
    // set status to powered
    usbdev->curr_state = Powered;
    usbdev->dev_addr = 0;
    // enable interrupt for reset line status
    USB->INTEN1_bit.RESSTATUS = 1; // enable IRQ for USB reset line status
    USB->INTEN1_bit.SUSPEND = 1;   // interrupt on suspend line
    USB->INTEN1_bit.RESUME = 1;    // interrupt on resume line

    // enable global IRQ for USB
    PLIC_SetIrqHandler(Plic_Mach_Target, IsrVect_IRQ_USB, USBIntHandler);
    PLIC_SetPriority(IsrVect_IRQ_USB, 0x1);
    PLIC_IntEnable(Plic_Mach_Target, IsrVect_IRQ_USB);
}

void USB_init()
{
    memcpy(&device_dscr, &device_dscr_const, sizeof(DEVICE_DSCR_TYPE));

    RCU->CGCFGAHB_bit.USBEN = 1;
    RCU->RSTDISAHB_bit.USBEN = 1;
    USBPLL_init();
    USB->PLLUSBCFG3_bit.USBCLKSEL = 0; // 0-PLLUSBClk (FOUT0); 1- SYSClk
    USB->PHY_PD = 0;
    //*((uint32_t*)(0x20010704)) = (1<<11);
    // USBDev_Init();
    USBDev_Init(&USBDev_0);
}

uint32_t USBDev_DMAOpStart(uint8_t *pbuf, uint32_t len, uint8_t epnum, uint32_t op)
{
    uint32_t wr_to_ep = USBDMA_OP_WRITE_TO_EP & op;

    if (len > 1024)
        return 1; // avoid buffer overflow

    if (wr_to_ep != 0)
        memcpy(dma_tmpbuf, pbuf, len);

    USB->AHB_DMA_ADDR = (uint32_t)&dma_tmpbuf[0];
    USB->DMA_CNT = len;
    USB->DMA_CTRL_STS = op | epnum;

    // ждем окончания работы DMA
    while (USB->DMA_CTRL_STS & USBDMA_OP_MASK_DONE)
    {
    };

    if (wr_to_ep == 0)
        memcpy(pbuf, &dma_tmpbuf[0], len);

    return 0;
}

uint32_t USBDev_ReadPacket(uint8_t *dst, uint32_t len, uint8_t epnum)
{
    if (len > 0)
        USBDev_DMAOpStart(dst, len, epnum, USBDMA_OP_READ_FROM_EP | USBDMA_OP_START);

    return 0;
}

uint32_t USBDev_WritePacket(uint8_t *src, uint32_t len, uint8_t epnum)
{
    if (len > 0)
    {
        USBDev_DMAOpStart(src, len, epnum, USBDMA_OP_WRITE_TO_EP | USBDMA_OP_START);
        // отправляем пакет
        // if(len < ep->max_packet)
#ifdef USE_EP4
        USB->USB_EP[3].RSP_SC = USB_USB_EP_RSP_SC_PKTEND_Msk;
#endif

#ifdef USE_EP1
        USB->USB_EP[3].RSP_SC = USB_USB_EP_RSP_SC_PKTEND_Msk;
#endif
        // USBDev_EPSetResponse(EP_PKTEND, epnum_mask);
    }

    return 0;
}

uint32_t USBDev_PutNextCEPPacket(void)
{
    uint32_t data_size;

    if ((USBDev_0.setup_pkt.xfer_len - USBDev_0.setup_pkt.xfer_count) > USB_DEVICE_MAX_EP0_SIZE)
        data_size = USB_DEVICE_MAX_EP0_SIZE;
    else
        data_size = USBDev_0.setup_pkt.xfer_len - USBDev_0.setup_pkt.xfer_count;

    if (data_size == 0)
        USBDev_CEPSendResponse(CEP_ZEROLEN);
    else
    {
        USBDev_DMAOpStart((uint8_t *)(USBDev_0.setup_pkt.xfer_buff + USBDev_0.setup_pkt.xfer_count),
                          data_size, 0, USBDMA_OP_WRITE_TO_EP | USBDMA_OP_START);
        USB->CEP_IN_XFRCNT = data_size;

        USBDev_0.setup_pkt.xfer_count += data_size;
    }

    return 0;
}

uint32_t USBDev_EPSendData(uint8_t *pbuf, uint32_t len, uint8_t epnum)
{
    if (epnum == 0) // if CEP
    {
        if (len == 0)
        {
            USBDev_CEPSendResponse(CEP_ZEROLEN);
            return 0;
        }

        USBDev_0.setup_pkt.xfer_buff = pbuf;
        USBDev_0.setup_pkt.xfer_count = 0;
        USBDev_0.setup_pkt.xfer_len = len;

        USBDev_PutNextCEPPacket();
    }
    else // if ep 1...4
    {
        /*if(USBDev_IsEPTypeIN(epnum) == 0)
                return 1; //error! ep is not IN-endpoint

        if(len == 0) //send zerolen packet
        {
            USBDev_EPSetResponse(EP_ZEROLEN,epnum);
            return 0;
        }
        //else len != 0
        USBDevice.in_ep[epnum - 1].xfer_buff  = pbuf;
        USBDevice.in_ep[epnum - 1].xfer_count = 0;
        USBDevice.in_ep[epnum - 1].xfer_len   = len;
*/
        // TODO!!!
        // USBDev_PutNextEPPacket(epnum);
    }

    return 0;
}

uint32_t USBDev_SetSetupStage(USBDev_SetupStage_TypeDef stage)
{
    USBDev_0.setup_pkt.stage = stage;
    return 0;
}

uint32_t USBD_GetDescriptorReq(USBDev_Setup_Pkt *pUSB_SETUP_PACKET)
{
    uint32_t len = 0;
    uint8_t *pbuf = NULL;
    uint8_t __attribute__((aligned(4))) local_buffer[256];

    switch ((uint8_t)(pUSB_SETUP_PACKET->wValue >> 8))
    {
    case USB_DESC_TYPE_DEVICE:
        if (USBDev_GetState(&USBDev_0) == Default)
            len = 8;
        else
            len = sizeof(device_dscr);
        pbuf = (uint8_t *)&device_dscr;
        break;
    case USB_DESC_TYPE_CONFIGURATION:
        // GPIOB->DATAOUTSET = 0x04;
        pbuf = (uint8_t *)&cfg_dscr;
        len = sizeof(cfg_dscr);
        /* if(USBDevice.speed == USBDev_Speed_Full)
             pbuf = USBDevice.class_cb->GetConfigDscr(USBDevice.speed, &len);
         else
             pbuf = USBDevice.class_cb->GetConfigDscr(USBDevice.speed, &len);*/
        break;
    case USB_DESC_TYPE_STRING:

        pbuf = &local_buffer[0];

        switch ((uint8_t)(pUSB_SETUP_PACKET->wValue & 0xff))
        {
        case USBD_IDX_LANGID_STR:

            local_buffer[0] = 4;
            local_buffer[1] = USB_DESC_TYPE_STRING;
            local_buffer[2] = LOBYTE(USB_DEVICE_LANGID_STR);
            local_buffer[3] = HIBYTE(USB_DEVICE_LANGID_STR);

            len = MIN(4, pUSB_SETUP_PACKET->wLength);
            break;

        case USBD_IDX_MFC_STR:
            len = MIN(get_string_to_unicode(USB_DEVICE_MFC_STR, local_buffer), pUSB_SETUP_PACKET->wLength);
            break;

        case USBD_IDX_PRODUCT_STR:
            len = MIN(get_string_to_unicode(USB_DEVICE_PRODUCT_STR, local_buffer), pUSB_SETUP_PACKET->wLength);
            break;

        case USBD_IDX_SERIAL_STR:
            len = MIN(get_string_to_unicode(USB_DEVICE_SERIAL_STR, local_buffer), pUSB_SETUP_PACKET->wLength);
            break;

        case USBD_IDX_CONFIG_STR:
            len = MIN(get_string_to_unicode(USB_DEVICE_CONFIG_STR, local_buffer), pUSB_SETUP_PACKET->wLength);
            break;

        case USBD_IDX_INTERFACE_STR:
            len = MIN(get_string_to_unicode(USB_DEVICE_INTERFACE_STR, local_buffer), pUSB_SETUP_PACKET->wLength);
            break;

        default:
            USBDev_CEPSendResponse(CEP_STALL);
            break;
        }
        break;

    case USB_DESC_TYPE_DEVICE_QUALIFIER:
        // USBDev_CEPSendResponse(CEP_STALL);
        len = sizeof(device_qualifier);
        pbuf = (uint8_t *)&device_qualifier;
        break;

    default:
        USBDev_CEPSendResponse(CEP_STALL);
        break;
    }

    if ((len != 0) && ((uint16_t)(pUSB_SETUP_PACKET->wLength) != 0))
    {
        USBDev_EPSendData(pbuf, MIN(len, pUSB_SETUP_PACKET->wLength), 0);
        USBDev_SetSetupStage(USBDev_SetupStage_Data);
    }

    // GPIOB->DATAOUTCLR = 0x04;

    return 0;
}

uint32_t USBD_SetAddressReq(void)
{
    if ((USBDev_0.setup_pkt.wIndex == 0) && (USBDev_0.setup_pkt.wLength == 0))
    {
        if (USBDev_0.curr_state == Configured)
            USBDev_CEPSendResponse(CEP_STALL);
        else
        {
            USBDev_0.setup_pkt.set_addr_active_flag = 1;
            USBDev_0.dev_addr = (uint8_t)(USBDev_0.setup_pkt.wValue) & 0x7F;
            USBDev_SetSetupStage(USBDev_SetupStage_Status);
            // нулевой пакет подтверждения
            USBDev_CEPSendResponse(CEP_ZEROLEN);

            if (USBDev_0.dev_addr != 0)
                USBDev_0.curr_state = Addressed;
            else
                USBDev_0.curr_state = Default;
        }
    }
    else
        USBDev_CEPSendResponse(CEP_STALL);

    return 0;
}

/*
 * Функция настройки конечных точек в зависимости от заданной конфигурации
 * */
void USBD_SetCfg(uint8_t cfgNum)
{
    if (cfgNum != 0)
    {

#ifdef USE_EP4
        // Common settings for EP[3]
        USB->USB_EP[3].MPS = HID_IN_PACKET;
        USB->USB_EP[3].USB_EP_CFG = 0x01 | (USB_USB_EP_USB_EP_CFG_EP_DIR_IN << USB_USB_EP_USB_EP_CFG_EP_DIR_Pos) | // 2 - Int
                                    (1 << USB_USB_EP_USB_EP_CFG_EP_NUM_Pos) |
                                    (USB_USB_EP_USB_EP_CFG_EP_TYPE_Int << USB_USB_EP_USB_EP_CFG_EP_TYPE_Pos);
        USB->USB_EP[3].RSP_SC = (USB_USB_EP_RSP_SC_MODE_Auto << USB_USB_EP_RSP_SC_MODE_Pos) | 0x1;
        USB->USB_EP[3].START_ADDR = USB->CEP_END_ADDR + 1;
        USB->USB_EP[3].END_ADDR = USB->USB_EP[3].START_ADDR + 64 - 1;
        USB->USB_EP[3].IRQ_ENB = USB_USB_EP_IRQ_ENB_DATAPKTTRINTEN_Msk | USB_USB_EP_IRQ_ENB_ERRSENTINTEN_Msk;
        USB->INTEN0 |= USB_INTEN0_EP3_INTEN_Msk;
#endif
#ifdef USE_EP1
        // common settings
        USB->USB_EP[3].MPS = HID_IN_PACKET;
        USB->USB_EP[3].USB_EP_CFG = (USB_USB_EP_USB_EP_CFG_EP_TYPE_Int << USB_USB_EP_USB_EP_CFG_EP_TYPE_Pos);
        USB->USB_EP[3].RSP_SC = (USB_USB_EP_RSP_SC_MODE_Auto << USB_USB_EP_RSP_SC_MODE_Pos);
        // ep1 configure
        USB->USB_EP[0].USB_EP_CFG = 0x01 | (USB_USB_EP_USB_EP_CFG_EP_DIR_IN << USB_USB_EP_USB_EP_CFG_EP_DIR_Pos) | // 2 - Int
                                    (1 << USB_USB_EP_USB_EP_CFG_EP_NUM_Pos) |
                                    (USB_USB_EP_USB_EP_CFG_EP_TYPE_Int << USB_USB_EP_USB_EP_CFG_EP_TYPE_Pos);
        USB->USB_EP[0].RSP_SC = (USB_USB_EP_RSP_SC_MODE_Auto << USB_USB_EP_RSP_SC_MODE_Pos) | 0x1;
        USB->USB_EP[0].START_ADDR = 0x100; // USB->CEP_END_ADDR + 1;
        USB->USB_EP[0].END_ADDR = USB->USB_EP[0].START_ADDR + 64 - 1;
        USB->USB_EP[0].IRQ_ENB = USB_USB_EP_IRQ_ENB_DATAPKTTRINTEN_Msk | USB_USB_EP_IRQ_ENB_ERRSENTINTEN_Msk;
        USB->INTEN0 |= USB_INTEN0_EP0_INTEN_Msk;
#endif
        hid_report_updated = 0;

        /*
                USB->USB_EP[0].USB_EP_CFG = 0x01 | (USB_USB_EP_USB_EP_CFG_EP_DIR_IN << USB_USB_EP_USB_EP_CFG_EP_DIR_Pos) |
                                                   (1 << USB_USB_EP_USB_EP_CFG_EP_NUM_Pos) |
                                                   (2 << USB_USB_EP_USB_EP_CFG_EP_TYPE_Pos) ;
                USB->USB_EP[0].START_ADDR = USB->CEP_END_ADDR + 1;
                USB->USB_EP[0].END_ADDR = USB->USB_EP[0].START_ADDR + 64 - 1;
                USB->USB_EP[0].RSP_SC = 1; //buff flush

        //		USB->USB_EP[1].RSP_SC = (USB_USB_EP_RSP_SC_MODE_Fly << USB_USB_EP_RSP_SC_MODE_Pos) | 0x1;
                USB->USB_EP[0].IRQ_ENB = USB_USB_EP_IRQ_ENB_INTOKENINTEN_Msk | USB_USB_EP_IRQ_ENB_DATAPKTTRINTEN_Msk;

                //USB->USB_EP[1].DATA_BUF = 0x5533;
                //USB->USB_EP[1].CNT = 0x2;


                USB->USB_EP[1].USB_EP_CFG = 0x01 | (USB_USB_EP_USB_EP_CFG_EP_DIR_OUT << USB_USB_EP_USB_EP_CFG_EP_DIR_Pos) |
                                                   (2 << USB_USB_EP_USB_EP_CFG_EP_NUM_Pos) |
                                                   (2 << USB_USB_EP_USB_EP_CFG_EP_TYPE_Pos) ;
                USB->USB_EP[1].START_ADDR = USB->USB_EP[0].END_ADDR + 1;
                USB->USB_EP[1].END_ADDR = USB->USB_EP[1].START_ADDR + 64 - 1;
                USB->USB_EP[1].RSP_SC = 1; //buff flush

        //		USB->USB_EP[1].RSP_SC = (USB_USB_EP_RSP_SC_MODE_Fly << USB_USB_EP_RSP_SC_MODE_Pos) | 0x1;
                USB->USB_EP[1].IRQ_ENB = USB_USB_EP_IRQ_ENB_DATAPKTRECINTEN_Msk;

                USB->INTEN0 |= USB_INTEN0_EP0_INTEN_Msk | USB_INTEN0_EP1_INTEN_Msk;*/

        /*USB->USB_EP[0].START_ADDR = USB->CEP_END_ADDR + 1;
        USB->USB_EP[0].END_ADDR = USB->USB_EP[0].START_ADDR + 64 - 1;
        USB->USB_EP[0].USB_EP_CFG = (USB_USB_EP_USB_EP_CFG_EP_DIR_IN << USB_USB_EP_USB_EP_CFG_EP_DIR_Pos) |
                                    (USB_USB_EP_USB_EP_CFG_EP_TYPE_Bulk << USB_USB_EP_USB_EP_CFG_EP_TYPE_Pos) |
                                    (1 << USB_USB_EP_USB_EP_CFG_EP_NUM_Pos) |
                                    (1 << USB_USB_EP_USB_EP_CFG_EP_VALID_Pos);
        //USB->USB_EP[0].RSP_SC = (USB_USB_EP_RSP_SC_MODE_Fly << USB_USB_EP_RSP_SC_MODE_Pos);
        USB->USB_EP[0].RSP_SC = USB_USB_EP_RSP_SC_BUFFFLUSH_Msk | USB_USB_EP_RSP_SC_EPNAK_Msk | (USB_USB_EP_RSP_SC_MODE_Manual << USB_USB_EP_RSP_SC_MODE_Pos);
        USB->USB_EP[0].MPS = 64;
        USB->USB_EP[0].IRQ_ENB = USB_USB_EP_IRQ_ENB_DATAPKTTRINTEN_Msk;

        USB->USB_EP[1].START_ADDR = USB->USB_EP[0].END_ADDR + 1;
        USB->USB_EP[1].END_ADDR = USB->USB_EP[1].START_ADDR + 64 - 1;
        USB->USB_EP[1].USB_EP_CFG = (USB_USB_EP_USB_EP_CFG_EP_DIR_OUT << USB_USB_EP_USB_EP_CFG_EP_DIR_Pos) |
                                    (USB_USB_EP_USB_EP_CFG_EP_TYPE_Bulk << USB_USB_EP_USB_EP_CFG_EP_TYPE_Pos) |
                                    (2 << USB_USB_EP_USB_EP_CFG_EP_NUM_Pos) |
                                    (1 << USB_USB_EP_USB_EP_CFG_EP_VALID_Pos);
        USB->USB_EP[1].RSP_SC = USB_USB_EP_RSP_SC_BUFFFLUSH_Msk;
        USB->USB_EP[1].MPS = 64;
        USB->USB_EP[1].IRQ_ENB = USB_USB_EP_IRQ_ENB_DATAPKTRECINTEN_Msk;

        USB->USB_EP[2].START_ADDR = USB->USB_EP[1].END_ADDR + 1;
        USB->USB_EP[2].END_ADDR = USB->USB_EP[2].START_ADDR + 64 - 1;
        USB->USB_EP[2].USB_EP_CFG = (USB_USB_EP_USB_EP_CFG_EP_DIR_IN << USB_USB_EP_USB_EP_CFG_EP_DIR_Pos) |
                                    (USB_USB_EP_USB_EP_CFG_EP_TYPE_Int << USB_USB_EP_USB_EP_CFG_EP_TYPE_Pos) |
                                    (3 << USB_USB_EP_USB_EP_CFG_EP_NUM_Pos) |
                                    (1 << USB_USB_EP_USB_EP_CFG_EP_VALID_Pos);
        USB->USB_EP[2].RSP_SC = USB_USB_EP_RSP_SC_BUFFFLUSH_Msk | USB_USB_EP_RSP_SC_EPNAK_Msk | (USB_USB_EP_RSP_SC_MODE_Manual << USB_USB_EP_RSP_SC_MODE_Pos);
        USB->USB_EP[2].MPS = 64;
        //USB->USB_EP[2].IRQ_ENB = USB_USB_EP_IRQ_ENB_DATAPKTRECINTEN_Msk;

        USB->INTEN0 |= USB_INTEN0_EP0_INTEN_Msk | USB_INTEN0_EP1_INTEN_Msk;*/
    }
}

uint32_t USBD_SetConfigReq(void)
{
    if (USBDev_0.setup_pkt.wValue > USB_DEVICE_CFG_MAX_NUM)
    {
        USBDev_CEPSendResponse(CEP_STALL);
    }
    else
    {
        switch (USBDev_0.curr_state)
        {
        case Addressed:
            if (USBDev_0.setup_pkt.wValue) // если номер полученной конфигурации больше нулевой
            {
                USBDev_0.config = USBDev_0.setup_pkt.wValue;

                // TODO:
                USBD_SetCfg(USBDev_0.config);

                USBDev_0.curr_state = Configured;
            }

            USBDev_CEPSendResponse(CEP_ZEROLEN);
            USBDev_SetSetupStage(USBDev_SetupStage_Status);

            break;

        case Configured:
            // если полученная конфигурация равна нулю - деконфигурируем
            if (USBDev_0.setup_pkt.wValue == 0)
            {
                USBDev_0.config = USBDev_0.setup_pkt.wValue;
                USBDev_0.curr_state = Addressed;

                // TODO:
                // USBD_ClrCfg(USBDevice.config);
            }
            // если заданная конфигурация отличается от запрашиваемой, то реконфигурируем
            else if (USBDev_0.config != USBDev_0.setup_pkt.wValue)
            {
                // TODO:
                // USBD_ClrCfg(USBDevice.config);

                USBDev_0.config = USBDev_0.setup_pkt.wValue;

                // TODO:
                // USBD_SetCfg(USBDevice.config);

                USBDev_CEPSendResponse(CEP_ZEROLEN);
            }
            // else //иначе стоит та же самая конфигурация

            USBDev_CEPSendResponse(CEP_ZEROLEN);
            USBDev_SetSetupStage(USBDev_SetupStage_Status);
            break;
        default:
            USBDev_CEPSendResponse(CEP_STALL);
            break;
        }
    }

    return 0;
}

void USBDev_StdDevReq(USBDev_Setup_Pkt *pkt)
{
    switch (pkt->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR:
        USBD_GetDescriptorReq(pkt);
        break;
    case USB_REQ_SET_ADDRESS:
        // GPIOA->DATAOUT = 0x3001;
        USBD_SetAddressReq();
        break;
    case USB_REQ_SET_CONFIGURATION:
        // GPIOA->DATAOUT = 0x3002;
        USBD_SetConfigReq();
        break;
    case USB_REQ_GET_CONFIGURATION:
        // GPIOA->DATAOUT = 0x3003;
        break;
    case USB_REQ_GET_STATUS:
        // GPIOA->DATAOUT = 0x3004;
        break;
    case USB_REQ_SET_FEATURE:
        // GPIOA->DATAOUT = 0x3005;
        break;
    case USB_REQ_CLEAR_FEATURE:
        // GPIOA->DATAOUT = 0x3006;
        break;
    default:
        // GPIOA->DATAOUT = 0x300e;
        break;
    }
}
uint32_t USBDev_EPPrepareRx(uint8_t *pbuf, uint32_t len, uint8_t epnum)
{
    USBDev_0.setup_pkt.xfer_buff = pbuf;
    USBDev_0.setup_pkt.xfer_count = 0;
    USBDev_0.setup_pkt.xfer_len = len;

    if (epnum != 0)
    {
        // отключаем автоответ NAK от точки
        // USBDev_EPSetResponse(EP_CLRNAK, epnum);
        while (1)
        {
        };
    }
    else
        USBDev_SetSetupStage(USBDev_SetupStage_Data);

    return 0;
}

uint32_t USBD_StdItfReq(USBDev_Setup_Pkt *pkt)
{
    uint32_t len = 0;
    uint8_t *pbuf;

    switch (USBDev_0.curr_state)
    {
    case Configured:
        if (pkt->wIndex <= 1)
        {
            //
            GPIOB->DATAOUTSET = 0x04;
            GPIOB->DATAOUTCLR = 0x04;

            switch (pkt->bmRequestType & 0x60)
            {
            case 0x00: // STANDART_REQUEST
                switch (pkt->bRequest)
                {
                case USB_REQ_GET_DESCRIPTOR:
                    //
                    if ((pkt->wValue >> 8) == HID_REPORT_DESCRIPTOR)
                    {
                        len = MIN(sizeof(HID_ReportDesc), pkt->wLength);
                        pbuf = (uint8_t *)HID_ReportDesc;
                    }
                    else if ((pkt->wValue >> 8) == HID_DESCRIPTOR_TYPE)
                    {
                        len = MIN(9, pkt->wLength);
                        pbuf = (uint8_t *)(cfg_dscr + 18);
                    }
                    else
                    {
                        USBDev_CEPSendResponse(CEP_STALL);
                        break;
                    }

                    USBDev_EPSendData(pbuf, len, 0);
                    USBDev_SetSetupStage(USBDev_SetupStage_Data);

                    break;
                default:
                    USBDev_CEPSendResponse(CEP_STALL);
                    break;
                }
                break;
            case 0x20:
                break;
            default:
                USBDev_CEPSendResponse(CEP_STALL);
                break;
            }
        }
        else
            USBDev_CEPSendResponse(CEP_STALL);
        break;

    default:
        USBDev_CEPSendResponse(CEP_STALL);
        break;
    }

    return 0;
}

static uint8_t hid_protocol;
static uint8_t hid_idle_state;

void USBDev_SetupStage(USBDev_TypeDef *usbdev)
{

    uint8_t rq_type = (usbdev->setup_pkt.bmRequestType >> 5) & 0x03;

    switch (rq_type)
    {
    case 0x00: // standard request
        switch (usbdev->setup_pkt.bmRequestType & 0x1F)
        {
        case USB_REQ_RECIPIENT_DEVICE:
            USBDev_StdDevReq(&usbdev->setup_pkt);
            break;
        case USB_REQ_RECIPIENT_INTERFACE:
            USBD_StdItfReq(&usbdev->setup_pkt);
            break;
        case USB_REQ_RECIPIENT_ENDPOINT:
            USBDev_CEPSendResponse(CEP_STALL);
            break;
        default:
            USBDev_CEPSendResponse(CEP_STALL);
            break;
        }
        break;
    case 0x01: // class request
               // GPIOA->DATAOUT = 0x2101;
        switch (usbdev->setup_pkt.bRequest)
        {
        case HID_REQ_SET_PROTOCOL:
            hid_protocol = (uint8_t)(usbdev->setup_pkt.wValue);
            USBDev_CEPSendResponse(CEP_ZEROLEN);
            USBDev_SetSetupStage(USBDev_SetupStage_Status);
            break;
        case HID_REQ_GET_PROTOCOL:
            USBDev_EPSendData(&hid_protocol, 1, 0);
            USBDev_SetSetupStage(USBDev_SetupStage_Data);
            break;
        case HID_REQ_SET_IDLE:
            hid_idle_state = (uint8_t)(usbdev->setup_pkt.wValue);
            USBDev_CEPSendResponse(CEP_ZEROLEN);
            USBDev_SetSetupStage(USBDev_SetupStage_Status);
            break;
        case HID_REQ_GET_REPORT:
            USBDev_EPSendData(HID_Buffer, 3, 0);
            USBDev_SetSetupStage(USBDev_SetupStage_Data);
            break;
        default:
            USBDev_CEPSendResponse(CEP_STALL);
            break;
        }
        break;
    case 0x02: // vendor request
        USBDev_CEPSendResponse(CEP_STALL);
        break;
    default: // reserved
        USBDev_CEPSendResponse(CEP_STALL);
        break;
    }
}

void USBDev_ParseSetupPacket(USBDev_TypeDef *usbdev)
{
    usbdev->setup_pkt.bmRequestType = (uint8_t)(USB->CEP_SETUP1_0 & 0xff);
    usbdev->setup_pkt.bRequest = (uint8_t)((USB->CEP_SETUP1_0 >> 8) & 0xff);
    usbdev->setup_pkt.wValue = USB->CEP_SETUP3_2;
    usbdev->setup_pkt.wIndex = USB->CEP_SETUP5_4;
    usbdev->setup_pkt.wLength = USB->CEP_SETUP7_6;
}

void USBDev_ChangeState(USBDev_TypeDef *usbdev, USBDev_State_TypeDef new_state)
{
    usbdev->prev_state = usbdev->curr_state;
    usbdev->curr_state = new_state;
}

void USBDev_RevertState(USBDev_TypeDef *usbdev)
{
    usbdev->curr_state = usbdev->prev_state;
}

USBDev_State_TypeDef USBDev_GetState(USBDev_TypeDef *usbdev)
{
    return usbdev->curr_state;
}

USBDev_State_TypeDef USBDev_GetPrevState(USBDev_TypeDef *usbdev)
{
    return usbdev->prev_state;
}

void USBDev_SetNewAddr(USBDev_TypeDef *usbdev, uint8_t new_addr)
{
    usbdev->dev_addr = new_addr;
    USB->USBADDR = new_addr;
}

uint8_t USBDev_GetCurrDevAddr(USBDev_TypeDef *usbdev)
{
    return usbdev->dev_addr;
}

uint32_t USBDev_SetAddress(uint8_t address)
{
    USB->USBADDR = address;
    return 0;
}

uint32_t USBDev_GetNextCEPPacket(void)
{
    uint32_t data_size;

    // получаем количество пришедших данных
    data_size = USB->CEP_OUT_XFRCNT;
    // получаем точку и инфу о ней
    if (data_size > 0)
    {
        USBDev_DMAOpStart((uint8_t *)(USBDev_0.setup_pkt.xfer_buff + USBDev_0.setup_pkt.xfer_count),
                          data_size,
                          0,
                          USBDMA_OP_READ_FROM_EP | USBDMA_OP_START);

        USBDev_0.setup_pkt.xfer_count += data_size;
    }

    return 0;
}

//----- cep handler -------

void USBDev_ControlEPHandler(USBDev_TypeDef *usbdev)
{
    uint32_t irq_stat = USB->CEP_IRQ_STAT;
    irq_stat &= USB->CEP_IRQ_ENB;

    // SETUP packet was received
    if ((irq_stat & USB_CEP_IRQ_ENB_SETUPPKT_Msk) != 0)
    {
        if (USBDev_0.setup_pkt.stage == USBDev_SetupStage_Wait)
        {
            USB->CEP_IRQ_STAT = USB_CEP_IRQ_ENB_SETUPPKT_Msk;

            USBDev_ParseSetupPacket(usbdev);
            USBDev_SetupStage(usbdev);
        }
    }

    // DATA packet received
    if ((irq_stat & USB_CEP_IRQ_ENB_DATAPKTREC_Msk) != 0)
    {
        if (USBDev_0.setup_pkt.stage == USBDev_SetupStage_Data)
        {
            USB->CEP_IRQ_STAT = USB_CEP_IRQ_ENB_DATAPKTREC_Msk;
            USBDev_GetNextCEPPacket();
            if (USBDev_0.setup_pkt.xfer_count >= USBDev_0.setup_pkt.xfer_len)
            {
                // USBDevice.class_cb->EP0_RxReady();
                // send zerolen packet
                USBDev_CEPSendResponse(CEP_ZEROLEN);
                USBDev_SetSetupStage(USBDev_SetupStage_Status);
            }
        } /*else {
            USBDev_CEPSendResponse(CEP_STALL);
            USBDev_SetSetupStage(USBDev_SetupStage_Wait);
        }*/
    }

    // DATA packet transmitted
    if ((irq_stat & USB_CEP_IRQ_ENB_DATAPKTTR_Msk) != 0)
    {
        if (USBDev_0.setup_pkt.stage == USBDev_SetupStage_Data)
        {
            USB->CEP_IRQ_STAT = USB_CEP_IRQ_ENB_DATAPKTTR_Msk;
            if (USBDev_0.setup_pkt.xfer_count >= USBDev_0.setup_pkt.xfer_len)
            {
                // TODO: add callback TX data event
                USBDev_CEPSendResponse(CEP_ACK);
                USBDev_SetSetupStage(USBDev_SetupStage_Status);
            }
            else
            {
                // TODO:
                USBDev_PutNextCEPPacket();
            }
        } /*else {
            USBDev_CEPSendResponse(CEP_STALL);
            USBDev_SetSetupStage(USBDev_SetupStage_Wait);
        }*/
    }

    // end STATUS stage of SETUP xfer
    if ((irq_stat & USB_CEP_IRQ_ENB_STATCMPLN_Msk) != 0)
    {
        if (USBDev_0.setup_pkt.stage == USBDev_SetupStage_Status)
        {
            USB->CEP_IRQ_STAT = USB_CEP_IRQ_ENB_STATCMPLN_Msk;
            if (USBDev_0.setup_pkt.set_addr_active_flag)
            {
                // записываем новый адрес, выданный нам
                USBDev_SetAddress(USBDev_0.dev_addr);
                USBDev_0.setup_pkt.set_addr_active_flag = 0;
            }
            USBDev_SetSetupStage(USBDev_SetupStage_Wait);
        }
    }

    // usb ERROR
    if ((irq_stat & USB_CEP_IRQ_ENB_USBERR_Msk) != 0)
    {
        // discard data
        USBDev_SetSetupStage(USBDev_SetupStage_Wait);
        // USBDev_CEPSendResponse(CEP_STALL);
        USBDev_0.setup_pkt.xfer_count = 0;
        USBDev_0.setup_pkt.xfer_len = 0;
        USB->CEP_CTRL_STAT = USB_CEP_CTRL_STAT_CEPFLUSH_Msk | USB_CEP_CTRL_STAT_STALL_Msk;
        // and set for ready receive new SETUP packet
        // TODO:
        USB->CEP_IRQ_STAT = USB_CEP_IRQ_ENB_USBERR_Msk;
    }

    if ((irq_stat & USB_CEP_IRQ_ENB_SETUPTOKEN_Msk) != 0)
    {
        // TODO:
        USB->CEP_IRQ_STAT = USB_CEP_IRQ_ENB_SETUPTOKEN_Msk;
    }

    if ((irq_stat & USB_CEP_IRQ_ENB_INTOKEN_Msk) != 0)
    {
        // TODO:
        USB->CEP_IRQ_STAT = USB_CEP_IRQ_ENB_INTOKEN_Msk;
    }

    if ((irq_stat & USB_CEP_IRQ_ENB_OUTTOKEN_Msk) != 0)
    {
        // TODO:
        USB->CEP_IRQ_STAT = USB_CEP_IRQ_ENB_OUTTOKEN_Msk;
    }
}

//-- IRQ INTERRUPT HANDLERS ---------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//                          USER INTERRUPT HANDLERS
//-----------------------------------------------------------------------------------------

volatile uint32_t usb_framecnt;

void USBDev_Ep_Handler(uint32_t ep_idx)
{
    // check direction of EP
    if (USB->USB_EP[ep_idx].USB_EP_CFG_bit.EP_DIR != USB_USB_EP_USB_EP_CFG_EP_DIR_OUT)
    {
        // endpoint is IN
        if (USB->USB_EP[ep_idx].IRQ_STAT_bit.DATAPKTTRINT != 0)
        {
            // clear interrupt flag
            USB->USB_EP[ep_idx].IRQ_STAT = USB_USB_EP_IRQ_STAT_DATAPKTTRINT_Msk;
            // clear flag for updated hid report
            hid_report_updated = 0;
        }

        if (USB->USB_EP[ep_idx].IRQ_STAT_bit.ERRSENTINT != 0)
        {
            USB->USB_EP[ep_idx].IRQ_STAT = USB_USB_EP_IRQ_STAT_ERRSENTINT_Msk;

            while (1)
            {
            }; // TRAP on error!
        }
    }
}

void USBIntHandler(void)
{

    uint32_t irq_stat = USB->INTSTAT0;
    irq_stat &= USB->INTEN0;
    // GPIOB->DATAOUTSET = 0x02;

    GPIO_DBG(0x01);

    if ((irq_stat & USB_INTEN0_CEP_INTEN_Msk) != 0)
    {
        /* Прерывание Control EndPoint */
        GPIO_DBG(0x09);

        GPIOB->DATAOUTSET = 0x02;
        USBDev_ControlEPHandler(&USBDev_0);
        GPIOB->DATAOUTCLR = 0x02;
        // Handle CEP irqs
        irq_stat ^= USB_INTEN0_CEP_INTEN_Msk; // clear bit
    }

    if ((irq_stat & 0x3c) != 0)
    {
        // get irqs from endpoints
        /* Прерывание EndPoint */
        // GPIOB->DATAOUTSET = 0x04;
        // GPIOB->DATAOUTCLR = 0x04;

#ifdef USE_EP4
        if (irq_stat & USB_INTSTAT0_EP3_INT_Msk)
        {
            // clear interrupt status of endpoint
            USB->INTSTAT0 = USB_INTSTAT0_EP3_INT_Msk;
            // call EP handler
            // GPIOB->DATAOUTSET = 0x04;
            USBDev_Ep_Handler(0x3);
            // GPIOB->DATAOUTCLR = 0x04;

            irq_stat ^= USB_INTSTAT0_EP3_INT_Msk;
        }
#endif
#ifdef USE_EP1
        if (irq_stat & USB_INTSTAT0_EP0_INT_Msk)
        {
            // clear interrupt status of endpoint
            USB->INTSTAT0 = USB_INTSTAT0_EP0_INT_Msk;
            // call EP handler
            // GPIOB->DATAOUTSET = 0x04;
            USBDev_Ep_Handler(0x0);
            // GPIOB->DATAOUTCLR = 0x04;

            irq_stat ^= USB_INTSTAT0_EP0_INT_Msk;
        }
#endif
    }

    if ((irq_stat & USB_INTSTAT0_USBBUSINT_Msk) != 0)
    {
        uint32_t usbirq_stat = USB->INTSTAT1;
        usbirq_stat &= USB->INTEN1;

        if ((usbirq_stat & USB_INTSTAT1_CLKUNSTBL_Msk) != 0)
        {
            USB->INTSTAT1 = USB_INTSTAT1_CLKUNSTBL_Msk; // clear bit
        }
        /* Прерывание SetupOfFrame*/
        if ((usbirq_stat & USB_INTSTAT1_SOF_Msk) != 0)
        {
            usb_framecnt = USB->FRAMECNT_bit.FRAME_COUNTER;
            USB->INTSTAT1 = USB_INTSTAT1_SOF_Msk; // clear bit
        }

        if ((usbirq_stat & USB_INTSTAT1_RESSTATUS_Msk) != 0)
        {
            // configure control endpoint here for setup transactions
            USB->CEP_START_ADDR = 0;
            USB->CEP_END_ADDR = 64 - 1; // TODO: set as max control endpoint buffer value
            // enable interrupts for CEP
            USB->CEP_IRQ_ENB_bit.SETUPPKT = 1;   // enable interrupt on setup packet
            USB->CEP_IRQ_ENB_bit.DATAPKTREC = 1; // data packet received interrupt
            USB->CEP_IRQ_ENB_bit.DATAPKTTR = 1;  // data packet transmit interrupt
            USB->CEP_IRQ_ENB_bit.STATCMPLN = 1;  // status completion interrupt
            USB->CEP_IRQ_ENB_bit.USBERR = 1;     // error in control endpoint
            USB->CEP_IRQ_ENB_bit.SETUPTOKEN = 1;
            // USB->CEP_IRQ_ENB_bit.INTOKEN = 1;
            // USB->CEP_IRQ_ENB_bit.OUTTOKEN = 1;
            //
            USB->INTEN0_bit.CEP_INTEN = 1; // enable interrupts for control endpoint

            USB->INTSTAT1 = USB_INTSTAT1_RESSTATUS_Msk; // clear bit
            USB->INTEN1_bit.SOF = 1;

            USB->INTEN0 &= 0x3;
            USB->USB_EP[0].USB_EP_CFG = 0;
            USB->USB_EP[1].USB_EP_CFG = 0;
            USB->USB_EP[2].USB_EP_CFG = 0;
            USB->USB_EP[3].USB_EP_CFG = 0;

            hid_report_updated = 0;

            USBDev_ChangeState(&USBDev_0, Default);
            USBDev_0.setup_pkt.stage = USBDev_SetupStage_Wait;
        }

        if ((usbirq_stat & USB_INTSTAT1_RESUME_Msk) != 0)
        {
            USBDev_RevertState(&USBDev_0);

            USB->INTSTAT1 = USB_INTSTAT1_RESUME_Msk; // clear bit
        }

        if ((usbirq_stat & USB_INTSTAT1_SUSPEND_Msk) != 0)
        {

            USBDev_ChangeState(&USBDev_0, Suspended);

            USB->INTSTAT1 = USB_INTSTAT1_SUSPEND_Msk; // clear bit
        }
        /*
                if((usbirq_stat & USB_INTSTAT1_DMACMPL_Msk) != 0) {
                    USB->INTSTAT1 = USB_INTSTAT1_DMACMPL_Msk; //clear bit
                }*/

        if ((usbirq_stat & USB_INTSTAT1_CLKUNSTBL_Msk) != 0)
        {

            USB->INTSTAT1 = USB_INTSTAT1_CLKUNSTBL_Msk; // clear bit
            // USB->INTEN1_bit.CLKUNSTBL = 0; //disable this irq
        }

        // Handle USB bus irqs
        irq_stat ^= USB_INTSTAT0_USBBUSINT_Msk; // clear bit
    }
}

uint32_t HID_SendReport(uint8_t *hid_report)
{
    // static uint8_t btns = 0;
    if (USBDev_0.curr_state == Configured)
    {
        if (hid_report_updated == 0)
        {
            // set flag for control writing buffer
            hid_report_updated = 1;

#ifdef USE_EP4
            USBDev_WritePacket(hid_report, 3, 0x4);
#endif
#ifdef USE_EP1
            USBDev_WritePacket(hid_report, 3, 0x1);
#endif
        }
    }

    return 0;
}
