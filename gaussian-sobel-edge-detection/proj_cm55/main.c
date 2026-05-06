#include "cybsp.h"
#include "cy_pdl.h"

#include <stdlib.h>
#include <string.h>

// Access to retarget-io initialization
// used to have printf over KitProg3
#include "retarget_io_init.h"

// Driver for USBD support (using emUSB from Seeger)
#include "driver/usbd/usbd.h"

// Core logic
#include "core.h"

/**
 * @def COM_OVERHEAD
 * Size of the header packet (USB communication)
 * Used for synchronization and validation
 */
#define COM_OVERHEAD	8

/**
 * @def COM_CMD_SIZE
 * Size of a command from computer to PSoC Edge
 */
#define COM_CMD_SIZE	1

/**
 * @def COM_CMD_START_STREAM
 * Command enabling to start the streaming of data
 */
#define COM_CMD_START_STREAM	49

static uint8_t* image_buffer_0 = NULL;
static uint8_t* image_buffer_1 = NULL;
static cy_stc_scb_i2c_context_t i2c_master_context;
static bool frame_ready = false;
static bool active_frame = false;

int camera_init(void)
{
    cy_rslt_t result;

    /* allocated image buffers */
    image_buffer_0 = malloc(OV7675_MEMORY_BUFFER_SIZE);
    if (image_buffer_0 == NULL) {
        printf("Failed to malloc image buffer 0\n");
        return -1;
    }

    image_buffer_1 = malloc(OV7675_MEMORY_BUFFER_SIZE);
    if (image_buffer_1 == NULL) {
        printf("Failed to malloc image buffer 1\n");
        return -1;
    }

    /* Enable I2C Controller */
    result = Cy_SCB_I2C_Init(CYBSP_I2C_CAM_CONTROLLER_HW, &CYBSP_I2C_CAM_CONTROLLER_config, &i2c_master_context);
    if (CY_SCB_I2C_SUCCESS != result) {
        printf("I2C init failed\n");
        return -1;
    }

    Cy_SCB_I2C_Enable(CYBSP_I2C_CAM_CONTROLLER_HW);

    /* Initialize the camera DVP OV7675 */
    result = mtb_dvp_cam_ov7675_init(image_buffer_0, image_buffer_1, &i2c_master_context,
                                     &frame_ready, &active_frame);
    if (CY_RSLT_SUCCESS != result) {
        printf("DVP camera init failed\n");
        return -1;
    }

    return 0;
}

int main(void)
{
	uint8_t* comm_buffer = NULL;

	usbd_t* usb_handle;

	int send_data = 0;

    cy_rslt_t result;

    // Initialize the device and board peripherals
    result = cybsp_init();
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    // Init retarget-io -> printf redirected to KitProg3
	init_retarget_io();

	// Filling gaussian kernel - only one time
    // fill_gaussian_blur_kernel();

    // Enable global interrupts
    __enable_irq();

    if (camera_init() != 0) {
        CY_ASSERT(0);
    }

	printf("PSOC EDGE OV7675 Streaming over USB v1.0\r\n");

	// Allocate communication buffer
	// We assume that the size of a picture captured by the OV7675
	// is bigger than the size of a radar frame
	comm_buffer = malloc(OV7675_MEMORY_BUFFER_SIZE + COM_OVERHEAD);
	if (comm_buffer == NULL)
	{
		printf("Cannot allocate comm_buffer \r\n");
		return 0;
	}

	// Init USB CDC
	// This call will block until USB cable is plugged to a computer
	usb_handle = usbd_create();

    for (;;)
    {
    	uint8_t cmd = 0;

    	// Something in USB read buffer?
    	if ( usbd_read(usb_handle, &cmd, COM_CMD_SIZE) == COM_CMD_SIZE)
    	{
    		printf("Received command: %d \r\n", cmd);
    		if (cmd == COM_CMD_START_STREAM)
    		{
    			send_data = 1;
    		}
    		else send_data = 0;
    	}

    	// Frame ready from the OV7675?
		if (frame_ready)
		{
            uint8_t *buf = active_frame ? image_buffer_1 : image_buffer_0;
			
			convert_rgb565_to_mono_rgb888(SIZE, buf, out_monochrome);
			// gaussian_blur(HEIGHT, WIDTH, out_monochrome, out_gaussian_blur);
			// sobel_edge_detection(HEIGHT, WIDTH, out_gaussian_blur, out_sobel);
			// mono_rgb888_to_rgb565(SIZE, out_sobel, &comm_buffer[COM_OVERHEAD]);
			mono_rgb888_to_rgb565(SIZE, out_monochrome, &comm_buffer[COM_OVERHEAD]);

			frame_ready = false;

			// Add overhead
			comm_buffer[0] = 0xf8;
			comm_buffer[1] = 0x1f;
			comm_buffer[2] = 0xf8;
			comm_buffer[3] = 0x1f;
			*((uint32_t*)&comm_buffer[4]) = OV7675_MEMORY_BUFFER_SIZE;

			// Send per USB
			if (send_data == 1)
			{
				if (usbd_write(usb_handle, comm_buffer, OV7675_MEMORY_BUFFER_SIZE + COM_OVERHEAD) != 0)
				{
					printf("Failed to write OV7675 values over USB\r\n");
					send_data = 0;
				}
			}
		}
    }
}