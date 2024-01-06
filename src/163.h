#pragma once

#include "http_client.h"
#include "rapidjson/document.h"

class NeteaseMusicApi {
public:
    NeteaseMusicApi();
    void fetchSongDetail(int id);
    std::string aesEncrypt(std::string text, std::string mode, std::string key, std::string iv, bool base64 = true);
    std::string base64Encode(std::string text);
    std::string rsaEncrypt(std::string text, std::string key);
    void weapi(Request& req, rapidjson::Document& d);
private:
    HttpClient client = HttpClient("music.163.com");
};
