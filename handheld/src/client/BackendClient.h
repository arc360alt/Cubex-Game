#pragma once
#ifndef STANDALONE_SERVER

#include <string>
#include <vector>

#if defined(__ANDROID__) || defined(_WIN32)

class BackendClient {
public:
    struct Response {
        std::string body;
        long status = 0;
    };
    static Response post(const std::string&, const std::string&) { return {}; }
    static Response get(const std::string&) { return {}; }
    static std::string registerUser(const std::string&, const std::string&, const std::string&, std::string&) { return ""; }
    static std::string login(const std::string&, const std::string&, const std::string&, std::string&) { return ""; }
    static std::string verifyToken(const std::string&, const std::string&, std::string&) { return ""; }
    static std::string changeUsername(const std::string&, const std::string&, const std::string&) { return ""; }
    static bool uploadSkin(const std::string&, const std::string&, const std::string&) { return false; }
    static bool downloadSkin(const std::string&, const std::string&, const std::string&) { return false; }
    static std::vector<std::string> listCapes(const std::string&) { return {}; }
    static bool downloadCape(const std::string&, const std::string&, const std::string&) { return false; }
    static bool setCape(const std::string&, const std::string&, const std::string&, std::string&) { return false; }
    static std::string getPlayerCape(const std::string&, const std::string&) { return ""; }
    static std::string resolveCapeTexturePath(const std::string&, const std::string&) { return ""; }
};

#else

#include <curl/curl.h>
#include <fstream>
#include <unistd.h>

class BackendClient {
public:
    struct Response {
        std::string body;
        long status = 0;
    };

    static Response post(const std::string& url, const std::string& body) {
        Response res;
        CURL* curl = curl_easy_init();
        if (!curl) return res;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return res;
    }

