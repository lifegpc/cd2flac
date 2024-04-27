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
#include "time_util.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <regex>
#include <time.h>


const std::string iv = "0102030405060708";
const std::string presetKey = "0CoJUm6Qyw8W8jud";
const std::string linuxapiKey = "rFgB&h#%2?^eDg:Q";
const std::string base62 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const std::string publicKey = "-----BEGIN PUBLIC KEY-----\n\
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDgtQn2JZ34ZC28NWYpAUd98iZ37BUrX/aKzmFbt7clFSs6sXqHauqKWqdtLkF2KexO40H1YTX8z2lSgBBOAxLsvaklV8k4cBFK9snQXE9/DDaFt6Rr7iVZMldczhC0JNgTz+SHXT6CBHuX3e9SdB1Ua44oncaTWz7OBGLbCiK45wIDAQAB\n\
-----END PUBLIC KEY-----";
const std::string eapiKey = "e82ckenh8dichen8";

using namespace rapidjson;

NeteaseMusicApi::NeteaseMusicApi(std::string cookiePath) {
    this->client.headers["User-Agent"] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.1938.39 Safari/537.36";
    this->client.headers["Connection"] = "close";
    this->client.headers["Referer"] = "https://music.163.com/";
    this->basicHeader["User-Agent"] = this->client.headers["User-Agent"];
    this->basicHeader["Connection"] = this->client.headers["Connection"];
    this->basicHeader["Referer"] = this->client.headers["Referer"];
    this->client.cookies = &this->cookies;
    this->cookies.path = cookiePath;
    this->cookies.load();
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
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "fetchSongDetail return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

Document NeteaseMusicApi::fetchAlbum(uint64_t id) {
    auto r = this->client.request("/api/v1/album/" + std::to_string(id), "POST");
    av_log(nullptr, AV_LOG_DEBUG, "POST /api/v1/album/%" PRIu64 "\n", id);
    Document d;
    d.SetObject();
    weapi(r, d);
    auto re = r.send();
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "fetchAlbum return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

Document NeteaseMusicApi::fetchLyric(uint64_t id) {
    auto r = this->client.request("/api/song/lyric?_nmclfl=1", "POST");
    av_log(nullptr, AV_LOG_DEBUG, "POST /api/song/lyric?_nmclfl=1\n");
    QueryData data;
    data.set("id", std::to_string(id));
    data.set("tv", "-1");
    data.set("lv", "-1");
    data.set("rv", "-1");
    data.set("kv", "-1");
    r.setBody(new QueryData(data));
    auto re = r.send();
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "fetchLyric return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

Document NeteaseMusicApi::fetchLoginStatus() {
    auto r = this->client.request("/weapi/w/nuser/account/get", "POST");
    av_log(nullptr, AV_LOG_DEBUG, "POST /weapi/w/nuser/account/get\n");
    Document d;
    d.SetObject();
    weapi(r, d);
    auto re = r.send();
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "fetchLoginStatus return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

Document NeteaseMusicApi::search(std::string keywords, int64_t type, int64_t limit, int64_t offset) {
    if (type == 2000) {
        auto r = this->client.request("/api/search/voice/get", "POST");
        av_log(nullptr, AV_LOG_DEBUG, "POST /api/search/voice/get\n");
        Document d;
        d.SetObject();
        Value keyword;
        keyword.SetString(keywords.c_str(), d.GetAllocator());
        d.AddMember("keyword", keyword, d.GetAllocator());
        Value scene;
        scene.SetString("normal", d.GetAllocator());
        d.AddMember("scene", scene, d.GetAllocator());
        Value dlimit;
        dlimit.SetInt64(limit);
        d.AddMember("limit", dlimit, d.GetAllocator());
        Value doffset;
        doffset.SetInt64(offset);
        d.AddMember("offset", doffset, d.GetAllocator());
        weapi(r, d);
        auto re = r.send();
        av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
        if (re.code != 200) {
            av_log(nullptr, AV_LOG_WARNING, "search return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
        }
        auto re_text = re.readAll();
        av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
        return fromJson(re_text);
    }
    auto r = this->client.request("/api/search/get", "POST");
    av_log(nullptr, AV_LOG_DEBUG, "POST /api/search/get\n");
    Document d;
    d.SetObject();
    Value s;
    s.SetString(keywords.c_str(), d.GetAllocator());
    d.AddMember("s", s, d.GetAllocator());
    Value dtype;
    dtype.SetInt64(type);
    d.AddMember("type", dtype, d.GetAllocator());
    Value dlimit;
    dlimit.SetInt64(limit);
    d.AddMember("limit", dlimit, d.GetAllocator());
    Value doffset;
    doffset.SetInt64(offset);
    d.AddMember("offset", doffset, d.GetAllocator());
    weapi(r, d);
    auto re = r.send();
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "search return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

Document NeteaseMusicApi::sentSms(std::string phone, std::string countrycode) {
    auto r = this->client.request("/api/sms/captcha/sent", "POST");
    av_log(nullptr, AV_LOG_DEBUG, "POST /api/sms/captcha/sent\n");
    Document d;
    d.SetObject();
    Value phoneValue;
    phoneValue.SetString(phone.c_str(), d.GetAllocator());
    d.AddMember("cellphone", phoneValue, d.GetAllocator());
    Value countrycodeValue;
    if (countrycode.find("+") == 0) {
        countrycode = countrycode.substr(1);
    }
    countrycodeValue.SetString(countrycode.c_str(), d.GetAllocator());
    d.AddMember("ctcode", countrycodeValue, d.GetAllocator());
    weapi(r, d);
    auto re = r.send();
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "sentSms return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

Document NeteaseMusicApi::loginWithSms(std::string phone, std::string code, std::string countrycode) {
    auto r = this->client.request("/api/login/cellphone", "POST");
    av_log(nullptr, AV_LOG_DEBUG, "POST /api/login/cellphone\n");
    Document d;
    d.SetObject();
    Value phoneValue;
    phoneValue.SetString(phone.c_str(), d.GetAllocator());
    d.AddMember("phone", phoneValue, d.GetAllocator());
    Value countrycodeValue;
    if (countrycode.find("+") == 0) {
        countrycode = countrycode.substr(1);
    }
    countrycodeValue.SetString(countrycode.c_str(), d.GetAllocator());
    d.AddMember("countrycode", countrycodeValue, d.GetAllocator());
    Value codeValue;
    codeValue.SetString(code.c_str(), d.GetAllocator());
    d.AddMember("captcha", codeValue, d.GetAllocator());
    Value rememberLoginValue;
    rememberLoginValue.SetString("true", d.GetAllocator());
    d.AddMember("rememberLogin", rememberLoginValue, d.GetAllocator());
    weapi(r, d);
    r.options.use_custom_cookie = true;
    auto cookie = parseCookie(this->cookies.getCookieHeader(r.host, r.path, r.https));
    cookie["os"] = "ios";
    cookie["appver"] = "8.20.21";
    r.headers["Cookie"] = dumpCookie(cookie);
    auto re = r.send();
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "loginWithSms return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

Document NeteaseMusicApi::loginRefresh() {
    auto r = this->client.request("/weapi/login/token/refresh", "POST");
    av_log(nullptr, AV_LOG_DEBUG, "POST /weapi/login/token/refresh\n");
    Document d;
    d.SetObject();
    weapi(r, d);
    auto re = r.send();
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "loginRefresh return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

Document NeteaseMusicApi::fetchSongUrl(std::list<uint64_t> id, int br) {
    auto r = Request("https://interface3.music.163.com/api/song/enhance/player/url", "POST", this->client.options, this->basicHeader);
    av_log(nullptr, AV_LOG_DEBUG, "POST /api/song/enhance/player/url\n");
    Document d;
    d.SetObject();
    Value ids;
    ids.SetArray();
    for (auto &i: id) {
        Value v;
        v.SetUint64(i);
        ids.PushBack(v, d.GetAllocator());
    }
    d.AddMember("ids", ids, d.GetAllocator());
    Value brValue;
    brValue.SetInt(br);
    d.AddMember("br", brValue, d.GetAllocator());
    eapi(r, d);
    auto re = r.send();
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "fetchSongUrl return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
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
    EVP_PKEY* rsa = nullptr;
    std::string re;
    unsigned char* buf = nullptr;
    int buf_len;
    int size;
    BIGNUM* n = nullptr, *e = nullptr, *bn = nullptr, *ren = nullptr;
    BN_CTX* ctx = nullptr;
    PEM_read_bio_PUBKEY(pub, &rsa, nullptr, nullptr);
    if (!rsa) {
        av_log(nullptr, AV_LOG_ERROR, "PEM_read_bio_RSA_PUBKEY failed\n");
        goto end;
    }
    size = EVP_PKEY_get_size(rsa);
    buf = (unsigned char*)malloc(size);
    if (!buf) {
        av_log(nullptr, AV_LOG_ERROR, "malloc failed\n");
        goto end;
    }
    EVP_PKEY_get_bn_param(rsa, "n", &n);
    EVP_PKEY_get_bn_param(rsa, "e", &e);
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
    if (rsa) EVP_PKEY_free(rsa);
    if (buf) free(buf);
    if (bn) BN_free(bn);
    if (n) BN_free(n);
    if (e) BN_free(e);
    if (ren) BN_free(ren);
    if (ctx) BN_CTX_free(ctx);
    return re;
}

const std::regex weapiRegex = std::regex("\\w*api");
static bool randed = false;

void NeteaseMusicApi::weapi(Request& req, rapidjson::Document& d) {
    Value csrf_token;
    std::string csrf;
    auto cookie = parseCookie(this->cookies.getCookieHeader(req.host, req.path, req.https));
    if (cookie.find("__csrf") != cookie.end()) {
        csrf = cookie["__csrf"];
    }
    csrf_token.SetString(csrf.c_str(), d.GetAllocator());
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

void NeteaseMusicApi::eapi(Request& req, Document& d) {
    auto cookie = parseCookie(this->cookies.getCookieHeader(req.host, req.path, req.https));
    Value header;
    std::map<std::string, std::string> headerMap;
    header.SetObject();
    if (cookie.find("osver") != cookie.end()) {
        headerMap["osver"] = cookie["osver"];
    } else {
        headerMap["osver"] = "17,1,2";
    }
    if (cookie.find("deviceId") != cookie.end()) {
        headerMap["deviceId"] = cookie["deviceId"];
    }
    if (cookie.find("appver") != cookie.end()) {
        headerMap["appver"] = cookie["appver"];
    } else {
        headerMap["appver"] = "8.9.70";
    }
    if (cookie.find("versioncode") != cookie.end()) {
        headerMap["versioncode"] = cookie["versioncode"];
    } else {
        headerMap["versioncode"] = "140";
    }
    if (cookie.find("mobilename") != cookie.end()) {
        headerMap["mobilename"] = cookie["mobilename"];
    }
    if (cookie.find("buildver") != cookie.end()) {
        headerMap["buildver"] = cookie["buildver"];
    } else {
        headerMap["buildver"] = std::to_string(time_util::time_ns() / 1000000LL).substr(0, 10);
    }
    if (cookie.find("resolution") != cookie.end()) {
        headerMap["resolution"] = cookie["resolution"];
    } else {
        headerMap["resolution"] = "1920x1080";
    }
    if (cookie.find("__csrf") != cookie.end()) {
        headerMap["__csrf"] = cookie["__csrf"];
    } else {
        headerMap["__csrf"] = "";
    }
    if (cookie.find("os") != cookie.end()) {
        headerMap["os"] = cookie["os"];
    } else {
        headerMap["os"] = "ios";
    }
    if (cookie.find("channel") != cookie.end()) {
        headerMap["channel"] = cookie["channel"];
    }
    if (!randed) {
        srand(time(nullptr));
        randed = true;
    }
    char ran[5] = { 0 };
    int rannum = rand() % 1000;
    snprintf(ran, 5, "%04d", rannum);
    std::string t = std::to_string(time_util::time_ns() / 1000000LL) + "_" + std::string(ran, 4);
    headerMap["requestId"] = t;
    if (cookie.find("MUSIC_U") != cookie.end()) {
        headerMap["MUSIC_U"] = cookie["MUSIC_U"];
    }
    if (cookie.find("MUSIC_A") != cookie.end()) {
        headerMap["MUSIC_A"] = cookie["MUSIC_A"];
    }
    req.options.use_custom_cookie = true;
    for (auto &i: headerMap) {
        Value v;
        v.SetString(i.second.c_str(), d.GetAllocator());
        Value k;
        k.SetString(i.first.c_str(), d.GetAllocator());
        header.AddMember(k, v, d.GetAllocator());
    }
    req.headers["Cookie"] = dumpCookie(headerMap);
    d.AddMember("header", header, d.GetAllocator());
    auto text = toJson(d);
    av_log(nullptr, AV_LOG_DEBUG, "Request params: %s\n", text.c_str());
    auto url = std::regex_replace(req.path, weapiRegex, "api");
    std::string message = "nobody" + url + "use" + text + "md5forencrypt";
    auto digest = md5Encode(message);
    if (digest.empty()) {
        throw std::runtime_error("md5Encode failed");
    }
    digest = str_util::str_hex(digest);
    std::string data = url + "-36cd479b6b5-" + text + "-36cd479b6b5-" + digest;
    std::string param = aesEncrypt(data, "ecb", eapiKey, "", false);
    if (param.empty()) {
        throw std::runtime_error("aesEncrypt failed");
    }
    QueryData query;
    query.set("params", param);
    req.setBody(new QueryData(query));
    req.path = std::regex_replace(req.path, weapiRegex, "eapi");
}

std::string NeteaseMusicApi::md5Encode(std::string text) {
    const EVP_MD* md5 = EVP_md5();
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    std::string re;
    int res = 0;
    unsigned char buf[16];
    unsigned int out_len = 0;
    if (!ctx) {
        av_log(nullptr, AV_LOG_ERROR, "EVP_MD_CTX_new failed\n");
        return "";
    }
    if (!EVP_DigestInit(ctx, md5)) {
        av_log(nullptr, AV_LOG_ERROR, "EVP_DigestInit failed\n");
        goto end;
    }
    if (!EVP_DigestUpdate(ctx, text.c_str(), text.length())) {
        av_log(nullptr, AV_LOG_ERROR, "EVP_DigestUpdate failed\n");
        goto end;
    }
    if (!EVP_DigestFinal(ctx, buf, &out_len)) {
        av_log(nullptr, AV_LOG_ERROR, "EVP_DigestFinal failed\n");
        goto end;
    }
    re = std::string((const char*)buf, out_len);
end:
    EVP_MD_CTX_free(ctx);
    return re;
}
