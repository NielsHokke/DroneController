#!/usr/bin/python

import argparse
import dfu_transport_serial
from nrfhex import *

parser = argparse.ArgumentParser(description='Script for uploading application binary to the nrf51 dfu bootloader through serial.')
parser.add_argument('filename', type=str, help='the filename of the binary')
parser.add_argument('-p', '--port', default = '/dev/ttyUSB0', help='default is /dev/ttyUSB0')

args = parser.parse_args()
#print args.filename

#hexFirmware = nRFHex(args.filename)
#hexFirmware.tobinfile('converted.bin')

serialTransport = dfu_transport_serial.DfuTransportSerial(args.port)
serialTransport.open()
firmware = dfu_transport_serial.read_file(args.filename)
print len(firmware)
serialTransport.send_start_dfu(4,0,0,len(firmware))
serialTransport.send_init_packet(firmware)
serialTransport.send_firmware(firmware)
serialTransport.close()