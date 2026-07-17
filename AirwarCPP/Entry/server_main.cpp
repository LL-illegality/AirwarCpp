#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>
#include <SDL3/SDL.h>
#include "../Core/RNG.h"
#include "../Core/Constants.h"
#include "../Core/Message.h"
#include "../Core/Queue.h"
#include "../Game/Game.h"
#include "../Net/UdpServer.h"

static std::string getExeDir() {
    const char* base = SDL_GetBasePath();
    if (!base) return "";
    std::string p = base;
    SDL_free(const_cast<char*>(base));
    return p;
}

int main(int argc, char* argv[]) {
    printf("AirwarCPP Server\n");
    printf("================\n");
    seedRNG();

    int port = 8000;
    if (argc > 1) port = atoi(argv[1]);
    if (port <= 0 || port > 65535) port = 8000;

    printf("Starting server on port %d...\n", port);

    Queue<Message> mq;
    auto game = std::make_shared<Game>(mq);

    UdpServer server;
    server.setGame(game);

    if (!server.start(port)) {
        printf("Failed to start server on port %d\n", port);
        return 1;
    }

    printf("Server running. Press Ctrl+C to stop.\n");

    // Run the event loop
    while (server.isRunning()) {
        server.runAsync();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    server.stop();
    printf("Server stopped.\n");
    return 0;
}
