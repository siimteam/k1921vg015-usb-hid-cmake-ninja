/*
 * usbhid.h
 *
 *  Created on: 5 сент. 2024 г.
 *      Author: dav
 */

#ifndef USBHID_H_
#define USBHID_H_

#include <K1921VG015.h>
#include <stdio.h>
#include <string.h>
#include <system_k1921vg015.h>

#define GPIO_DBG(X)

#define USB_REQ_GET_STATUS 0x00
#define USB_REQ_CLEAR_FEATURE 0x01
#define USB_REQ_SET_FEATURE 0x03
#define USB_REQ_SET_ADDRESS 0x05
#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_REQ_SET_DESCRIPTOR 0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09
#define USB_REQ_GET_INTERFACE 0x0A
#define USB_REQ_SET_INTERFACE 0x0B
#define USB_REQ_SYNCH_FRAME 0x0C

#define USB_DESC_TYPE_DEVICE 1
#define USB_DESC_TYPE_CONFIGURATION 2
#define USB_DESC_TYPE_STRING 3
#define USB_DESC_TYPE_INTERFACE 4
#define USB_DESC_TYPE_ENDPOINT 5
#define USB_DESC_TYPE_DEVICE_QUALIFIER 6
#define USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION 7
#define USB_DESC_TYPE_OTG 9

#define USB_BCD 0x0200
#define USB_DEVICE_MAX_EP0_SIZE 64
#define USB_DEVICE_VENDOR_ID 0x0483
#define USB_DEVICE_PRODUCT_ID 0x5711
#define USB_BCD_DEVICE 0x0001
#define USB_DEVICE_IDX_MFC_STR 0x01
#define USB_DEVICE_IDX_PRODUCT_STR 0x02
#define USB_DEVICE_IDX_SERIAL_STR 0x03

/* USB HID */
#define HID_DESCRIPTOR_TYPE 0x21
#define HID_CLASS_SPEC_RELEASE_NUMBER 0x0111

#define HID_REPORT_DESCRIPTOR 0x22
#define HID_IN_PACKET 4

#define HID_REQ_SET_PROTOCOL 0x0B
#define HID_REQ_GET_PROTOCOL 0x03
#define HID_REQ_SET_IDLE 0x0A
#define HID_REQ_GET_IDLE 0x02

#define HID_REQ_GET_REPORT 0x01

#define USB_DEVICE_CFG_MAX_NUM 1

#define USBD_IDX_LANGID_STR 0x00
#define USBD_IDX_MFC_STR 0x01
#define USBD_IDX_PRODUCT_STR 0x02
#define USBD_IDX_SERIAL_STR 0x03
#define USBD_IDX_CONFIG_STR 0x04
#define USBD_IDX_INTERFACE_STR 0x05

#define USB_REQ_RECIPIENT_DEVICE 0x00
#define USB_REQ_RECIPIENT_INTERFACE 0x01
#define USB_REQ_RECIPIENT_ENDPOINT 0x02

#define LOBYTE(x) ((unsigned char)(x & 0x00FF))
#define HIBYTE(x) ((unsigned char)((x & 0xFF00) >> 8))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define GPIO_C0_TOGLE         \
    GPIOC->DATAOUTSET = 0x01; \
    __asm("NOP");             \
    __asm("NOP");             \
    __asm("NOP");             \
    __asm("NOP");             \
    GPIOC->DATAOUTCLR = 0x01;

#define EP_IN 0x80
#define EP_OUT 0x00

#define EP_DATA_IN (EP_IN | 0x1)
#define EP_DATA_OUT (EP_OUT | 0x2)
#define EP_STATUS_IN (EP_IN | 0x3)

#define EP_CONTROL 0x00
#define EP_ISO 0x01
#define EP_BULK 0x02
#define EP_INTERRUPT 0x03

#define SET_LINE_CODING 0x20
#define GET_LINE_CODING 0x21
#define SET_CONTROL_LINE_STATE 0x22

#define UBUFF_MAX 1024

#define USE_EP4

typedef enum
{
    CEP_ACK = 0,
    CEP_NAK = 1,
    CEP_STALL = 2,
    CEP_ZEROLEN = 4
} CEP_RESPONSE;

typedef enum
{
    USBDev_SetupStage_Wait = 0,
    USBDev_SetupStage_Data,
    USBDev_SetupStage_Status
} USBDev_SetupStage_TypeDef;

typedef enum
{
    Attached = 0,
    Powered,
    Default,
    Addressed,
    Configured,
    Suspended
} USBDev_State_TypeDef;

typedef struct
{
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;

    uint8_t *xfer_buff;
    uint32_t xfer_len;
    uint32_t xfer_count;
    uint8_t set_addr_active_flag; // TODO
    USBDev_SetupStage_TypeDef stage;
} USBDev_Setup_Pkt;

typedef struct
{
    uint8_t dev_addr;
    USBDev_Setup_Pkt setup_pkt;
    __IO USBDev_State_TypeDef curr_state;
    __IO USBDev_State_TypeDef prev_state; // for suspend/resume operations

    uint8_t config;
} USBDev_TypeDef;

typedef struct __attribute__((__packed__))
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bscUSB[2];
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint8_t idVendor[2];
    uint8_t idProduct[2];
    uint8_t bcdDevice[2];
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} DEVICE_DSCR_TYPE;

// Configuration descriptor
typedef struct __attribute__((__packed__))
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t wTotalLength[2];
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttibutes;
    uint8_t MaxPower;
} CONFIGURATION_DSCR_TYPE;

// Interface Descriptor
typedef struct __attribute__((__packed__))
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} INTERFACE_DSCR_TYPE;

// Endpoint Descriptor
typedef struct __attribute__((__packed__))
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} ENDPOINT_DSCR_TYPE;

#define USB_DEVICE_LANGID_STR 0x0409

#define USB_DEVICE_MFC_STR "NIIET K1921VG015"
#define USB_DEVICE_PRODUCT_STR "NIIET USB HID device"
#define USB_DEVICE_SERIAL_STR "000000001S"
#define USB_DEVICE_CONFIG_STR "Joystick"
#define USB_DEVICE_INTERFACE_STR "USB"

#define USBDMA_OP_READ_FROM_EP (0 << 4)
#define USBDMA_OP_WRITE_TO_EP (1 << 4)
#define USBDMA_OP_START (1 << 5)
#define USBDMA_OP_MASK_DONE (1 << 5)

/* USB func prototype */
void USBPLL_init();
void USBDev_Init(USBDev_TypeDef *usbdev);
void USB_init();
void USBIntHandler(void);
uint32_t HID_SendReport(uint8_t *hid_report);

#endif /* USBHID_H_ */
