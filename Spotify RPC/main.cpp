#include <windows.h>
#include <csignal>
#include "RPC/spotify_player.h"

SpotifyPlayer* g_spotify = nullptr;

void cleanup()
{
	if (g_spotify)
	{
		g_spotify->stop();
		delete g_spotify;
		g_spotify = nullptr;
	}
}

void signal_handler(int signal)
{
	std::cout << "\nReceived signal to terminate. Cleaning up..." << std::endl;
	cleanup();
	exit(signal);
}

int main()
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	g_spotify = new SpotifyPlayer();

	if (!g_spotify->start())
	{
		std::cerr << "Failed to initialize Spotify integration" << std::endl;
		cleanup();
		return 1;
	}

	std::cout << "Spotify integration initialized" << std::endl;

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	cleanup();
	return 0;
}