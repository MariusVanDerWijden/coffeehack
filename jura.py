# Q42
# jura.py created by taco ekkel
# protocol source: http://protocol-jura.do.am/index/protocol_to_coffeemaker/0-7

import sys
import serial
import time
import struct
import binascii
from bitarray import bitarray

SERIAL_PORT = '/dev/serial0'
#could be /dev/serialAMA0

# Given an ascii character c, returns an array (size 4) of bitarray in Jura encoding
#
# 4 UART bytes are one real byte:
# MSB.....LSB
# UART byte 33221100
# bit 52525252 contain the information
def to_jura(c):
    z0 = bitarray(8)
    z0.setall(True)
    z1 = bitarray(8)
    z1.setall(True)
    z2 = bitarray(8)
    z2.setall(True)
    z3 = bitarray(8)
    z3.setall(True)
    out = bitarray()
    out.frombytes(c)

    z0[2] = out[0]
    z0[5] = out[1]
    z1[2] = out[2]
    z1[5] = out[3]
    z2[2] = out[4]
    z2[5] = out[5]
    z3[2] = out[6]
    z3[5] = out[7]

    return [z0,z1,z2,z3]


# given an array of 4 bitarrays in jura encoding, return the decoded character byte
def from_jura(bytes):
    if len(bytes) != 4: raise ValueError('Needs an array of size 4')
    c = bitarray(8)

    c[0] = bytes[0][2]
    c[1] = bytes[0][5]
    c[2] = bytes[1][2]
    c[3] = bytes[1][5]
    c[4] = bytes[2][2]
    c[5] = bytes[2][5]
    c[6] = bytes[3][2]
    c[7] = bytes[3][5]

    return c.tobytes()


def test_encoder():
    teststring = b'AN:01' # enforce ascii
    bytes = []
    for c in teststring:
        bytes.append(to_jura(c))
    chars = ""
    for arr in bytes:
        chars += from_jura(arr)
    assert chars == teststring, "encoding/decoding functions must be symmetrical"


# encodes and writes a command without CRLF to the coffee machine
def send_command(cmd):
    print 'send_command' + cmd
    cmd += "\r\n"
    for c in cmd:
        bytes = to_jura(c)
        for b in bytes:
            serialport.write(b.tobytes())
            #time.sleep(8 / 1000) # 8ms break between bytegroups
    return


# receives a response of any size into a bytearray
def receive_response():
    print 'receive response'
    response = bytearray()
    bytes = bytearray(4)
    while serialport.in_waiting > 0:
        print 'read bytes'
        serialport.readinto(bytes)
        response.append(from_jura(bytes))
    return response


def start_machine():
    send_command(CMD_MACHINE_ON)
    print receive_response()


def stop_machine():
    send_command(CMD_MACHINE_OFF)
    print receive_response()


def order_product(index):
    if(index < len(products)):
        print "product not available"
    else:
        print "ordering product " + products[index]
        send_command(CMD_SEND_PRODUCT + products[index])
        print receive_response()


def the_big_test():
    start_machine()
    send_command(CMD_GET_MACHINE_TYPE)
    print receive_response()
    for index in len(products):
        order_product(index)
    stop_machine()


# commands without arguments
CMD_MACHINE_ON = b'AN:01'
CMD_MACHINE_OFF = b'AN:02'
CMD_TEST_DISPLAY = b'AN:03'
CMD_GET_DATE = b'AN:0D'
CMD_READ_MACHINE_TYPE = b'RE:31'
CMD_READ_INPUTS = b'IC:'
CMD_GET_MACHINE_TYPE = b'TY:'
CMD_READ_RAM = b'RR:'
CMD_SEND_PRODUCT = b'?P' #third char is product name

#if defined(S95)
productsS95 = "EFABJIG";
#if defined(X7)
productsX7 = "ABCHDEKJFG";
productsTest = "ABCDEFGHIJKLMNOPQRSTUVWKYZ"
products = productsTest

# commands that need arguments
CMD_READ_WORD = b'RE:' # needs address
CMD_READ_LINE = b'RT:' # needs address

print "----------------------"
print "-  Q42 JURA HACKING  -"
print "----------------------"
test_encoder()
try:
    serialport = serial.Serial(port=SERIAL_PORT, baudrate = 9600,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                timeout=0) #9600 8 N 1
    serialport.close()
    serialport.open()
except Exception as e:
    print e
    print "Cannot open serial port ('" + str(e) + "'). This script requires a raspberry pi serial port connected to a Jura coffee machine."
    sys.exit()
print serialport.is_open
send_command(CMD_GET_MACHINE_TYPE)
r = receive_response()
print r
the_big_test()
print r

# green:  GND, RPI 06 -- orange - rechtsboven 1 (RS)
# yellow: TXD, RPI 08 -- white  - rechtsboven 2 (RS)
# red:    RXD, RPI 10 -- black  - rechtsboven 3 (RS)