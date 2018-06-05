"""
parameter message
|byte 1 |byte 2          |byte      |byte 4 |byte 5 |byte6     |byte 7 |
|0x55   |register adress |Data(msb) |data   |data   |data(lsb) |CRC    |


desired new mode

register adresses
|adress |msb     |        |        |lsb     |
|0x00   |--------|--------|--------|--------| intentionally unused
|0x04   |--------|--------|--------|mode    | NEWMODE: the new desired state to be in
|0x08   |--------|--------|--------|P       | PARAMETER_YAW : the parameter to be used for oa yaw controll
|0x12   |--------|--------|--------|P1      |
|0x16   |--------|--------|--------|P2      |
parameter2
parameter3
parameter4

|0xFB   |--------|--------|--------|0xFF    | ABORTREQUEST: the drone is requested to safely abort


"""

REGMAP_NEWMODE = b'\x04'
REGMAP_PARAMETER_YAW = b'\x08'
REGMAP_PARAMETER_P1 = b'\x12'
REGMAP_PARAMETER_P2 = b'\x16'

REGMAP_ABORTREQUEST = b'\xFB'