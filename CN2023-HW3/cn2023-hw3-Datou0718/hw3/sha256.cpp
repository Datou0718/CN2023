/*
    SHA256 sample code
*/

#include <openssl/evp.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

// to hex string
string hexDigest(const void *buf, int len) {
    const unsigned char *cbuf = static_cast<const unsigned char *>(buf);
    ostringstream hx{};

    for (int i = 0; i != len; ++i)
        hx << hex << setfill('0') << setw(2) << (unsigned int)cbuf[i];

    return hx.str();
}


int main() {
    // we want to know the hash of "h", "he", "hel", ..., "hello, world"
    // without rehashing the entire string
    string str = "hello, world";
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    // make a SHA256 object and initialize
    EVP_MD_CTX *sha256 = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha256, EVP_sha256(), NULL);
    
    for (int s_i = 0; s_i != str.size(); ++s_i) {
        // update the object given a buffer and length
        // (here we add just one character per update)
        EVP_DigestUpdate(sha256, str.c_str() + s_i, 1);

        // calculating hash
        // (we need to make a copy of `sha256` for EVP_DigestFinal_ex to use,
        // otherwise `sha256` will be broken)
        EVP_MD_CTX *tmp_sha256 = EVP_MD_CTX_new();
        EVP_MD_CTX_copy_ex(tmp_sha256, sha256);
        EVP_DigestFinal_ex(tmp_sha256, hash, &hash_len);
        EVP_MD_CTX_free(tmp_sha256);
        
        // print hash
        cout << "sha256(\"" << str.substr(0, s_i + 1) << "\") = " << hexDigest(hash, hash_len) << endl;
    }

    EVP_MD_CTX_free(sha256);
}

/*****************

# You can use the below python code to verify the hash

import hashlib

s = "hello, world"

for i in range(1, len(s)+1):
    m = hashlib.sha256()
    m.update(s[:i].encode('ascii'))
    print(f'sha256("{s[:i]}") = {m.hexdigest()}')

*****************/