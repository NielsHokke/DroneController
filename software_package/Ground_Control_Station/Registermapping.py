"""
parameter message
|byte 1 |byte 2          |byte      |byte 4 |byte 5 |byte6     |byte 7 |
|0x55   |register adress |Data(msb) |data   |data   |data(lsb) |CRC    |


desired new mode

register adresses
|adress |msb     |        |        |lsb     |
|0x00   |--------|--------|--------|--------| intentionally unused
|0x01   |--------|--------|--------|mode    | NEWMODE: the new desired state to be in

|0xFF   |--------|--------|--------|0xFF    | ABORTREQUEST: the drone is requested to safely abort


"""

REGMAP_NEWMODE = b'\x01'
REGMAP_ABORTREQUEST = b'\xFF'