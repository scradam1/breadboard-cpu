import serial
import time

PORT = '/dev/ttyACM0'
BAUD = 115200
BUFFER_SIZE = 256

def serial_program():
    """ Send binary data from file to Arduino through USB port """

    # Open binary file with ROM image
    with open('rom_signed.bin', 'rb') as f:
        data = f.read()

    print("Uploading %d bytes at %d Ki/s in %d-byte packets..." %(len(data), BAUD/1000, BUFFER_SIZE))

    # Initialize serial connection and wait for Arduino to initialize
    ser = serial.Serial(PORT, BAUD)
    time.sleep(5)

    # Loop through ROM image
    base = 0;
    while base < len(data) - BUFFER_SIZE:
        page = data[base:base + BUFFER_SIZE] # Get next page from ROM image

        # Only send packet if Arduino is ready
        if ser.in_waiting > 0:
            if ser.read(1) == b'K':
                ser.write(b'P') # 'Program' signal to Arduino
                ser.write(page) # Write data to serial bus
                base += BUFFER_SIZE

    ser.close()
    print("done.")

serial_program()
