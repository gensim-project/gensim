/*
 * Copyright (C) 2018 Harry Wagstaff
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
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
