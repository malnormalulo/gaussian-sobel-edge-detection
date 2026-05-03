/******************************************************************************
* File Name:   usbd.c
*
* Description: This file contains functions for streaming data over a serial
*              USB CDC interface.
*
* Related Document: See README.md
*
*******************************************************************************
* (c) 2025-2025, Infineon Technologies AG, or an affiliate of Infineon Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is owned by
* Infineon Technologies AG or one of its affiliates ("Infineon") and is protected
* by and subject to worldwide patent protection, worldwide copyright laws, and
* international treaty provisions. Therefore, you may use this Software only as
* provided in the license agreement accompanying the software package from which
* you obtained this Software. If no license agreement applies, then any use,
* reproduction, modification, translation, or compilation of this Software is
* prohibited without the express written permission of Infineon.
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING,
* BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF THIRD-PARTY RIGHTS AND
* IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A SPECIFIC USE/PURPOSE OR
* MERCHANTABILITY. Infineon reserves the right to make changes to the Software
* without notice. You are responsible for properly designing, programming, and
* testing the functionality and safety of your intended application of the
* Software, as well as complying with any legal requirements related to its
* use. Infineon does not guarantee that the Software will be free from intrusion,
* data theft or loss, or other breaches ("Security Breaches"), and Infineon
* shall have no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any application
* where a failure of the Product or any consequences of the use thereof can
* reasonably be expected to result in personal injury.
*
* Rutronik Elektronische Bauelemente GmbH Disclaimer: The evaluation board
* including the software is for testing purposes only and,
* because it has limited functions and limited resilience, is not suitable
* for permanent use under real conditions. If the evaluation board is
* nevertheless used under real conditions, this is done at one’s responsibility;
* any liability of Rutronik is insofar excluded
*******************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <USB.h>
#include <USB_CDC.h>

#include "usbd.h"
#include "cybsp.h"

/*******************************************************************************
* Local Function Prototypes
*******************************************************************************/
static USB_CDC_HANDLE _usbd_add_cdc(void);

/*******************************************************************************
* Functions
*******************************************************************************/

/*******************************************************************************
* Function Name: _usbd_add_cdc
********************************************************************************
* Summary:
*  Initializes USB CDC.
*
*******************************************************************************/
static USB_CDC_HANDLE _usbd_add_cdc(void)
{
    static U8             OutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
    USB_CDC_INIT_DATA     InitData;
    USB_ADD_EP_INFO       EPBulkIn;
    USB_ADD_EP_INFO       EPBulkOut;
    USB_ADD_EP_INFO       EPIntIn;

    memset(&InitData, 0, sizeof(InitData));
    EPBulkIn.Flags          = 0;                             /* Flags not used */
    EPBulkIn.InDir          = USB_DIR_IN;                    /* IN direction (Device to Host) */
    EPBulkIn.Interval       = 0;                             /* Interval not used for Bulk endpoints */
    EPBulkIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   /* Maximum packet size (512B for Bulk in full-speed) */
    EPBulkIn.TransferType   = USB_TRANSFER_TYPE_BULK;        /* Endpoint type - Bulk */
    InitData.EPIn  = USBD_AddEPEx(&EPBulkIn, NULL, 0);

    EPBulkOut.Flags         = 0;                             /* Flags not used */
    EPBulkOut.InDir         = USB_DIR_OUT;                   /* OUT direction (Host to Device) */
    EPBulkOut.Interval      = 0;                             /* Interval not used for Bulk endpoints */
    EPBulkOut.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   /* Maximum packet size (512B for Bulk in full-speed) */
    EPBulkOut.TransferType  = USB_TRANSFER_TYPE_BULK;        /* Endpoint type - Bulk */
    InitData.EPOut = USBD_AddEPEx(&EPBulkOut, OutBuffer, sizeof(OutBuffer));

    EPIntIn.Flags           = 0;                             /* Flags not used */
    EPIntIn.InDir           = USB_DIR_IN;                    /* IN direction (Device to Host) */
    EPIntIn.Interval        = 64;                            /* Interval of 8 ms (64 * 125us) */
    EPIntIn.MaxPacketSize   = USB_HS_BULK_MAX_PACKET_SIZE ;   /* Maximum packet size (64 for Interrupt) */
    EPIntIn.TransferType    = USB_TRANSFER_TYPE_INT;         /* Endpoint type - Interrupt */
    InitData.EPInt = USBD_AddEPEx(&EPIntIn, NULL, 0);

    return USBD_CDC_Add(&InitData);
}

/*******************************************************************************
* Function Name: usbd_create
********************************************************************************
* Summary:
*   Creates and initializes a new streaming instance.
*
* Parameters:
*   protocol: Pointer to the protocol handle.
*
* Return:
*   Pointer to the newly created streaming instance.
*
*******************************************************************************/
usbd_t* usbd_create()
{
    usbd_t* usb = malloc(sizeof(usbd_t));

    if(usb == NULL)
        return NULL;

    static char serial_str[37];
    sprintf(serial_str, "Rutronik_20251208");
    usb->usb_deviceInfo.sSerialNumber = (const char *)serial_str;
    usb->usb_deviceInfo.VendorId = 0x058B;
    usb->usb_deviceInfo.ProductId = 0x027D;
    usb->usb_deviceInfo.sVendorName = "Infineon Technologies";
    usb->usb_deviceInfo.sProductName = "Rutronik USB Streaming";

    /* Initializes the USB stack */
    USBD_Init();

    /* Endpoint Initialization for CDC class */
    usb->usb_cdcHandle = _usbd_add_cdc();

    /* Set device info used in enumeration */
    USBD_SetDeviceInfo(&usb->usb_deviceInfo);

    /* Start the USB stack */
    USBD_Start();

    return usb;
}

/*******************************************************************************
* Function Name: usbd_free
********************************************************************************
* Summary:
*   Frees the allocated resources for the streaming instance.
*
* Parameters:
*   streaming: Pointer to the streaming instance.
*
*******************************************************************************/
void usbd_free(usbd_t* usb)
{
    /*TODO: Implement hardware shutdown logic if necessary*/
    free(usb);
}

int usbd_write(usbd_t* usb, uint8_t* buffer, size_t count)
{
	if ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED)
	{
		return -1;
	}

	int retval = USBD_CDC_Write(usb->usb_cdcHandle, buffer, count, 100);
    if ( retval != count)
    {
    	return -1;
    }

    /* Block until write is complete */
    USBD_CDC_WaitForTX(usb->usb_cdcHandle, 0);

    return 0;
}

int usbd_read(usbd_t* usb, uint8_t* buf, size_t count)
{
    if (count == 0)
        return true;

    return USBD_CDC_Read(usb->usb_cdcHandle,
    		buf,
			count,
                      1);
}

/* [] END OF FILE */
