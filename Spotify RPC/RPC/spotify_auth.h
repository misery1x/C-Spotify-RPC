#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <random>

using json = nlohmann::json;

class SpotifyAuth
{
private:
    const std::string client_id = ""; // Developer client ID
    const std::string client_secret = ""; // Developer client secret here
    const std::string redirect_uri = "http://localhost:8888/callback";
    const std::string auth_endpoint = "https://accounts.spotify.com/authorize";
    const std::string token_endpoint = "https://accounts.spotify.com/api/token";
    const std::string scope = "user-read-currently-playing user-read-playback-state";
    
    std::string generateState(int length = 16)
    {
        const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, chars.size() - 1);
        
        std::string state;
        for (int i = 0; i < length; ++i)
        {
            state += chars[distribution(generator)];
        }
        return state;
    }

    SOCKET createServerSocket()
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            throw std::runtime_error("WSAStartup failed");
        }

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET)
        {
            WSACleanup();
            throw std::runtime_error("Failed to create socket");
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(8888);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            closesocket(serverSocket);
            WSACleanup();
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(serverSocket, 1) == SOCKET_ERROR)
        {
            closesocket(serverSocket);
            WSACleanup();
            throw std::runtime_error("Failed to listen on socket");
        }

        return serverSocket;
    }

    std::string waitForAuthCode(SOCKET serverSocket)
    {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
        {
            closesocket(serverSocket);
            WSACleanup();
            throw std::runtime_error("Failed to accept connection");
        }

        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == SOCKET_ERROR)
        {
            closesocket(clientSocket);
            closesocket(serverSocket);
            WSACleanup();
            throw std::runtime_error("Failed to receive data");
        }

        const char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            "<html><body><h1>Authentication successful!</h1>"
            "<p>You can close this window and return to the application.</p></body></html>";
        send(clientSocket, response, strlen(response), 0);

        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();

        std::string request(buffer, bytesReceived);
        size_t codeStart = request.find("code=");
        if (codeStart == std::string::npos)
        {
            throw std::runtime_error("No authorization code found in response");
        }
        codeStart += 5;
        size_t codeEnd = request.find("&", codeStart);
        if (codeEnd == std::string::npos)
        {
            codeEnd = request.find(" ", codeStart);
        }
        return request.substr(codeStart, codeEnd - codeStart);
    }

    json exchangeCodeForTokens(const std::string& code)
    {
        HINTERNET hSession = WinHttpOpen(L"Spotify Auth/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (!hSession) throw std::runtime_error("Failed to create WinHTTP session");

        HINTERNET hConnect = WinHttpConnect(hSession,
            L"accounts.spotify.com",
            INTERNET_DEFAULT_HTTPS_PORT,
            0);

        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            throw std::runtime_error("Failed to connect to Spotify");
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect,
            L"POST",
            L"/api/token",
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);

        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            throw std::runtime_error("Failed to create request");
        }

        std::string post_data = "grant_type=authorization_code"
            "&code=" + code +
            "&redirect_uri=" + redirect_uri +
            "&client_id=" + client_id +
            "&client_secret=" + client_secret;

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
            throw std::runtime_error("Failed to send request");
        }

        if (!WinHttpReceiveResponse(hRequest, NULL))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            throw std::runtime_error("Failed to receive response");
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

        return json::parse(response);
    }

    void saveTokens(const json& tokens)
    {
        std::filesystem::path appdata = std::getenv("APPDATA");
        std::filesystem::path tokenPath = appdata / "SpotifyMonitor";
        
        if (!std::filesystem::exists(tokenPath))
        {
            std::filesystem::create_directory(tokenPath);
        }

        std::ofstream tokenFile(tokenPath / "tokens.json");
        tokenFile << tokens.dump(4);
    }

public:
    json loadSavedTokens()
    {
        std::filesystem::path appdata = std::getenv("APPDATA");
        std::filesystem::path tokenFile = appdata / "SpotifyMonitor" / "tokens.json";
        
        if (!std::filesystem::exists(tokenFile))
        {
            return json();
        }

        std::ifstream file(tokenFile);
        return json::parse(file);
    }

    json authenticate()
    {
        json savedTokens = loadSavedTokens();
        if (!savedTokens.empty())
        {
            return savedTokens;
        }

        std::string state = generateState();

        std::string authUrl = auth_endpoint +
            "?client_id=" + client_id +
            "&response_type=code" +
            "&redirect_uri=" + redirect_uri +
            "&state=" + state +
            "&scope=" + scope;

        ShellExecuteA(NULL, "open", authUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);

        SOCKET serverSocket = createServerSocket();

        std::string code = waitForAuthCode(serverSocket);

        json tokens = exchangeCodeForTokens(code);

        saveTokens(tokens);

        return tokens;
    }
}; 
