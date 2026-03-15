from picamera2 import Picamera2
import time
import cv2
import numpy as np
import serial
import argparse
from flask import Flask, Response
import threading
import signal
import sys

# Configuration
CAMERA_WIDTH = 640
CAMERA_HEIGHT = 360
SEARCH_REDUCTION = 0.3  # Reduce error by 70% when searching for lost line

# Serial communication setup
SERIAL_PORT = '/dev/ttyS0'  # Raspberry Pi hardware UART (TX=GPIO14, RX=GPIO15)
BAUD_RATE = 57600
# Parse command line arguments
parser = argparse.ArgumentParser(description='Line following robot with optional web streaming')
parser.add_argument('--stream', action='store_true', help='Enable web streaming on port 5000')
args = parser.parse_args()

# Flask app for streaming (only if enabled)
if args.stream:
    app = Flask(__name__)
    stream_frame = None
    stream_lock = threading.Lock()
# Initialize serial connection to Arduino
try:
    arduino_serial = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.01)
    time.sleep(2)  # Wait for Arduino to reset after serial connection
    # Flush any garbage data from initial connection
    arduino_serial.reset_input_buffer()
    arduino_serial.reset_output_buffer()
    print(f"Serial connection established on {SERIAL_PORT} at {BAUD_RATE} baud")
except Exception as e:
    print(f"Failed to open serial port: {e}")
    print("Make sure UART is enabled in raspi-config and Arduino is connected")
    sys.exit(1)

def send_error_value(error):
    """Send proportional error value to Arduino via UART
    
    Args:
        error: Line position error in pixels (-320 to +320)
               Negative = line on left, Positive = line on right
    
    Protocol: [SYNC][ERROR_HIGH][ERROR_LOW][CHECKSUM]
    """
    
    # Convert to signed 16-bit bytes
    error_signed = error if error >= 0 else (0x10000 + error)
    error_high = (error_signed >> 8) & 0xFF
    error_low = error_signed & 0xFF
    
    # Simple checksum
    checksum = (0xFF + error_high + error_low) & 0xFF
    
    # Send packet
    packet = bytes([0xFF, error_high, error_low, checksum])
    try:
        arduino_serial.write(packet)
    except Exception as e:
        print(f"Serial write error: {e}")

# Setup camera with picamera2
camera = Picamera2()
camera_config = camera.create_preview_configuration(
    main={"size": (CAMERA_WIDTH, CAMERA_HEIGHT), "format": "RGB888"}
)
camera.configure(camera_config)
camera.start()
time.sleep(1)  # Camera warm-up

center_x = CAMERA_WIDTH // 2  # Center of image

print("Starting line following with UART serial control...")

# Wait longer for Arduino to fully boot and clear bootloader garbage
time.sleep(1)
arduino_serial.reset_input_buffer()
time.sleep(0.5)

send_error_value(0)  # Start with no error (motors will be controlled by Arduino)

frame_count = 0
start_time = time.time()
last_status_time = start_time
last_error = 0  # Track last error for line search
frames_without_line = 0  # Count frames where line is lost
line_found = False  # Has the line ever been found?
running = True  # Control flag for main loop

# Signal handler for graceful shutdown
def signal_handler(sig, frame):
    global running
    print("\nShutdown signal received...")
    running = False

signal.signal(signal.SIGINT, signal_handler)

