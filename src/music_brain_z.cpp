#include "music_brain_z.h"
#include "cd2flac_version.h"
#include "str_util.h"
extern "C" {
#include "libavutil/avutil.h"
}
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace rapidjson;

MusicBrainZApi::MusicBrainZApi() {
    this->client.headers["Accept"] = "application/json";
    this->client.headers["Connection"] = "close";
    this->client.headers["User-Agent"] = "cd2flac/" CD2FLAC_VERSION " (https://github.com/lifegpc/cd2flac)";
}

Document MusicBrainZApi::lookup(std::string entity_type, std::string mbid, std::list<std::string> inc) {
    std::string url = "/ws/2/" + entity_type + "/" + mbid;
    if (inc.size()) {
        QueryData data;
        data.set("inc", str_util::str_join(inc, " "));
        url += "?" + data.toQuery();
    }
    auto r = client.request(url, "GET");
    av_log(nullptr, AV_LOG_DEBUG, "GET %s\n", url.c_str());
    auto re = r.send();
    av_log(nullptr, AV_LOG_DEBUG, "HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    if (re.code != 200) {
        av_log(nullptr, AV_LOG_WARNING, "lookup return HTTP %" PRIu16 " %s\n", re.code, re.reason.c_str());
    }
    auto re_text = re.readAll();
    av_log(nullptr, AV_LOG_DEBUG, "Response: %s\n", re_text.c_str());
    return fromJson(re_text);
}

Document MusicBrainZApi::fromJson(std::string& s) {
    Document d;
    d.Parse(s.c_str(), s.length());
    return std::move(d);
}

std::string MusicBrainZApi::toJson(Document& d) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);
    return buffer.GetString();
}
