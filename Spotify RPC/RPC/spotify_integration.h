#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>

using json = nlohmann::json;

class SpotifyAPI
{
private:
    std::string access_token;
    std::string refresh_token;
    std::chrono::system_clock::time_point token_expiry;

    std::wstring string_to_wide(const std::string& str)
    {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    bool refreshAccessToken()
    {
        HINTERNET hSession = WinHttpOpen(L"Spotify Monitor/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession,
            L"accounts.spotify.com",
            INTERNET_DEFAULT_HTTPS_PORT,
            0);

        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return false;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect,
            L"POST",
            L"/api/token",
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        std::string post_data = "grant_type=refresh_token&refresh_token=" + refresh_token;

        WinHttpAddRequestHeaders(hRequest,
            L"Content-Type: application/x-www-form-urlencoded",
            -1L,
            WINHTTP_ADDREQ_FLAG_ADD);

        if (!WinHttpSendRequest(hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            (LPVOID)post_data.c_str(),
            post_data.length(),
            post_data.length(),
            0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        if (!WinHttpReceiveResponse(hRequest, NULL))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        std::string response;
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        char* pszOutBuffer;
        
        do
        {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;

            pszOutBuffer = new char[dwSize + 1];
            ZeroMemory(pszOutBuffer, dwSize + 1);

            if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
            {
                delete[] pszOutBuffer;
                break;
            }

            response.append(pszOutBuffer, dwDownloaded);
            delete[] pszOutBuffer;
        } while (dwSize > 0);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        try
        {
            json j = json::parse(response);
            access_token = j["access_token"];
            token_expiry = std::chrono::system_clock::now() + std::chrono::seconds(3600);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

public:
    struct CurrentTrack
    {
        std::string name;
        std::string artist;
        std::string album;
        bool is_playing;
    };

    SpotifyAPI(const std::string& access_token, const std::string& refresh_token)
        : access_token(access_token), refresh_token(refresh_token)
    {
        token_expiry = std::chrono::system_clock::now() + std::chrono::seconds(3600);
    }

    CurrentTrack getCurrentlyPlaying()
    {
        if (std::chrono::system_clock::now() >= token_expiry)
        {
            if (!refreshAccessToken())
            {
                return {"Error", "Failed to refresh token", "", false};
            }
        }

        HINTERNET hSession = WinHttpOpen(L"Spotify Monitor/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        CurrentTrack track = {"No track playing", "", "", false};

        if (!hSession) return track;

        HINTERNET hConnect = WinHttpConnect(hSession,
            L"api.spotify.com",
            INTERNET_DEFAULT_HTTPS_PORT,
            0);

        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            return track;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect,
            L"GET",
            L"/v1/me/player/currently-playing",
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return track;
        }

        std::wstring auth_header = L"Authorization: Bearer " + string_to_wide(access_token);
        WinHttpAddRequestHeaders(hRequest,
            auth_header.c_str(),
            -1L,
            WINHTTP_ADDREQ_FLAG_ADD);

        if (!WinHttpSendRequest(hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return track;
        }

        if (!WinHttpReceiveResponse(hRequest, NULL))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return track;
        }

        std::string response;
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        char* pszOutBuffer;
        
        do
        {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;

            pszOutBuffer = new char[dwSize + 1];
            ZeroMemory(pszOutBuffer, dwSize + 1);

            if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
            {
                delete[] pszOutBuffer;
                break;
            }

            response.append(pszOutBuffer, dwDownloaded);
            delete[] pszOutBuffer;
        } while (dwSize > 0);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (!response.empty())
        {
            try
            {
                json j = json::parse(response);
                if (j.contains("item") && !j["item"].is_null())
                {
                    track.name = j["item"]["name"];
                    track.artist = j["item"]["artists"][0]["name"];
                    track.album = j["item"]["album"]["name"];
                    track.is_playing = j["is_playing"];
                }
            }
            catch (...)
            {
                track.name = "Error parsing response";
            }
        }

        return track;
    }
}; 