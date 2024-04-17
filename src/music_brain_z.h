#pragma once

#include "http_client.h"
#include "rapidjson/document.h"
#include <stdint.h>
#include <list>

class MusicBrainZApi {
public:
    MusicBrainZApi();
    rapidjson::Document lookup(std::string entity_type, std::string mbid, std::list<std::string> inc);
    std::string toJson(rapidjson::Document& d);
    rapidjson::Document fromJson(std::string& s);
private:
    HttpClient client = HttpClient("https://musicbrainz.org");
};
