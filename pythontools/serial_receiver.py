# Import the serial module
import serial# Define the serial port as a parameter
import matplotlib.pyplot as plt


serial_port = input("Enter the serial port (COM* in Windows, /dev/ttyUSB* in Linux, /dev/tty.usb*): ")# Create a serial object with the given port and baud rate

ser = serial.Serial(serial_port, 115200)# Loop indefinitely

while True:

    # Read a line from the serial output
    line = ser.readline()

    # Decode the line as UTF-8 and strip any whitespace
    # line = line.decode("utf-8").strip()

    # Print the line to the console
    # print(line)

    if len(line) < 22:
        if line[18]==10:
            line2 = ser.readline()
            line = line + line2
        else:
            print("Invalid data: ", line)
            continue

    rssi = line[5] - 256
    crc = line[6]
    pkgLen = line[7]
    pktId = line[18]

    if not (line[9] == 74 and line[10] == 87 and line[11] == 76):
        print("Invalid data: ", line)
        continue

    # print in a line
    print("PacketID:", pktId, "RSSI:", rssi, " CRC:", crc, " PKGLEN:", pkgLen)


