#include "163.h"

#include "cstr_util.h"
#include "str_util.h"
extern "C" {
#include "libavutil/avutil.h"
}
#include "malloc.h"
#include "openssl/bio.h"
#include "openssl/buffer.h"
#include "openssl/evp.h"
#include "openssl/err.h"
#include "openssl/pem.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <stdlib.h>
#include <string.h>
#include <regex>


const std::string iv = "0102030405060708";
const std::string presetKey = "0CoJUm6Qyw8W8jud";
const std::string linuxapiKey = "rFgB&h#%2?^eDg:Q";
const std::string base62 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const std::string publicKey = "-----BEGIN PUBLIC KEY-----\n\
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDgtQn2JZ34ZC28NWYpAUd98iZ37BUrX/aKzmFbt7clFSs6sXqHauqKWqdtLkF2KexO40H1YTX8z2lSgBBOAxLsvaklV8k4cBFK9snQXE9/DDaFt6Rr7iVZMldczhC0JNgTz+SHXT6CBHuX3e9SdB1Ua44oncaTWz7OBGLbCiK45wIDAQAB\n\
-----END PUBLIC KEY-----";
const std::string eapiKey = "e82ckenh8dichen8";

using namespace rapidjson;

NeteaseMusicApi::NeteaseMusicApi() {
    this->client.options.https = true;
    this->client.headers["User-Agent"] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.1938.39 Safari/537.36";
    this->client.headers["Connection"] = "close";
    this->client.headers["Referer"] = "https://music.163.com/";
}

Document NeteaseMusicApi::fetchSongDetail(uint64_t id) {
    return fetchSongDetails({id});
}

Document NeteaseMusicApi::fetchSongDetails(std::list<uint64_t> id) {
    auto r = this->client.request("/api/v3/song/detail", "POST");
    av_log(nullptr, AV_LOG_DEBUG, "POST /api/v3/song/detail\n");
    Document d;
    d.SetObject();
    Value s;
    Document c;
    c.SetArray();
    for (auto &i: id) {
        Value v;
        v.SetObject();
        Value id;
        id.SetUint64(i);
        v.AddMember("id", id, d.GetAllocator());
        c.PushBack(v, d.GetAllocator());
    }
    std::string ids = toJson(c);
    s.SetString(ids.c_str(), d.GetAllocator());
    d.AddMember("c", s, d.GetAllocator());
    weapi(r, d);
    auto re = r.send();
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

std::string NeteaseMusicApi::aesEncrypt(std::string text, std::string mode, std::string key, std::string iv, bool base64) {
    const EVP_CIPHER *cipher = nullptr;
    if (!cstr_stricmp(mode.c_str(), "cbc")) {
        cipher = EVP_aes_128_cbc();
    } else if (!cstr_stricmp(mode.c_str(), "ecb")) {
        cipher = EVP_aes_128_ecb();
    } else {
        av_log(nullptr, AV_LOG_ERROR, "unknown mode %s\n", mode.c_str());
        return "";
    }
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::string re;
    int res = 0;
    unsigned char* buf = nullptr;
    int len = text.length() + EVP_MAX_BLOCK_LENGTH;
    int finalLen = 0;
    if (!ctx) {
        av_log(nullptr, AV_LOG_ERROR, "EVP_CIPHER_CTX_new failed\n");
        return "";
    }
    res = EVP_CIPHER_CTX_set_padding(ctx, EVP_PADDING_PKCS7);
    if (res == -1) {
        av_log(nullptr, AV_LOG_ERROR, "EVP_CIPHER_CTX_set_padding failed\n");
        goto end;
    }
    res = EVP_EncryptInit_ex(ctx, cipher, nullptr, (const unsigned char*)key.c_str(), (const unsigned char*)iv.c_str());
    if (res == -1) {
        av_log(nullptr, AV_LOG_ERROR, "EVP_EncryptInit_ex failed\n");
        goto end;
    }
    buf = (unsigned char*)malloc(len);
    if (!buf) {
        av_log(nullptr, AV_LOG_ERROR, "malloc failed\n");
        goto end;
    }
    res = EVP_EncryptUpdate(ctx, buf, &len, (const unsigned char*)text.c_str(), text.length());
    if (res == -1) {
        av_log(nullptr, AV_LOG_ERROR, "EVP_EncryptUpdate failed\n");
        goto end;
    }
    res = EVP_EncryptFinal_ex(ctx, buf + len, &finalLen);
    if (res == -1) {
        av_log(nullptr, AV_LOG_ERROR, "EVP_EncryptFinal_ex failed\n");
        goto end;
    }
    len += finalLen;
    re = std::string((const char*)buf, len);
    if (base64) {
        re = base64Encode(re);
    } else {
        auto tmp = str_util::str_hex(re);
        if (!str_util::touppercase(tmp, re)) {
            av_log(nullptr, AV_LOG_ERROR, "str_util::touppercase failed: Out of memory\n");
            goto end;
        }
    }
end:
    EVP_CIPHER_CTX_free(ctx);
    if (buf) free(buf);
    return re;
}

std::string NeteaseMusicApi::base64Encode(std::string text) {
    BIO* bio = nullptr, *b64 = nullptr;
    BUF_MEM* buf = nullptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, text.c_str(), text.length());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buf);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);
    std::string re(buf->data, buf->length);
    BUF_MEM_free(buf);
    return re;
}

