#include "spotify_integration.h"
#include "spotify_auth.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>

class SpotifyPlayer
{
private:
    std::atomic<bool> running;
    std::thread monitor_thread;
    SpotifyAPI* spotify;

    void monitorLoop()
    {
        std::string last_track;

        while (running)
        {
            auto track = spotify->getCurrentlyPlaying();

            if (track.is_playing)
            {
                std::string current_track = track.name + " - " + track.artist;
                if (current_track != last_track)
                {
                    system("cls");
                    std::cout << "Spotify Now Playing Monitor" << std::endl;
                    std::cout << "-------------------------" << std::endl;
                    std::cout << "Track: " << track.name << std::endl;
                    std::cout << "Artist: " << track.artist << std::endl;
                    std::cout << "Album: " << track.album << std::endl;
                    std::cout << "Status: Playing" << std::endl;
                    last_track = current_track;
                }
            }
            else
            {
                if (last_track != "paused")
                {
                    system("cls");
                    std::cout << "Spotify Now Playing Monitor" << std::endl;
                    std::cout << "-------------------------" << std::endl;
                    std::cout << "Status: No track playing" << std::endl;
                    last_track = "paused";
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

public:
    SpotifyPlayer() : running(false), spotify(nullptr) {}

    ~SpotifyPlayer()
    {
        stop();
        if (spotify)
        {
            delete spotify;
        }
    }

    bool start()
    {
        try
        {
            std::cout << "Spotify Now Playing Monitor" << std::endl;
            std::cout << "-------------------------" << std::endl;
            std::cout << "Authenticating with Spotify..." << std::endl;

            SpotifyAuth auth;
            json tokens = auth.authenticate();

            if (tokens.empty() || !tokens.contains("access_token") || !tokens.contains("refresh_token"))
            {
                std::cerr << "Failed to authenticate with Spotify" << std::endl;
                return false;
            }

            spotify = new SpotifyAPI(tokens["access_token"], tokens["refresh_token"]);

            std::cout << "Successfully authenticated!" << std::endl;
            std::cout << "Monitoring your Spotify playback..." << std::endl;
            std::cout << "-------------------------" << std::endl;

            running = true;
            monitor_thread = std::thread(&SpotifyPlayer::monitorLoop, this);
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    void stop()
    {
        if (running)
        {
            running = false;
            if (monitor_thread.joinable())
            {
                monitor_thread.join();
            }
        }
    }
};