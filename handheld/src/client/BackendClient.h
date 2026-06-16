#pragma once
#ifndef STANDALONE_SERVER

#include <string>
#include <curl/curl.h>
#include <fstream>

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

    // Returns token on success, empty on failure
    static std::string registerUser(const std::string& baseUrl,
                                    const std::string& username,
                                    const std::string& password) {
        std::string body = "{\"username\":\"" + escapeJson(username) +
                           "\",\"password\":\"" + escapeJson(password) + "\"}";
        auto res = post(baseUrl + "/api/register", body);
        if (res.status == 200 || res.status == 201)
            return extractJsonString(res.body, "token");
        return "";
    }

    // Returns token on success, empty on failure
    static std::string login(const std::string& baseUrl,
                             const std::string& username,
                             const std::string& password) {
        std::string body = "{\"username\":\"" + escapeJson(username) +
                           "\",\"password\":\"" + escapeJson(password) + "\"}";
        auto res = post(baseUrl + "/api/login", body);
        if (res.status == 200)
            return extractJsonString(res.body, "token");
        return "";
    }

    // Returns verified username on success, empty on failure
    static std::string verifyToken(const std::string& baseUrl, const std::string& token) {
        auto res = get(baseUrl + "/api/verify?token=" + urlEncode(token));
        if (res.status == 200)
            return extractJsonString(res.body, "username");
        return "";
    }

    // Returns new username on success, empty on failure
    static std::string changeUsername(const std::string& baseUrl,
                                      const std::string& token,
                                      const std::string& newUsername) {
        std::string body = "{\"token\":\"" + escapeJson(token) +
                           "\",\"new_username\":\"" + escapeJson(newUsername) + "\"}";
        auto res = post(baseUrl + "/api/username", body);
        if (res.status == 200)
            return extractJsonString(res.body, "username");
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

#endif // STANDALONE_SERVER