std::string NeteaseMusicApi::rsaEncrypt(std::string text, std::string key) {
    BIO* pub = BIO_new_mem_buf((void*)key.c_str(), key.length());
    if (!pub) {
        av_log(nullptr, AV_LOG_ERROR, "BIO_new failed\n");
        return "";
    }
    RSA* rsa = nullptr;
    std::string re;
    unsigned char* buf = nullptr;
    int buf_len;
    int size;
    const BIGNUM* n = nullptr, *e = nullptr;
    BIGNUM* bn = nullptr, *ren = nullptr;
    BN_CTX* ctx = nullptr;
    PEM_read_bio_RSA_PUBKEY(pub, &rsa, nullptr, nullptr);
    if (!rsa) {
        av_log(nullptr, AV_LOG_ERROR, "PEM_read_bio_RSA_PUBKEY failed\n");
        goto end;
    }
    size = RSA_size(rsa);
    buf = (unsigned char*)malloc(size);
    if (!buf) {
        av_log(nullptr, AV_LOG_ERROR, "malloc failed\n");
        goto end;
    }
    n = RSA_get0_n(rsa);
    e = RSA_get0_e(rsa);
    bn = BN_new();
    if (!bn) {
        av_log(nullptr, AV_LOG_ERROR, "BN_new failed\n");
        goto end;
    }
    ren = BN_new();
    if (!ren) {
        av_log(nullptr, AV_LOG_ERROR, "BN_new failed\n");
        goto end;
    }
    BN_bin2bn((const unsigned char*)text.c_str(), text.length(), bn);
    ctx = BN_CTX_new();
    if (!ctx) {
        av_log(nullptr, AV_LOG_ERROR, "BN_CTX_new failed\n");
        goto end;
    }
    BN_mod_exp(ren, bn, e, n, ctx);
    buf_len = BN_bn2bin(ren, buf);
    re = std::string((const char*)buf, buf_len);
    re = str_util::str_hex(re);
end:
    if (pub) BIO_free(pub);
    if (rsa) RSA_free(rsa);
    if (buf) free(buf);
    if (bn) BN_free(bn);
    if (ren) BN_free(ren);
    if (ctx) BN_CTX_free(ctx);
    return re;
}

const std::regex weapiRegex = std::regex("\\w*api");
static bool randed = false;

void NeteaseMusicApi::weapi(Request& req, rapidjson::Document& d) {
    Value csrf_token;
    csrf_token.SetString("", d.GetAllocator());
    d.AddMember("csrf_token", csrf_token, d.GetAllocator());
    std::string text = toJson(d);
    av_log(nullptr, AV_LOG_DEBUG, "Request params: %s\n", text.c_str());
    char secKey[16] = {0};
    if (!randed) {
        srand(time(nullptr));
        randed = true;
    }
    for (int i = 0; i < 16; i++) {
        secKey[i] = base62[rand() % 62];
    }
    std::string secretKey(secKey, 16);
    text = aesEncrypt(text, "cbc", presetKey, iv);
    if (text.empty()) {
        throw std::runtime_error("aesEncrypt failed first round");
    }
    text = aesEncrypt(text, "cbc", secretKey, iv);
    if (text.empty()) {
        throw std::runtime_error("aesEncrypt failed second round");
    }
    QueryData data;
    data.set("params", text);
    secretKey = std::string(secretKey.rbegin(), secretKey.rend());
    text = rsaEncrypt(secretKey, publicKey);
    if (text.empty()) {
        throw std::runtime_error("rsaEncrypt failed");
    }
    data.set("encSecKey", text);
    req.setBody(new QueryData(data));
    req.path = std::regex_replace(req.path, weapiRegex, "weapi");
}

std::string NeteaseMusicApi::toJson(rapidjson::Document& d) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);
    return buffer.GetString();
}

rapidjson::Document NeteaseMusicApi::fromJson(std::string& s) {
    Document d;
    d.Parse(s.c_str(), s.length());
    return std::move(d);
}
