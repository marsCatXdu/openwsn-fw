import matplotlib.pyplot as plt
import re

def read_rssi_data(filename):
    data = {}
    current_distance = filename.split('.')[0] + ' cm'
    rssi_values = []

    with open(filename, 'r') as file:
        for line in file:
            # Example line format: "PacketID: 0 RSSI: -32 CRC: 1 PKGLEN: 127"
            match = re.search(r'PacketID: (\d+) RSSI: (-?\d+)', line)
            if match:
                packet_id = int(match.group(1))
                rssi = int(match.group(2))
                
                # Store the RSSI value
                rssi_values.append(rssi)
                data[current_distance] = rssi_values

    return data

filename1 = '20.txt'
data1 = read_rssi_data(filename1)
filename2 = '50.txt'
data2 = read_rssi_data(filename2)
filename3 = '100.txt'
data3 = read_rssi_data(filename3)
filename4 = '200.txt'
data4 = read_rssi_data(filename4)
filename5 = '300.txt'
data5 = read_rssi_data(filename5)
filename6 = '400.txt'
data6 = read_rssi_data(filename6)

plt.figure(figsize=(10, 6))
for distance, rssi_values in data1.items():
    plt.plot(rssi_values, label=distance) 

for distance, rssi_values in data2.items():
    plt.plot(rssi_values, label=distance)

for distance, rssi_values in data3.items():
    plt.plot(rssi_values, label=distance)

for distance, rssi_values in data4.items():
    plt.plot(rssi_values, label=distance)

for distance, rssi_values in data5.items():
    plt.plot(rssi_values, label=distance)

for distance, rssi_values in data6.items():
    plt.plot(rssi_values, label=distance)

plt.title("RSSI Changes with Distance over Time")
plt.xlabel("Packet ID")
plt.ylabel("RSSI")
plt.legend(title="Distances")
plt.grid(True)

plt.show()
