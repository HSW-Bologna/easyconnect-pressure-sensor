#!/usr/bin/env python3
import minimalmodbus
import serial
import sys



def setSerialNumber(port, slave_address, value):

    instrument = minimalmodbus.Instrument(port, slave_address)  # port name, slave address (in decimal)
    instrument.serial.baudrate = 115200         # Baud
    instrument.serial.bytesize = 8
    instrument.serial.parity   = serial.PARITY_NONE
    instrument.serial.stopbits = 1
    instrument.serial.timeout  = 0.05          # seconds

    instrument.write_register(2, value)
    new_value = instrument.read_register(2)
    print(f"update register with value: {new_value}")
    
if __name__ == "__main__":
    if len(sys.argv)<4:
        print(f"usage: {sys.argv[0]} porta indirizzo valore")
        exit(1)
    setSerialNumber(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))