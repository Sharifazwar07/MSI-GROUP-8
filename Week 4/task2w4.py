import serial
import time
import numpy as np

# Configured for COM5 and 9600 baud rate
SERIAL_PORT = 'COM5'
BAUD_RATE = 9600

# REPLACE these strings with the actual ID numbers scanned by your USB RFID reader
authorized_cards = ["0013078065"]


def detect_circular_motion(ser, duration=4.0, min_radius=2000):
    """
    Reads MPU6050 X-Y data for a specific duration to verify
    if the sensor moves in a circular pattern across 4 quadrants.
    """
    print("\n--> Please perform a circular motion with the MPU6050 now (for 4 seconds)...")
    ser.reset_input_buffer()

    x_vals, y_vals = [], []
    start_time = time.time()

    # Capture data for 4 seconds
    while time.time() - start_time < duration:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                values = line.split(',')
                if len(values) == 3:
                    ax_val = int(values[0])
                    ay_val = int(values[1])
                    x_vals.append(ax_val)
                    y_vals.append(ay_val)
        except ValueError:
            continue

    if len(x_vals) < 10:
        print("--> Failed: Insufficient sensor data received.")
        return False

    center_x = np.mean(x_vals)
    center_y = np.mean(y_vals)

    quadrants = set()
    for x, y in zip(x_vals, y_vals):
        dx = x - center_x
        dy = y - center_y
        radius = np.sqrt(dx ** 2 + dy ** 2)

        if radius > min_radius:
            if dx > 0 and dy > 0:
                quadrants.add(1)  # Quadrant 1
            elif dx < 0 and dy > 0:
                quadrants.add(2)  # Quadrant 2
            elif dx < 0 and dy < 0:
                quadrants.add(3)  # Quadrant 3
            elif dx > 0 and dy < 0:
                quadrants.add(4)  # Quadrant 4

    print(f"--> Circular quadrants detected: {len(quadrants)} out of 4.")
    return len(quadrants) == 4


def main():
    try:
        print(f"Connecting to Arduino on {SERIAL_PORT}...")
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        print("=== SMART ACCESS SYSTEM READY ===")

        while True:
            # Click inside the terminal before scanning. The USB RFID types the ID here automatically.
            uid = input("\nClick here, then scan (tap) your RFID card: ").strip().upper()

            if not uid:
                continue

            print(f"Card Detected: '{uid}'")

            if uid in authorized_cards:
                print("RFID Status: AUTHORIZED. Proceed to motion verification.")
                if detect_circular_motion(ser):
                    print("Motion Status: VERIFIED! Access Granted (Servo Unlock, Green LED ON).")
                    ser.write(b'A')
                else:
                    print("Motion Status: FAILED! Access Denied (Servo Lock, Red LED ON).")
                    ser.write(b'D')
            else:
                print(f"RFID Status: DENIED! '{uid}' is not an authorized card.")
                ser.write(b'D')

    except serial.SerialException as e:
        print(f"\nFailed to open port {SERIAL_PORT}: {e}")
        print("Make sure the Serial Monitor in Arduino IDE is closed!")
    except KeyboardInterrupt:
        print("\nProgram terminated by user.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


if __name__ == "__main__":
    main()