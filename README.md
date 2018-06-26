# DroneController

For the TU Delft course CS4140ES Embedded Systems Lab writen, FreeRTOS based drone controller.

## Report
[Cognitive WI-FI: A small step towards online self-learning data rate control](https://github.com/NielsHokke/DynamicDatarateControler/blob/master/Cognitive%20WI-FI%20A%20small%20step%20towards%20online%20self-learning%20data%20rate%20control.pdf)

## Getting Started

### Prerequisites

* Linux
* python 3

### Running

#### Programming Drone

* in software_package/in4073/ run:
```
sudo make -B upload port="/dev/ttyUSB0"
```

#### GUI

* in software_package/Ground_Control_Station/ run:
```
sudo python3 GCS.py 
```

## Authors

* **Niels Hokke** - [NielsHokke](https://github.com/NielsHokke)
* **Jetse Brouwer** - [JetseBrouwer](https://github.com/JetseBrouwer)
* **David Enthoven** - [David Enthoven](https://github.com/Davidenthoven)
* **Nilay Sheth** - [Nilay Sheth](https://github.com/nilay994)