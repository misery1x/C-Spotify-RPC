# Spotify RPC Monitor

A real-time Spotify playback monitor that displays your currently playing track information in a clean console interface.

## Features

- 🎵 Real-time track monitoring
- 👤 Secure Spotify authentication
- 🎨 Clean console interface
- 🔄 Automatic playback status updates
- ⚡ Low resource usage
- 🛑 Graceful shutdown handling

## Prerequisites

- Windows operating system
- Visual Studio 2019 or later
- [Spotify Premium Account](https://www.spotify.com/premium/)
- [Spotify Developer Account](https://developer.spotify.com/dashboard)

## Setup

1. Clone the repository:
```bash
git clone https://github.com/yourusername/spotify-rpc.git
cd spotify-rpc
```

2. Create a Spotify Application:
   - Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
   - Create a new application
   - Add `http://localhost:8888/callback` to the Redirect URIs
   - Note down your Client ID and Client Secret

3. Configure the application:
   - Open `RPC/spotify_auth.h`
   - Replace `YOUR_CLIENT_ID` with your Spotify application's Client ID
   - Replace `YOUR_CLIENT_SECRET` with your Spotify application's Client Secret

4. Build the project:
   - Open the solution in Visual Studio
   - Select Release configuration
   - Build the solution (Ctrl + Shift + B)

## Usage

1. Run the compiled executable
2. First-time usage will open your browser for Spotify authentication
3. Grant the requested permissions
4. The console will display your currently playing track information
5. Press Ctrl+C to exit cleanly

## Features in Detail

### Real-time Track Monitoring
- Displays current track name
- Shows artist information
- Displays album name
- Updates playback status

### Clean Interface
```
Spotify Now Playing Monitor
-------------------------
Track: Song Name
Artist: Artist Name
Album: Album Name
Status: Playing
```

### Authentication
- Secure OAuth2 authentication
- Automatic token refresh
- Persistent token storage
- Safe credential handling

## Dependencies

- Windows HTTP API (winhttp)
- Windows Sockets API (ws2_32)
- nlohmann/json for JSON parsing

## Building from Source

### Requirements
- Visual Studio 2019 or later
- C++20 compatible compiler
- Windows SDK 10.0 or later

### Build Steps
1. Open `Spotify RPC.sln` in Visual Studio
2. Select your desired configuration (Debug/Release)
3. Build the solution

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Spotify Web API](https://developer.spotify.com/documentation/web-api/) for providing the API
- [nlohmann/json](https://github.com/nlohmann/json) for JSON parsing capabilities

## Support

If you encounter any issues or have questions, please [open an issue](https://github.com/yourusername/spotify-rpc/issues) on GitHub. 
