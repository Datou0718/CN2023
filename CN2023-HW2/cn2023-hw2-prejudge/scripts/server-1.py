from assets.utils import *
import requests
import string
import sys
import re

PORT = 8080
REPO = '..'
def regex(_content, rexexpfile):
    with open(rexexpfile, 'r') as f:
        _rexexp = f.read()
    
    content = _content.translate({ord(c): None for c in string.whitespace})
    rexexp = _rexexp.translate({ord(c): None for c in string.whitespace})

    reger = re.compile(r'{}'.format(rexexp))
    match = reger.match(content)
    assert(match)

    # print(f"{rexexp = }")
    # print(f"{content = }")
    
    return match.groups()

def parseTable(html):
    table = html[html.find("<tbody>")+7:html.rfind("</tbody>")].strip()
    # print(table)

    items = {}
    rows = table.split("<tr><td>")
    for r in rows:
        row = r.strip()
        if row == "":
            continue

        # print(row)
        reger = re.compile(r'<a href=\"([\w\W]+)\">([\w\W]+)</a></td></tr>')
        match = reger.match(row)
        href = match.groups()[0]
        text = match.groups()[1]
        
        items[text] = href

    return(items)

def uploadFile(filename):
    return {'upfile': open(filename,'rb')}

def saveFile(content, filename):
    with open(filename, 'wb') as f:
        f.write(content)

if __name__ == '__main__':
    path = ''
    if len(sys.argv) >= 2:
        PORT = sys.argv[1]
    if len(sys.argv) >= 3:
        REPO = sys.argv[2]
        
    delPath('assets/files')
    delPath('assets/download')

    if not os.path.exists('assets/files'):
        os.makedirs('assets/files')
    
    if not os.path.exists('assets/download'):
        os.makedirs('assets/download')

    genFile('assets/files/afile', size='10M')
    genFile('assets/files/alargerfile', size='100M')
    genFile('assets/files/averylargefile', size='200M')
    genFile('assets/files/a STaRanG3 F1le', size='3K')

    req = requests.get(f'http://localhost:{PORT}/')
    assert(req.status_code == 200)
    name, id = regex(req.content.decode(), 'assets/index.html')
    print(f"{name = } {id = }")

    req = requests.post(f'http://localhost:{PORT}/api/file', files=uploadFile('assets/files/afile'))
    assert(req.status_code == 401)

    req = requests.post(f'http://localhost:{PORT}/api/file', files=uploadFile('assets/files/afile'), auth=requests.auth.HTTPBasicAuth('m4JorTOM', 'SpAcEoDD1TY'))
    assert(req.status_code == 200)
    cmpFile('assets/files/afile', f'{REPO}/hw2/web/files/afile')

    req = requests.post(f'http://localhost:{PORT}/api/file', files=uploadFile('assets/files/alargerfile'))
    assert(req.status_code == 401)

    req = requests.post(f'http://localhost:{PORT}/api/file', files=uploadFile('assets/files/alargerfile'), auth=requests.auth.HTTPBasicAuth('admin', 'admin'))
    assert(req.status_code == 401)

    req = requests.post(f'http://localhost:{PORT}/api/file', files=uploadFile('assets/files/alargerfile'), auth=requests.auth.HTTPBasicAuth('demo', '123'))
    assert(req.status_code == 200)
    cmpFile('assets/files/alargerfile', f'{REPO}/hw2/web/files/alargerfile')

    req = requests.post(f'http://localhost:{PORT}/api/file', files=uploadFile('assets/files/averylargefile'), auth=requests.auth.HTTPBasicAuth('demo', '123'))
    assert(req.status_code == 200)
    cmpFile('assets/files/averylargefile', f'{REPO}/hw2/web/files/averylargefile')

    req = requests.post(f'http://localhost:{PORT}/api/file', files=uploadFile('assets/files/a STaRanG3 F1le'), auth=requests.auth.HTTPBasicAuth('demo', '123'))
    assert(req.status_code == 200)
    cmpFile('assets/files/a STaRanG3 F1le', f'{REPO}/hw2/web/files/a STaRanG3 F1le')

    req = requests.get(f'http://localhost:{PORT}/file/')
    assert(req.status_code == 200)
    regex(req.content.decode(), 'assets/listf.rhtml')
    fileList = parseTable(req.content.decode())
    print(f"{fileList = }")
    assert(fileList == {'a STaRanG3 F1le': '/api/file/a%20STaRanG3%20F1le', 'afile': '/api/file/afile', 'alargerfile': '/api/file/alargerfile', 'averylargefile': '/api/file/averylargefile'})

    req = requests.get(f'http://localhost:{PORT}/video/')
    assert(req.status_code == 200)
    regex(req.content.decode(), 'assets/listv.rhtml')
    videoList = parseTable(req.content.decode())
    print(f"{videoList = }")
    assert(videoList == {})

    req = requests.get(f'http://localhost:{PORT}/api/file/a%20STaRanG3%20F1le')
    assert(req.status_code == 200)
    saveFile(req.content, 'assets/download/a STaRanG3 F1le')
    cmpFile('assets/download/a STaRanG3 F1le', f'{REPO}/hw2/web/files/a STaRanG3 F1le')
    
    delPath('assets/files')
    delPath('assets/download')
    delPath(f'{REPO}/hw2/web/files')
    delPath(f'{REPO}/hw2/web/videos')
    delPath(f'{REPO}/hw2/web/tmp')

