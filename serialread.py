 #!/usr/bin/env python
import time
import serial

serial = serial.Serial(
   port='/dev/ttyUSB0',
   baudrate = 9600,
   parity=serial.PARITY_NONE,
   stopbits=serial.STOPBITS_ONE,
   bytesize=serial.EIGHTBITS,
   timeout=1
)

while 1:
   print serial.readline()