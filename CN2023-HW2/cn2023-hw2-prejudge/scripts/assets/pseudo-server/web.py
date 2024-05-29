from flask import Flask, request, send_from_directory
from flask_httpauth import HTTPBasicAuth
import os

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = 'files-flask'
app.config['MAX_CONTENT_LENGTH'] = 200000000
auth = HTTPBasicAuth()

users = {
    "demo": "123"
}

@auth.verify_password
def verify_password(username, password):
    if username in users and users.get(username) == password:
        return username
    
@app.route('/', methods=['GET'])
@auth.login_required
def test():
    return "It works!"

@app.route('/api/file/<path:filepath>', methods=['GET'])
def download(filepath):
    return send_from_directory('files-flask', filepath)

@app.route('/api/file', methods=['POST'])
@auth.login_required
def upload():
    file = request.files['upfile']
    if file:
        file.save(os.path.join(app.config['UPLOAD_FOLDER'], file.filename))
        return "200 OK"

if __name__ == '__main__':
    def genFile(filename, size='1M'):
        os.system(f'dd if=/dev/urandom of={filename} bs={size} count=1 > /dev/null 2>&1')

    if not os.path.exists('files-flask'):
        os.makedirs('files-flask')

    genFile('files-flask/server.bin', size='1M')
    app.run(debug=True, port=2023, host="0.0.0.0")