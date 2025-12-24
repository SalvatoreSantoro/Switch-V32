# little script to use inside GDB to read and write basically
# a giant number of bytes to see if the stub is correctly reading and writing memory

import re
import gdb
import random

inferior = gdb.selected_inferior()

NUM_WRITES = 100000  # number of writes to perform
MAX_ADDR = 0xFFFFFFFF  # max address for example purposes
MAX_LEN = 16  # max number of bytes to write at once

# Track the last value of each byte written
byte_map = {}

# Random write phase
for _ in range(NUM_WRITES):
    addr = random.randint(0, MAX_ADDR)
    length = random.randint(1, MAX_LEN)
    data = bytearray(random.getrandbits(8) for _ in range(length))
    try:
        inferior.write_memory(addr, data, length)
        # Track last value for each byte
        for i in range(length):
            byte_map[addr + i] = data[i]
    except:
        continue


# Random read/assertion phase
addresses = list(byte_map.keys())
random.shuffle(addresses)

for addr in addresses:
	try:
		read_value = inferior.read_memory(addr, 1)[0]
		if bytearray(read_value.to_bytes()) != byte_map[addr]:
			print(f"Mismatch detected at address {hex(addr)}")
		break
	except:
		continue
