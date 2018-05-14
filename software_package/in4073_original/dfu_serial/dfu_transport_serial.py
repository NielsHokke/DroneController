#!/usr/bin/python

# Copyright (c) 2015, Nordic Semiconductor
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of Nordic Semiconductor ASA nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Python imports
import sys

import time
from datetime import datetime, timedelta
import binascii
import logging
import hashlib
import struct

# Python 3rd party imports
from serial import Serial

# Nordic Semiconductor imports
from util import slip_parts_to_four_bytes, slip_encode_esc_chars, int16_to_bytes, int32_to_bytes
import crc16 
#from package import calculate_sha256_hash
#from nordicsemi.exceptions import NordicSemiException
#from nordicsemi.dfu.dfu_transport import DfuTransport, DfuEvent


logger = logging.getLogger(__name__)


class DfuTransportSerial():

    DEFAULT_BAUD_RATE = 921600
    DEFAULT_FLOW_CONTROL = True
    DEFAULT_SERIAL_PORT_TIMEOUT = 1.0  # Timeout time on serial port read
    ACK_PACKET_TIMEOUT = 1.0  # Timeout time for for ACK packet received before reporting timeout through event system
    SEND_INIT_PACKET_WAIT_TIME = 1.0  # Time to wait before communicating with bootloader after init packet is sent
    SEND_START_DFU_WAIT_TIME = 2.0  # Time to wait before communicating with bootloader after start DFU packet is sent			## was 10 sec
    DFU_PACKET_MAX_SIZE = 512  # The DFU packet max size

    def __init__(self, com_port, baud_rate=DEFAULT_BAUD_RATE, flow_control=DEFAULT_FLOW_CONTROL, timeout=DEFAULT_SERIAL_PORT_TIMEOUT):
        self.com_port = com_port
        self.baud_rate = baud_rate
        self.flow_control = 1 if flow_control else 0
        self.timeout = timeout
        self.serial_port = None
        """:type: serial.Serial """

    def open(self):
        
        self.serial_port = Serial(port=self.com_port, baudrate=self.baud_rate, rtscts=self.flow_control, timeout=self.timeout)
	self.serial_port.flushInput()
	self.serial_port.flushOutput()
	self.serial_port.flush()

    def close(self):
        self.serial_port.close()

    def is_open(self):
        
        if self.serial_port is None:
            return False

        return self.serial_port.isOpen()

    def send_validate_firmware(self):
        super(DfuTransportSerial, self).send_validate_firmware()
        return True

    def send_init_packet(self,firmware):
#        super(DfuTransportSerial, self).send_init_packet(init_packet)

	init_packet = 'ffffffffffffffff0100feff'
	init_packet += binascii.hexlify(struct.pack('<H', crc16.calc_crc16(firmware, crc=0xffff)))
	init_packet = binascii.unhexlify(init_packet)
	
        frame = int32_to_bytes(DFU_INIT_PACKET)
        frame += init_packet
        frame += int16_to_bytes(0x0000)  # Padding required

        packet = HciPacket(frame)
        self.send_packet(packet)
        time.sleep(DfuTransportSerial.SEND_INIT_PACKET_WAIT_TIME)

    def send_start_dfu(self, mode, softdevice_size=None, bootloader_size=None, app_size=None):
  #      super(DfuTransportSerial, self).send_start_dfu(mode, softdevice_size, bootloader_size, app_size)

        frame = int32_to_bytes(DFU_START_PACKET)
        frame += int32_to_bytes(mode)
        frame += self.create_image_size_packet(softdevice_size, bootloader_size, app_size)

        packet = HciPacket(frame)
        self.send_packet(packet)
        time.sleep(DfuTransportSerial.SEND_START_DFU_WAIT_TIME)

    def send_activate_firmware(self):
        super(DfuTransportSerial, self).send_activate_firmware()

    def send_firmware(self, firmware):
#        super(DfuTransportSerial, self).send_firmware(firmware)

        def progress_percentage(part, whole):
            return int(100 * float(part)/float(whole))

        frames = []
#        self._send_event(DfuEvent.PROGRESS_EVENT, progress=0, done=False, log_message="")

        for i in range(0, len(firmware), DfuTransportSerial.DFU_PACKET_MAX_SIZE):
            data_packet = HciPacket(int32_to_bytes(DFU_DATA_PACKET) + firmware[i:i + DfuTransportSerial.DFU_PACKET_MAX_SIZE])
            frames.append(data_packet)

        frames_count = len(frames)

        # Send firmware packets
        for count, pkt in enumerate(frames):
            self.send_packet(pkt)	
            sys.stderr.write("\rProgress: %d/%d frames, %d%%" % (count+1, frames_count, progress_percentage(count+1, frames_count)))
       	sys.stderr.write('\r\n')

        # Send data stop packet
        frame = int32_to_bytes(DFU_STOP_DATA_PACKET)
        packet = HciPacket(frame)
        self.send_packet(packet)

#        self._send_event(DfuEvent.PROGRESS_EVENT, progress=100, done=False, log_message="")

    def send_packet(self, pkt):
        attempts = 0
        last_ack = None
        packet_sent = False

        logger.debug("PC -> target: {0}".format(pkt))

