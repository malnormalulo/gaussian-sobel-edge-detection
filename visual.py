import serial
import numpy as np
import cv2
import argparse
import struct

def main():
    parser = argparse.ArgumentParser(description='OV7675 Binary Serial Viewer')
    parser.add_argument('--port', type=str, required=True, help='Serial port (e.g., COM3 or /dev/ttyACM0)')
    parser.add_argument('--baud', type=int, default=1000000, help='Baud rate (default: 1Mbit)')
    args = parser.parse_args()

    # Configure the serial port
    try:
        ser = serial.Serial(args.port, args.baud, timeout=2)
    except Exception as e:
        print(f"Error opening port: {e}")
        return

    print(f"Connected to {args.port}. Press 'q' in the video window to exit.")

    # Define the sync word we expect: 0xF81F F81F
    SYNC_WORD = b'\xf8\x1f\xf8\x1f'

    while True:
        # Step 1: Send the trigger command (decimal 49)
        ser.write(b'1') # ASCII '1' is decimal 49

        # Step 2: Look for the sync word in the stream
        # This prevents "drifting" if a byte is dropped
        header_start = ser.read(4)
        if header_start != SYNC_WORD:
            # If we lose sync, we flush and try again
            ser.reset_input_buffer()
            continue

        # Step 3: Read the uint32_t length (Bytes 4-7)
        length_bytes = ser.read(4)
        if len(length_bytes) < 4:
            continue

        # Unpack as Little Endian uint32
        frame_size = struct.unpack('<I', length_bytes)[0]

        # Step 4: Read the actual image data
        raw_data = ser.read(frame_size)
        if len(raw_data) < frame_size:
            print(f"Incomplete frame: got {len(raw_data)} of {frame_size}")
            continue

        # Step 5: Process RGB565 Data
        # Interpret as Big Endian uint16 for the pixel bits
        data = np.frombuffer(raw_data, dtype='<u2')

        # Determine image shape depending on the frame_size
        if frame_size == (320*240*2):
            frame = data[:320*240].reshape((240, 320))
        elif frame_size == (640*480*2):
            frame = data[:640*480].reshape((480, 640))
        else:
            raise ValueError(f"Frame size {frame_size} does not correspond to QVGA or VGA shape")

        # Unpack RGB565 Bits
        r = ((frame & 0xF800) >> 11) << 3
        g = ((frame & 0x07E0) >> 5) << 2
        b = (frame & 0x001F) << 3

        # Stack into BGR for OpenCV
        img = cv2.merge([b.astype(np.uint8), g.astype(np.uint8), r.astype(np.uint8)])

        # Display the result
        cv2.imshow('OV7675 Live Feed', img)


        # Standard OpenCV exit handler
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    ser.close()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
