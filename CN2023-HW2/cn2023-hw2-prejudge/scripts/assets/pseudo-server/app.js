const express = require('express');
const path = require('path');
const multer = require('multer');
const basicAuth = require('express-basic-auth');
const fs = require('fs');
const { exec } = require("child_process");

var app = express();
var port = 4500;

var folder = './files-nodejs/';
if (!fs.existsSync(folder)){
    fs.mkdirSync(folder);
}
exec("dd if=/dev/urandom of=files-nodejs/server.bin bs=1M count=1 > /dev/null 2>&1", (error, stdout, stderr) => {
    if (error) {
        console.log(`error: ${error.message}`);
        return;
    }
    if (stderr) {
        console.log(`stderr: ${stderr}`);
        return;
    }
    console.log(`stdout: ${stdout}`);
});

const storage = multer.diskStorage ({
    destination: (req, file, cb) => {
        cb(null, folder)
    },
    
    filename: (req, file, cb) => {
        cb(null, file.originalname)
    }
});
const upload = multer({ storage });

app.get('/', function(req, res) {
    res.send('It works!');
})

app.get('/api/file/:path*', function(req, res) {
    const file = `files-nodejs/${req.params['path']}`;
    res.download(file);
})

app.post('/api/file', basicAuth({
        users: { 'demo': '123' },
        challenge: true,
        realm: 'b12345678'
    }), upload.single('upfile'), function(req, res) {
    const file = req.file;
    if (!file) {
        res.status(500).send();
    }
    res.send('200 OK');
})

app.listen(port, function() {
  console.log('listen on ' + port);
})