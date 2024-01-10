#pragma once

#include "http_client.h"
#include "rapidjson/document.h"
#include <stdint.h>

#include <list>

class NeteaseMusicApi {
public:
    NeteaseMusicApi();
    rapidjson::Document fetchSongDetail(uint64_t id);
    rapidjson::Document fetchSongDetails(std::list<uint64_t> id);
    rapidjson::Document fetchAlbum(uint64_t id);
    std::string aesEncrypt(std::string text, std::string mode, std::string key, std::string iv, bool base64 = true);
    std::string base64Encode(std::string text);
    std::string rsaEncrypt(std::string text, std::string key);
    void weapi(Request& req, rapidjson::Document& d);
    std::string toJson(rapidjson::Document& d);
    rapidjson::Document fromJson(std::string& s);
private:
    HttpClient client = HttpClient("https://music.163.com");
};
