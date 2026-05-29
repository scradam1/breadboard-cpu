import serial
import time

PORT = '/dev/ttyACM0'
FILE_NAME = 'rom_signed.bin'
#FILE_NAME = 'rom_unsigned.bin'
BAUD = 115200
BUFFER_SIZE = 256

def serial_program():
    """ Send binary data from file to Arduino through USB port """

    # Open binary file with ROM image
    with open(FILE_NAME, 'rb') as f:
        data = f.read()

    print("Uploading %d bytes at %d Ki/s in %d-byte packets..." %(len(data), BAUD/1000, BUFFER_SIZE))

    # Initialize serial connection and wait for Arduino to initialize
    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(5)

    # Loop through ROM image
    base = 0;
    while base < len(data) - BUFFER_SIZE:
        page = data[base:base + BUFFER_SIZE] # Get next page from ROM image

        # Wait to send packet until Arduino acknowledges
        while (ser.read() != b'K'): continue

        ser.write(b'P') # 'Program' signal to Arduino
        ser.write(page) # Write data to serial bus

        # Check for response from Arduino
        result = ser.read(1)
        #print(result)

        if result == b'D': # Successful program cycle
            base += BUFFER_SIZE
        elif result == b'E': # Packet size mismatch; resend data
            continue
        else: # Other error
            raise RuntimeError("Programming failed")

    ser.close()
    print("done.")

serial_program()
