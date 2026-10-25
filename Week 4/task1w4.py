import serial
import matplotlib
matplotlib.use('TkAgg')  # Forces an interactive pop-up window in PyCharm
import matplotlib.pyplot as plt
import numpy as np
from collections import deque

# =====================================================================
# IMPROVEMENT 1: Serial Configuration & Memory Buffer Limit
# Using deque(maxlen=35) to store only the 35 most recent data points.
# This prevents the plot from lagging and becoming cluttered over time.
# =====================================================================
SERIAL_PORT = 'COM5'  # Change to match your Arduino COM port (e.g., 'COM4')
BAUD_RATE = 9600
MAX_POINTS = 35

ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
ser.flushInput()

x_vals = deque(maxlen=MAX_POINTS)
y_vals = deque(maxlen=MAX_POINTS)

# =====================================================================
# IMPROVEMENT 2: Fixed Plot Syntax & Path Visualization
# Fixed the syntax error in ax.plot() from the starter code.
# Using 'ro-' (red dots connected by lines) to clearly visualize
# the continuous real-time hand motion path.
# =====================================================================
plt.ion()
fig, ax = plt.subplots(figsize=(7, 7))
ax.set_xlim(-20000, 20000)
ax.set_ylim(-20000, 20000)
ax.set_title("Task 1: Real-Time MPU6050 Motion Tracking")
ax.set_xlabel("Acceleration X (ax)")
ax.set_ylabel("Acceleration Y (ay)")
ax.grid(True, linestyle='--', alpha=0.6)

line, = ax.plot([], [], 'ro-', linewidth=2, markersize=5, label='Motion Path')
center_dot, = ax.plot([], [], 'bx', markersize=10, label='Circle Center')
status_box = ax.text(-18500, 17500, "Status: Collecting data...", fontsize=11, fontweight='bold',
                     bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
ax.legend(loc='lower right')


# =====================================================================
# IMPROVEMENT 3: Circular Motion Detection Algorithm
# Detects circular gestures based on 3 geometric criteria:
# 1. Motion amplitude (X and Y spans must exceed 6000 units).
# 2. Coverage across all 4 quadrants around the center point (centroid).
# 3. Radius consistency (standard deviation of radius relative to mean).
# =====================================================================
def check_circular_gesture(x_arr, y_arr):
    if len(x_arr) < 16:
        return False, 0, 0

    x = np.array(x_arr)
    y = np.array(y_arr)

    # Calculate the center of motion (centroid)
    cx, cy = np.mean(x), np.mean(y)

    # 1. Check motion amplitude/span to ignore minor hand tremors or noise
    span_x = np.max(x) - np.min(x)
    span_y = np.max(y) - np.min(y)
    if span_x < 6000 or span_y < 6000:
        return False, cx, cy

    # 2. Verify that the path crosses all four quadrants around the center
    dx = x - cx
    dy = y - cy
    q1 = np.any((dx > 0) & (dy > 0))
    q2 = np.any((dx < 0) & (dy > 0))
    q3 = np.any((dx < 0) & (dy < 0))
    q4 = np.any((dx > 0) & (dy < 0))

    if not (q1 and q2 and q3 and q4):
        return False, cx, cy

    # 3. Evaluate circularity using radius consistency (standard deviation)
    radii = np.sqrt(dx ** 2 + dy ** 2)
    mean_radius = np.mean(radii)
    std_radius = np.std(radii)

    # If the relative standard deviation of the radius is < 0.45, it forms a circle
    is_circle = (std_radius / mean_radius) < 0.45
    return is_circle, cx, cy


print("Starting motion tracking... Move the sensor in a circular pattern. Press Ctrl+C to exit.")

while True:
    try:
        raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
        if raw_line:
            values = raw_line.split(',')

            # =====================================================================
            # IMPROVEMENT 4: Fixed Serial Data Length Check
            # The Arduino sketch sends 3 values (ax, ay, az), not 6 as checked in
            # the starter code (len(values) == 6). Updated to len(values) == 3.
            # =====================================================================
            if len(values) == 3:
                ax_val = int(values[0])
                ay_val = int(values[1])

                x_vals.append(ax_val)
                y_vals.append(ay_val)

                # =================================================================
                # IMPROVEMENT 5: Moving Average Filter (Smoothing)
                # Smooths raw MPU6050 accelerometer readings to reduce sensor noise.
                # =================================================================
                if len(x_vals) >= 3:
                    smooth_x = np.convolve(x_vals, np.ones(3) / 3, mode='valid')
                    smooth_y = np.convolve(y_vals, np.ones(3) / 3, mode='valid')
                    line.set_data(smooth_x, smooth_y)
                else:
                    line.set_data(x_vals, y_vals)

                # Check if the current trajectory forms a circular motion
                detected, cx, cy = check_circular_gesture(x_vals, y_vals)
                if detected:
                    center_dot.set_data([cx], [cy])
                    status_box.set_text("Status: CIRCULAR MOTION DETECTED!")
                    status_box.set_bbox(dict(boxstyle='round', facecolor='lightgreen', alpha=0.9))
                else:
                    center_dot.set_data([], [])
                    status_box.set_text("Status: Tracking motion...")
                    status_box.set_bbox(dict(boxstyle='round', facecolor='wheat', alpha=0.8))

                plt.draw()
                plt.pause(0.02)

    except ValueError:
        # Ignore incomplete or corrupted serial lines during startup
        continue
    except KeyboardInterrupt:
        print("\nProgram terminated by user.")
        break

ser.close()
plt.close()