"""
parameter message
|byte 1 |byte 2          |byte      |byte 4 |byte 5 |byte6     |byte 7 |
|0x55   |register adress |Data(msb) |data   |data   |data(lsb) |CRC    |


desired new mode

register adresses
|adress |msb     |        |        |lsb     |
|0x00   |--------|--------|MIN_LIFT|MAX_RPM | BOUNDERIES
|0x04   |--------|--------|--------|mode    | NEWMODE: the new desired state to be in
|0x08   |--------|--------|P_YAW   |P_YAW   | PARAMETER_YAW : the parameter to be used for oa yaw controll
|0x0C   |anglemax|anglemin|yawmax  |yawmin  | BOUNDERIES
|0x10   |P1      |P1      |P2      |P2      | P1 values and P2 values
"""

REGMAP_MINLIFT = b'\x02'
REGMAP_MAXRPM = b'\x03'
REGMAP_NEWMODE = b'\x04'
REGMAP_PARAMETER_YAW = b'\x08'
REGMAP_BOUNDARIES = b'\x0C'
REGMAP_PARAMETER_P1_P2 = b'\x10'