if args.stream:
    @app.route('/')
    def index():
        """Video streaming home page"""
        return '''
        <html>
        <head>
            <title>Wheel-Tobot Camera Feed</title>
            <style>
                body { background: #000; color: #0f0; font-family: monospace; text-align: center; }
                img { max-width: 100%; height: auto; border: 2px solid #0f0; }
                h1 { color: #0f0; }
            </style>
        </head>
        <body>
            <h1>🤖 Wheel-Tobot Live Camera Feed</h1>
            <img src="/video_feed" />
            <p>Streaming from Raspberry Pi</p>
        </body>
        </html>
        '''
    
    @app.route('/video_feed')
    def video_feed():
        """Video streaming route - returns MJPEG stream"""
        def generate():
            global stream_frame
            while True:
                with stream_lock:
                    if stream_frame is None:
                        continue
                    # Encode frame as JPEG
                    ret, jpeg = cv2.imencode('.jpg', stream_frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
                    if not ret:
                        continue
                    frame_bytes = jpeg.tobytes()
                
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
                time.sleep(0.033)  # ~30 FPS max for stream
        
        return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')
    
    # Start Flask in background thread
    def run_flask():
        app.run(host='0.0.0.0', port=5000, threaded=True, debug=False)
    
    flask_thread = threading.Thread(target=run_flask, daemon=True)
    flask_thread.start()
    print("Web streaming enabled at http://pi-tobias.local:5000")

while running:
        # Capture frame
        image = camera.capture_array()
        roi = image[200:250, 0:639]
        Blackline = cv2.inRange(roi, (0, 0, 0), (50, 50, 50))
        kernel = np.ones((3, 3), np.uint8)
        Blackline = cv2.erode(Blackline, kernel, iterations=5)
        Blackline = cv2.dilate(Blackline, kernel, iterations=9)	
        contours, hierarchy = cv2.findContours(Blackline.copy(), cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)	
        
        error = 0  # Default: No error (centered)
        command_text = "CENTERED"
        line_center = None
        
        if len(contours) > 0:
            # Line detected!
            frames_without_line = 0
            line_found = True
            
            # Find the largest contour (most likely the line)
            largest_contour = max(contours, key=cv2.contourArea)
            x, y, w, h = cv2.boundingRect(largest_contour)
            line_center = x + (w / 2)
            
            # Calculate proportional error (pixels from center)
            error = int(line_center - center_x)
            last_error = error  # Remember for line search
            
            # Generate status text based on error magnitude
            if abs(error) < 20:
                command_text = "CENTERED"
            elif error < 0:
                command_text = "TURN_LEFT"
            else:
                command_text = "TURN_RIGHT"
        else:
            # No line detected
            frames_without_line += 1
            
            if not line_found:
                # Never found line - stop and wait
                error = 0
                command_text = "WAITING"
            else:
                # Line lost for several frames - enter search mode
                if abs(last_error) < 20:
                    # Was centered when lost - stop completely
                    error = 0
                    command_text = "STOPPED"
                else:
                    # Search slowly in last known direction
                    error = int(last_error * SEARCH_REDUCTION)
                    if last_error < 0:
                        command_text = "SEARCH_LEFT"
                    else:
                        command_text = "SEARCH_RIGHT"
        
        # Send proportional error to Arduino
        send_error_value(error)
        
        # Update frame counter
        frame_count += 1

        # Draw visualization for streaming and debug frames
        debug_img = image.copy()
        cv2.rectangle(debug_img, (0, 200), (639, 250), (0, 165, 255), 2)
        if line_center is not None:
            cv2.line(debug_img, (int(line_center), 200), (int(line_center), 250), (255, 0, 0), 3)
            error_val = int(line_center - center_x)
            cv2.putText(debug_img, f"Error: {error_val}px", (10, 70), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)
        cv2.line(debug_img, (center_x, 200), (center_x, 250), (0, 255, 0), 2)
        cv2.putText(debug_img, f"CMD: {command_text}", (10, 30), 
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        
        # Update stream frame if streaming is enabled
        if args.stream:
            with stream_lock:
                stream_frame = debug_img.copy()
        
        # Print status every 2 seconds
        current_time = time.time()
        if current_time - last_status_time >= 2.0:
            fps = frame_count / (current_time - start_time)
            status_msg = f"[{current_time - start_time:.1f}s] FPS: {fps:.1f} | CMD: {command_text} | ERR: {error:+4d} | CNT: {len(contours)}"
            if frames_without_line > 0:
                status_msg += f" | LOST: {frames_without_line}"
            print(status_msg)
            last_status_time = current_time

        # Minimal delay for headless mode (maximize FPS)
        time.sleep(0.001)

# Cleanup
print("\nCleaning up...")
send_error_value(0)  # Send zero error (stop motors)
time.sleep(0.1)
arduino_serial.close()
camera.stop()
print("Done!")

# Print final statistics
total_time = time.time() - start_time
avg_fps = frame_count / total_time if total_time > 0 else 0
print(f"Session complete: {frame_count} frames in {total_time:.1f}s (avg {avg_fps:.1f} FPS)")
    