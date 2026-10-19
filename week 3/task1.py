import serial
import time


ser = serial.Serial('COM5', 9600, timeout=1)
time.sleep(2)

try:
    while True:
        raw_data = ser.readline().decode('utf-8', errors='ignore').strip()

        if raw_data.isdigit():
            pot_value = int(raw_data)

            if pot_value > 512:
                ser.write(b'1')
                led_status = "ON"
            else:
                ser.write(b'0')
                led_status = "OFF"

            print(f"Potentiometer Value: {pot_value} | LED: {led_status}")

except KeyboardInterrupt:
    ser.close()
    print("Serial connection closed.")