#	print format(binascii.hexlify(pkt.data))	

#	print "Sequence number HCI: " + str(HciPacket.sequence_number)

        while not packet_sent:
            self.serial_port.write(pkt.data)
            attempts += 1
	    time.sleep(0.05)
            ack = self.get_ack_nr()
	    
	   # print "attempts: " + str(attempts)

            if last_ack is None:
		#print str(ack)
                break

            if ack == (last_ack + 1) % 8:
                last_ack = ack
                packet_sent = True

                if attempts > 3:
                    raise Exception("Three failed tx attempts encountered on packet {0}".format(pkt.sequence_number))

    def get_ack_nr(self):
        def is_timeout(start_time, timeout_sec):
            return not (datetime.now() - start_time <= timedelta(0, timeout_sec))

        uart_buffer = ''
        start = datetime.now()

        while uart_buffer.count('\xC0') < 2:
            # Disregard first of the two C0
            temp = self.serial_port.read(6)

	    if temp:
                uart_buffer += temp
		#print format(binascii.hexlify(uart_buffer))

            if is_timeout(start, DfuTransportSerial.ACK_PACKET_TIMEOUT):
                # reset HciPacket numbering back to 0
                HciPacket.sequence_number = 0
                print "Timed out waiting for acknowledgement from device."
		

                # quit loop
                break

                # read until you get a new C0
                # RESUME_WORK

        if len(uart_buffer) < 2:
            	print "No data received on serial port. Not able to proceed."
		return

        logger.debug("PC <- target: {0}".format(binascii.hexlify(uart_buffer)))
        data = self.decode_esc_chars(uart_buffer)

        # Remove 0xC0 at start and beginning
        data = data[1:-1]

        # Extract ACK number from header
        return (data[0] >> 3) & 0x07

    @staticmethod
    def decode_esc_chars(data):
        """Replace 0xDBDC with 0xCO and 0xDBDD with 0xDB"""
        result = []

        data = bytearray(data)

        while len(data):
            char = data.pop(0)

            if char == 0xDB:
                char2 = data.pop(0)

                if char2 == 0xDC:
                    result.append(0xC0)
                elif char2 == 0xDD:
                    result.append(0xDB)
                else:
                    raise Exception('Char 0xDB NOT followed by 0xDC or 0xDD')
            else:
                result.append(char)

        return result

    @staticmethod
    def create_image_size_packet(softdevice_size=0, bootloader_size=0, app_size=0):
        """
        Creates an image size packet necessary for sending start dfu.

        @param softdevice_size: Size of SoftDevice firmware
        @type softdevice_size: int
        @param bootloader_size: Size of bootloader firmware
        @type softdevice_size: int
        @param app_size: Size of application firmware
        :return: The image size packet
        :rtype: str
        """
        softdevice_size_packet = int32_to_bytes(softdevice_size)
        bootloader_size_packet = int32_to_bytes(bootloader_size)
        app_size_packet = int32_to_bytes(app_size)
        image_size_packet = softdevice_size_packet + bootloader_size_packet + app_size_packet
        return image_size_packet


DATA_INTEGRITY_CHECK_PRESENT = 1
RELIABLE_PACKET = 1
HCI_PACKET_TYPE = 14

DFU_INIT_PACKET = 1
DFU_START_PACKET = 3
DFU_DATA_PACKET = 4
DFU_STOP_DATA_PACKET = 5

DFU_UPDATE_MODE_NONE = 0
DFU_UPDATE_MODE_SD = 1
DFU_UPDATE_MODE_BL = 2
DFU_UPDATE_MODE_APP = 4


class HciPacket(object):
    """Class representing a single HCI packet"""

    sequence_number = 0

    def __init__(self, data=''):
        HciPacket.sequence_number = (HciPacket.sequence_number + 1) % 8
        self.temp_data = ''
        self.temp_data += slip_parts_to_four_bytes(HciPacket.sequence_number,
                                                   DATA_INTEGRITY_CHECK_PRESENT,
                                                   RELIABLE_PACKET,
                                                   HCI_PACKET_TYPE,
                                                   len(data))
        self.temp_data += data
        # Add escape characters
        crc = crc16.calc_crc16(self.temp_data, crc=0xffff)

        self.temp_data += chr(crc & 0xFF)
        self.temp_data += chr((crc & 0xFF00) >> 8)

        self.temp_data = slip_encode_esc_chars(self.temp_data)

        self.data = chr(0xc0)
        self.data += self.temp_data
        self.data += chr(0xc0)

    def __str__(self):
        return binascii.hexlify(self.data)

def calculate_sha256_hash(firmware_filename):
	read_buffer = 4096

        digest = hashlib.sha256()

        with open(firmware_filename, 'rb') as firmware_file:
            while True:
                data = firmware_file.read(read_buffer)

                if data:
                    digest.update(data)
                else:
                    break

        return digest.digest()

def read_file(file_path):
        """
        Reads a file and returns the content as a string.

        :param str file_path: The path to the file to read.
        :return str: Content of the file.
        """
        buffer_size = 4096

        file_content = ""

        with open(file_path, 'rb') as binary_file:
            while True:
                data = binary_file.read(buffer_size)

                if data:
                    file_content += data
                else:
                    break

        return file_content

