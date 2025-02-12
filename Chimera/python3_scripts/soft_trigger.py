#!/usr/bin/python

from axi_gpio import AXI_GPIO
from devices import gpio_devices
import struct

def trigger():
  gpio = AXI_GPIO(gpio_devices['axi_gpio_2'])
  reg=gpio.read_axi_gpio(channel=1)
  data_unset=struct.pack('4B', (reg[0]), (reg[1]), (reg[2]), (reg[3])&253) #mask bit1 on LSB
  data_set=struct.pack('4B', (reg[0]), (reg[1]), (reg[2]), (reg[3])|2)     #set bit1 on LSB and write

  print(data_unset, data_set)

  gpio.write_axi_gpio(data_unset,channel=1)
  gpio.write_axi_gpio(data_set,channel=1)
  gpio.write_axi_gpio(data_unset,channel=1)

if __name__ == "__main__":
  trigger()
