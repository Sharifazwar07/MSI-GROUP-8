import serial
import matplotlib.pyplot as plt
import time

ser = serial.Serial('COM5', 9600, timeout=1)
time.sleep(2)

plt.ion()
fig, ax = plt.subplots()
x_vals, y_vals = [], []

try:
    while True:
        pot_value = ser.readline().decode('utf-8', errors='ignore').strip()
        if pot_value.isdigit():
            val = int(pot_value)
            print("Potentiometer Value:", val)

            x_vals.append(len(x_vals))
            y_vals.append(val)

            ax.clear()
            ax.plot(x_vals, y_vals)
            plt.pause(0.1)

except KeyboardInterrupt:
    pass
finally:
    ser.write(b'q')
    ser.close()
    plt.ioff()
    plt.show()
    print("Serial connection closed.")


