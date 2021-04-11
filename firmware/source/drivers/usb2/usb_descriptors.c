/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Peter Lawrence
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*- Includes ----------------------------------------------------------------*/
#include <stdalign.h>
#include "usb_descriptors.h"

//#include "cdchelper.h"
#ifdef __cplusplus
extern "C" {
#endif

#define CDC_ITF_COMMAND 0x00
#define CDC_ITF_DATA    0x01
#define CDC_EP_COMMAND  0x81
#define CDC_EP_DATAOUT  0x02
#define CDC_EP_DATAIN   0x83

/*- Variables ---------------------------------------------------------------*/
const ALIGN(4) usb_device_descriptor_t usb_device_descriptor =
{
  .bLength            = sizeof(usb_device_descriptor_t),
  .bDescriptorType    = USB_DEVICE_DESCRIPTOR,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0xE0,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = USB_FS_MAX_PACKET_SIZE,
  .idVendor           = 0x6666,
  .idProduct          = 0x8889,
  .bcdDevice          = 0x0100,
  .iManufacturer      = USB_STR_MANUFACTURER,
  .iProduct           = USB_STR_PRODUCT,
  .iSerialNumber      = USB_STR_SERIAL_NUMBER,
  .bNumConfigurations = 1,
};

const ALIGN(4) usb_configuration_hierarchy_t usb_configuration_hierarchy =
{
  .configuration =
  {
    .bLength             = sizeof(usb_configuration_descriptor_t),
    .bDescriptorType     = USB_CONFIGURATION_DESCRIPTOR,
    .wTotalLength        = sizeof(usb_configuration_hierarchy_t),
    .bNumInterfaces      = 2,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = 0x80,
    .bMaxPower           = 50, // 100 mA
  },

  CDC_DESCRIPTOR(CDC_ITF_COMMAND, CDC_ITF_DATA, CDC_EP_COMMAND, CDC_EP_DATAOUT, CDC_EP_DATAIN)
  //RNDIS_DESCRIPTOR(/* Command ITF */ 0x00, /* Data ITF */ 0x01, /* Command EP */ USB_IN_ENDPOINT | USB_RNDIS_EP_COMM, /* DataOut EP */ USB_OUT_ENDPOINT | USB_RNDIS_EP_RECV, /* DataIn EP */ USB_IN_ENDPOINT | USB_RNDIS_EP_SEND)
};

const ALIGN(4) usb_string_descriptor_zero_t usb_string_descriptor_zero =
{
  .bLength               = sizeof(usb_string_descriptor_zero_t),
  .bDescriptorType       = USB_STRING_DESCRIPTOR,
  .wLANGID               = 0x0409, // English (United States)
};

char usb_serial_number[16];

const char *const usb_strings[] =
{
  [USB_STR_MANUFACTURER]  = "Acme",
  [USB_STR_PRODUCT]       = "RNDIS",
  [USB_STR_SERIAL_NUMBER] = usb_serial_number,
};

#ifdef __cplusplus
}
#endif
