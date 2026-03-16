"""
Web streaming server for wheel-tobot camera feed
Provides Flask-based MJPEG streaming over HTTP
"""

from flask import Flask, Response
import cv2
import threading
import time

app = Flask(__name__)
stream_frame = None
stream_lock = threading.Lock()
flask_thread = None

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

def update_frame(frame):
    """Update the current frame for streaming
    
    Args:
        frame: OpenCV image array (BGR format)
    """
    global stream_frame
    with stream_lock:
        stream_frame = frame.copy()

def start_streaming(host='0.0.0.0', port=5000):
    """Start Flask streaming server in background thread
    
    Args:
        host: Host address to bind to (default: 0.0.0.0 for all interfaces)
        port: Port number to listen on (default: 5000)
    """
    global flask_thread
    
    def run_flask():
        app.run(host=host, port=port, threaded=True, debug=False)
    
    flask_thread = threading.Thread(target=run_flask, daemon=True)
    flask_thread.start()
    print(f"Web streaming enabled at http://pi-tobias.local:{port}")
