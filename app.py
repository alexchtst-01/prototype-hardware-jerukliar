from flask import Flask, request, send_from_directory, render_template_string
import os
from datetime import datetime

app = Flask(__name__)
UPLOAD_FOLDER = "uploads"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route('/upload', methods=['POST'])
def upload():
    now = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"image_{now}.jpg"
    filepath = os.path.join(UPLOAD_FOLDER, filename)
    with open(filepath, 'wb') as f:
        f.write(request.data)
    print(f"Saved {filename}")
    return "Image received!", 200

@app.route('/')
def gallery():
    files = os.listdir(UPLOAD_FOLDER)
    images_html = ''.join(
        f'<div><img src="/uploads/{file}" width="400"><p>{file}</p></div>' for file in sorted(files, reverse=True)
    )
    return render_template_string(f"""
    <html>
      <head><title>ESP32-CAM Gallery</title></head>
      <body>
        <h1>Uploaded Images</h1>
        {images_html}
      </body>
    </html>
    """)

@app.route('/uploads/<filename>')
def serve_image(filename):
    return send_from_directory(UPLOAD_FOLDER, filename)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
