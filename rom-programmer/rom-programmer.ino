// FLASH pin definitions
#define FLASH_CE 2
#define FLASH_OE 3
#define FLASH_WE 4
#define FLASH_A0 30
#define FLASH_A18 48
#define FLASH_D0 22
#define FLASH_D7 29

const long FLASH_SIZE = 524288; // 2**19

// Presets for write operations on the SST39SF040 FLASH chip
const int PGM_CYCLES = 3;
const int PGM_CYCLE_ADDR[] = {0x5555, 0x2AAA, 0x5555};
const byte PGM_CYCLE_DATA[] = {0xAA, 0x55, 0xA0};
const int ERASE_CYCLES = 6;
const int ERASE_CYCLE_ADDR[] = {0x5555, 0x2AAA, 0x5555, 0x5555, 0x2AAA, 0x5555};
const byte ERASE_CYCLE_DATA[] = {0xAA, 0x55, 0x80, 0xAA, 0x55, 0x10};

// Presets for serial communication
const long BAUD = 115200;
const int BUFFER_SIZE = 256;

/**
 * Set address pins on FLASH chip
 * 
 * @param address Address value expressed as a long integer
 */
void set_address(long address) {
  
  // Prepare FLASH address pins with addresses
  for (int pin = FLASH_A0; pin <= FLASH_A18; pin++) {
    digitalWrite(pin, address & 1); // Write LSB of address
    address = address >> 1; // Shift address bitwise to get next bit
  }
}

/**
 * Read single byte of FLASH memory
 * 
 * @param address Address from which to get data
 * @return Returns the byte of data at the given address
 */
byte read_byte(long address) {
  
  set_address(address); // Set address pins

  // Read data bitwise from FLASH
  byte data = 0;
  for (int pin = FLASH_D7; pin >= FLASH_D0; pin--) {
    data = (data << 1) + digitalRead(pin); // Assemble byte from bitwise-shifted bits
  }

  return data;
}

/**
 * Reads specified section of FLASH memory
 * 
 * @param start Starting address to read
 * @param size Length of data to read
 */
void read_FLASH(long start = 0, long size = 1024) {
  
  // Set FLASH data pins as inputs
  for (int pin = FLASH_D0; pin <= FLASH_D7; pin++) {
    pinMode(pin, INPUT);
  }

  digitalWrite(FLASH_CE, LOW); // Enable chip
  digitalWrite(FLASH_OE, LOW); // Enable output
  
  Serial.println("Reading FLASH...");
  
  // Read and print FLASH contents bytewise
  for (long base = start; base < (start + size - 16); base += 16) {
    byte data[16]; // Buffer to hold data
    digitalWrite(DEBUG, HIGH);
    // Read one block of data (16 addresses)
    for (int offset = 0; offset < 16; offset++) {
      data[offset] = read_byte(base + offset);
    }
    
    // Format data into string for output
    char buf[80];
    sprintf(buf, "%05lx: %02x %02x %02x %02x %02x %02x %02x %02x\t%02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf); // Send formatted data to serial monitor
  }
  
  digitalWrite(FLASH_OE, HIGH); // Disable output
  digitalWrite(FLASH_CE, HIGH); // Disable chip
  Serial.println("done.");
  
}

/**
 * Write one byte of data to the FLASH chip
 * 
 * @param address Address where the data should be written
 * @param data Single-byte data to be written
 */
void write_byte(long address, byte data) {
  
  // Set address for write operation
  set_address(address);

  // Prepare FLASH data pins with data
  for (int pin = FLASH_D0; pin <= FLASH_D7; pin++) {
    digitalWrite(pin, data & 1); // Write LSB of data
    data = data >> 1; // Bitwise shift data to next bit
  }

  digitalWrite(FLASH_CE, LOW); // Enable chip
  digitalWrite(FLASH_WE, LOW); // Enable write operation (latches address)
  
  digitalWrite(FLASH_WE, HIGH); // Disable write operation (latches data)
  digitalWrite(FLASH_CE, HIGH); // Disable chip
}

/**
 * Write an array of data to the FLASH chip
 * 
 * @param data[] Array containing bytes of data to be written
 * @param size Size in bytes of the data array
 * @param address_offset Address where the first element of data should be written
 */
void write_FLASH(byte data[], size_t size, long address_offset = 0) {
  
  digitalWrite(FLASH_OE, HIGH); // Disable output

  // Configure FLASH data pins as outputs
  for (int pin = FLASH_D0; pin <= FLASH_D7; pin++) {
    pinMode(pin, OUTPUT);
  }
  
  for (long address = address_offset; address < size + address_offset; address++) {
    // FLASH chip write command sequence
    for (int cycle = 0; cycle < PGM_CYCLES; cycle++) {
      write_byte(PGM_CYCLE_ADDR[cycle], PGM_CYCLE_DATA[cycle]);
    }

    // Write data
    write_byte(address, data[address - address_offset]);
    delayMicroseconds(20); // byte program time
  }
}

/**
 * Erase entire FLASH chip to 0xFF
 */
void erase_FLASH() {
  
  digitalWrite(FLASH_OE, HIGH); // Unset output_enable

  // Configure FLASH data pins as outputs
  for (int pin = FLASH_D0; pin <= FLASH_D7; pin++) {
    pinMode(pin, OUTPUT);
  }

  // FLASH chip erase sequence
  Serial.print("Erasing FLASH... ");
  for (int cycle = 0; cycle < ERASE_CYCLES; cycle++) {
    write_byte(ERASE_CYCLE_ADDR[cycle], ERASE_CYCLE_DATA[cycle]);
  }

  delay(100); // chip erase time
  Serial.println("done.");
}

/**
 * Listens for data stream over USB port and writes to FLASH chip
 */
void serial_program() {

  long address = 0;
  bool done = false;

  while (address < FLASH_SIZE - BUFFER_SIZE && !done) {
    Serial.write('K'); // Send acknowledgement to serial bus

    // Wait for response from Python script
    while (!Serial.available());
    byte cmd = Serial.read();
    done = (cmd == 'F'); // Break out of loop if receive 'Finished' signal
    
    if (cmd == 'P') { // 'Program' signal
      byte page[BUFFER_SIZE]; // Initialize data buffer
    
      // Read packet from serial bus
      size_t received = Serial.readBytes(page, BUFFER_SIZE);

      // If packet was lost, try to receive it again
      if (received != BUFFER_SIZE) {
        Serial.write('E');
        continue;
      }

      // Write packet to flash and send success acknowledgement to serial bus
      write_FLASH(page, BUFFER_SIZE, address);
      address += BUFFER_SIZE;
      Serial.write('D');
    }
  }
}

void setup() {
  
  // Set up pins for communication with FLASH
  pinMode(FLASH_CE, OUTPUT);
  pinMode(FLASH_OE, OUTPUT);
  pinMode(FLASH_WE, OUTPUT);
  for (int pin = FLASH_A0; pin <= FLASH_A18; pin++) {
    pinMode(pin, OUTPUT); // Address pins
  }

  // Disable all chip functions
  digitalWrite(FLASH_CE, HIGH);
  digitalWrite(FLASH_OE, HIGH);
  digitalWrite(FLASH_WE, HIGH);

  // Initialize serial communication
  Serial.begin(BAUD);
  Serial.setTimeout(1000);

  // Only uncomment ONE (1) of these functions at a time!
  // Erase entire chip
  //erase_FLASH();

  // Write data
  //serial_program();

  // Read chunk of data
  read_FLASH();
}

void loop() {}
