import array

# Hexadecimal patterns for 7-segment display digits 0-9
DIGITS = [0x3f, 0x6, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x7, 0x7f, 0x6f]

ADDRESSES = 16 # Number of address lines
ROM_SIZE = 2**ADDRESSES # Size of address space
DISPLAY_DIGITS = 5 # Number of digits used, excluding sign digit
SIGN_START = END_ADDR = ROM_SIZE*DISPLAY_DIGITS
SIGN_END = SIGN_START + ROM_SIZE # Addresses for sign "digit"

# Unused states
DECODER_SELECTS = 3
UNUSED_STATES = 2**DECODER_SELECTS - (DISPLAY_DIGITS + 1)
FILL = ROM_SIZE*UNUSED_STATES

def generate_ROM():
    """ Generate binary ROM images for unsigned and signed
        numbers in 7-segment numerical display """

    file_unsigned = open('rom_unsigned.bin', 'wb')
    file_signed = open('rom_signed.bin', 'wb')

    # Generate outputs for unsigned numbers
    for i, index in enumerate(range(0, END_ADDR, ROM_SIZE)):
        print(f"{10**i}s place, from {index:,} to {index+ROM_SIZE-1:,}\n")
        for x in range(ROM_SIZE):
            file_unsigned.write(array.array("B", [ (DIGITS[x % 10] if i==0 else DIGITS[int(x / (10**i)) % 10]) ] ))

    # Fill in "sign" place with zeros for unsigned numbers
    print(f"Unsigned signs place, from {SIGN_START:,} to {SIGN_END-1:,}\n")
    for x in range(ROM_SIZE):
        file_unsigned.write(array.array("B", [0x0]))

    # Generate outputs for signed numbers
    for i, index in enumerate(range(0, END_ADDR, ROM_SIZE)):
        print(f"Complements {10**i}s place, from {index:,} to {index+ROM_SIZE-1:,}\n")
        for x in range(-(int(ROM_SIZE/2)), (int(ROM_SIZE/2))):
            file_signed.write(array.array("B", [ (DIGITS[abs(int(x/(10**i))) % 10]) ] ))

    # Generate sign outputs for signed numbers
    print(f"Complements signs place, from {SIGN_START:,} to {SIGN_END-1:,}\n")
    for x in range(-(int(ROM_SIZE/2)), (int(ROM_SIZE/2))):
        if x < 0:
            file_signed.write(array.array("B", [0x40])) # Negative sign for negative numbers
        else:
            file_signed.write(array.array("B", [0x0])) # Blank for nonnegative numbers

    # Fill in remainders of ROMs with zeros
    print(f"Filling {SIGN_END:,} to {SIGN_END+FILL-1:,} (unused addresses) with zeros\n")
    for x in range(FILL):
        file_unsigned.write(array.array("B", [0x0]))
        file_signed.write(array.array("B", [0x0]))

    file_unsigned.close()
    file_signed.close()

generate_ROM()
