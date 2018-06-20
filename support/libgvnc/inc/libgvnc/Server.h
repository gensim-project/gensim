/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <set>

#include "Framebuffer.h"
#include "Keyboard.h"
#include "Pointer.h"
#include "net/Socket.h"

namespace libgvnc {
    class ClientConnection;
    
    class Server {
        enum class State {
            INVALID,
            Starting,
            Closed,
            Open
        };

    public:
        Server(Framebuffer *fb);
        ~Server();

        bool Open(int listen_port);

        Framebuffer *GetFB() {
            return fb_;
        }

        Keyboard &GetKeyboard() {
            return keyboard_;
        }

        Pointer &GetPointer() {
            return pointer_;
        }

    private:
        static void *server_thread_tramp(Server *server) {
            return server->ServerThread();
        }

        void *ServerThread();
        void NotifyServerReady();
        void AddClientConnection(ClientConnection *client);

        State state_;
        std::mutex lock_;
        std::condition_variable ready_cv_;
        net::Socket *listen_socket_;
        std::thread server_thread_;
        std::set<ClientConnection *> clients_;
        Framebuffer *fb_;
        Keyboard keyboard_;
        Pointer pointer_;
    };
}