    static Response get(const std::string& url) {
        Response res;
        CURL* curl = curl_easy_init();
        if (!curl) return res;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status);
        curl_easy_cleanup(curl);
        return res;
    }

    // Same as get(), but with an "Authorization: Bearer <token>" header — the
    // account backend's token-authenticated endpoints (e.g. /api/verify) read
    // the token from that header, not from the query string.
    static Response getAuthed(const std::string& url, const std::string& token) {
        Response res;
        CURL* curl = curl_easy_init();
        if (!curl) return res;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        struct curl_slist* headers = nullptr;
        std::string authHeader = "Authorization: Bearer " + token;
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return res;
    }

    // Returns token on success (and sets outUuid to the account's stable,
    // never-changing id), empty on failure.
    static std::string registerUser(const std::string& baseUrl,
                                    const std::string& username,
                                    const std::string& password,
                                    std::string& outUuid) {
        std::string body = "{\"username\":\"" + escapeJson(username) +
                           "\",\"password\":\"" + escapeJson(password) + "\"}";
        auto res = post(baseUrl + "/api/register", body);
        if (res.status == 200 || res.status == 201) {
            outUuid = extractJsonString(res.body, "uuid");
            return extractJsonString(res.body, "token");
        }
        return "";
    }

    // Returns token on success (and sets outUuid), empty on failure.
    static std::string login(const std::string& baseUrl,
                             const std::string& username,
                             const std::string& password,
                             std::string& outUuid) {
        std::string body = "{\"username\":\"" + escapeJson(username) +
                           "\",\"password\":\"" + escapeJson(password) + "\"}";
        auto res = post(baseUrl + "/api/login", body);
        if (res.status == 200) {
            outUuid = extractJsonString(res.body, "uuid");
            return extractJsonString(res.body, "token");
        }
        return "";
    }

    // Returns verified username on success (and sets outUuid), empty on
    // failure. Also used to lazily fetch a uuid for accounts that logged in
    // before uuids existed — the backend backfills one on any verify call.
    static std::string verifyToken(const std::string& baseUrl, const std::string& token, std::string& outUuid) {
        auto res = getAuthed(baseUrl + "/api/verify", token);
        if (res.status == 200) {
            outUuid = extractJsonString(res.body, "uuid");
            return extractJsonString(res.body, "username");
        }
        return "";
    }

    // Same as post(), but issues a PUT with an Authorization header instead of
    // a POST — the backend's PUT-only, token-authenticated endpoints (change
    // username, set cape) need both to actually match and authenticate.
    static Response putAuthed(const std::string& url, const std::string& token, const std::string& body) {
        Response res;
        CURL* curl = curl_easy_init();
        if (!curl) return res;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string authHeader = "Authorization: Bearer " + token;
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return res;
    }

    // Returns new username on success, empty on failure
    static std::string changeUsername(const std::string& baseUrl,
                                      const std::string& token,
                                      const std::string& newUsername) {
        std::string body = "{\"username\":\"" + escapeJson(newUsername) + "\"}";
        // Was previously sent as a POST with the token embedded in the body —
        // the endpoint is PUT-only and reads the token from the Authorization
        // header (see _get_user_from_token() in backend/server.py), so this
        // never actually authenticated.
        auto res = putAuthed(baseUrl + "/api/username", token, body);
        if (res.status == 200)
            return extractJsonString(res.body, "username");
        return "";
    }

    // Names of all admin-provided capes (e.g. "creeper", "red"). Empty vector
    // on failure or if none have been added yet.
    static std::vector<std::string> listCapes(const std::string& baseUrl) {
        std::vector<std::string> names;
        auto res = get(baseUrl + "/api/capes");
        if (res.status != 200) return names;

        // {"capes": ["a", "b", ...]} — our own backend's output, safe to parse
        // with simple bracket/quote scanning rather than a full JSON parser.
        size_t start = res.body.find('[');
        size_t end = (start == std::string::npos) ? std::string::npos : res.body.find(']', start);
        if (start == std::string::npos || end == std::string::npos) return names;

        size_t i = start + 1;
        while (i < end) {
            size_t q1 = res.body.find('"', i);
            if (q1 == std::string::npos || q1 >= end) break;
            size_t q2 = res.body.find('"', q1 + 1);
            if (q2 == std::string::npos || q2 >= end) break;
            names.push_back(res.body.substr(q1 + 1, q2 - q1 - 1));
            i = q2 + 1;
        }
        return names;
    }

    // Downloads a named cape's PNG to destPath; returns true on success.
    static bool downloadCape(const std::string& baseUrl,
                             const std::string& capeName,
                             const std::string& destPath) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        std::string url = baseUrl + "/api/capes/" + urlEncode(capeName);
        std::ofstream file(destPath, std::ios::binary);
        if (!file.is_open()) { curl_easy_cleanup(curl); return false; }
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_perform(curl);
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_easy_cleanup(curl);
        file.close();
        if (status != 200) {
            remove(destPath.c_str());
            return false;
        }
        return true;
    }

    // Sets the authenticated account's selected cape (capeName == "" means
    // none). Returns true on success and sets outCape to the resulting value
    // (kept as an out-param, rather than "" meaning failure, since "" is also
    // the valid/successful result of picking "no cape").
    static bool setCape(const std::string& baseUrl,
                        const std::string& token,
                        const std::string& capeName,
                        std::string& outCape) {
        std::string body = "{\"cape\":\"" + escapeJson(capeName) + "\"}";
        auto res = putAuthed(baseUrl + "/api/cape", token, body);
        if (res.status == 200) {
            outCape = extractJsonString(res.body, "cape");
            return true;
        }
        return false;
    }

    // The cape name a given username has selected ("" if none/unknown).
    static std::string getPlayerCape(const std::string& baseUrl, const std::string& username) {
        auto res = get(baseUrl + "/api/cape/" + urlEncode(username));
        if (res.status != 200) return "";
        return extractJsonString(res.body, "cape");
    }

    // Resolves a cape name to a local, ready-to-bindTexture file path,
    // downloading and caching it from the backend the first time it's needed.
    // Returns "" if capeName is empty (no cape) or the download fails.
    static std::string resolveCapeTexturePath(const std::string& baseUrl, const std::string& capeName) {
        if (capeName.empty()) return "";
        std::string path = "/tmp/mcpe_cape_" + capeName + ".png";
        if (access(path.c_str(), F_OK) == 0) return path;
        if (downloadCape(baseUrl, capeName, path)) return path;
        return "";
    }

    // Returns true on success
    static bool uploadSkin(const std::string& baseUrl,
                           const std::string& token,
                           const std::string& filePath) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        std::string url = baseUrl + "/api/skin";
        curl_mime* form = curl_mime_init(curl);
        curl_mimepart* part = curl_mime_addpart(form);
        curl_mime_name(part, "skin");
        curl_mime_filedata(part, filePath.c_str());
        curl_mime_type(part, "image/png");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        struct curl_slist* headers = nullptr;
        std::string authHeader = "Authorization: Bearer " + token;
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        std::string body;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_perform(curl);
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_slist_free_all(headers);
        curl_mime_free(form);
        curl_easy_cleanup(curl);
        return status == 200;
    }

    // Downloads skin PNG to destPath; returns true on success
    static bool downloadSkin(const std::string& baseUrl,
                             const std::string& username,
                             const std::string& destPath) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        std::string url = baseUrl + "/api/skin/" + urlEncode(username);
        std::ofstream file(destPath, std::ios::binary);
        if (!file.is_open()) { curl_easy_cleanup(curl); return false; }
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_perform(curl);
        long status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_easy_cleanup(curl);
        file.close();
        if (status != 200) {
            remove(destPath.c_str());
            return false;
        }
        return true;
    }

private:
    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* str = static_cast<std::string*>(userdata);
        str->append(ptr, size * nmemb);
        return size * nmemb;
    }

    static size_t writeFileCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* f = static_cast<std::ofstream*>(userdata);
        f->write(ptr, size * nmemb);
        return size * nmemb;
    }

    static std::string extractJsonString(const std::string& json, const std::string& key) {
        std::string needle = "\"" + key + "\":\"";
        size_t pos = json.find(needle);
        if (pos == std::string::npos) return "";
        pos += needle.size();
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    }

    static std::string escapeJson(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else out += c;
        }
        return out;
    }

    static std::string urlEncode(const std::string& s) {
        CURL* curl = curl_easy_init();
        if (!curl) return s;
        char* encoded = curl_easy_escape(curl, s.c_str(), (int)s.size());
        std::string result = encoded ? encoded : s;
        if (encoded) curl_free(encoded);
        curl_easy_cleanup(curl);
        return result;
    }
};

#endif // __ANDROID__ || _WIN32

#endif // STANDALONE_SERVER
