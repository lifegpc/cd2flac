#pragma once

#include "http_client.h"
#include "rapidjson/document.h"
#include <stdint.h>

#include <list>

class NeteaseMusicApi {
public:
    NeteaseMusicApi(std::string cookiePath);
    rapidjson::Document fetchSongDetail(uint64_t id);
    rapidjson::Document fetchSongDetails(std::list<uint64_t> id);
    rapidjson::Document fetchAlbum(uint64_t id);
    rapidjson::Document fetchLyric(uint64_t id);
    rapidjson::Document fetchLoginStatus();
    rapidjson::Document sentSms(std::string phone, std::string countrycode = "86");
    rapidjson::Document loginWithSms(std::string phone, std::string code, std::string countrycode = "86");
    rapidjson::Document loginRefresh();
    rapidjson::Document fetchSongUrl(std::list<uint64_t> id, int br = 999000);
    rapidjson::Document search(std::string keywords, int64_t type = 1, int64_t limit = 30, int64_t offset = 0);
    std::string aesEncrypt(std::string text, std::string mode, std::string key, std::string iv, bool base64 = true);
    std::string base64Encode(std::string text);
    std::string rsaEncrypt(std::string text, std::string key);
    void weapi(Request& req, rapidjson::Document& d);
    std::string toJson(rapidjson::Document& d);
    rapidjson::Document fromJson(std::string& s);
    void eapi(Request& req, rapidjson::Document& d);
    std::string md5Encode(std::string text);
private:
    HttpClient client = HttpClient("https://music.163.com");
    NetscapeCookies cookies;
    HeaderMap basicHeader;
};
