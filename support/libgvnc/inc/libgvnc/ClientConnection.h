/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include <cstring>

#include "Framebuffer.h"
#include "net/Socket.h"

namespace libgvnc {
    class Server;
    
    struct fb_update_request {
        uint8_t incremental;
        uint16_t x_pos;
        uint16_t y_pos;
        uint16_t width;
        uint16_t height;
    };

    class ClientConnection {
    public:
        enum class State {
            Invalid,
            FreshlyConnected,

            // Handshaking
            HS_Done,

            // Initialisation
            Init_Done,

            Closed
        };

        ClientConnection(Server *server, net::Socket *client_socket);

        void Open();
        void Close();

        Server *GetServer() {
            return server_;
        }

    private:
        void SendRaw(const void *data, size_t size);

        template<typename T> void Send(const T& data) {
            SendRaw(&data, sizeof (T));
        }

        void Buffer(const void *data, size_t size) {
            auto oldsize = buffer_.size();
            buffer_.resize(buffer_.size() + size);
            memcpy(buffer_.data() + oldsize, (char*) data, size);
        }

        template<typename T> void Buffer(const T& data) {
            Buffer(&data, sizeof (T));
        }

        void SendBuffer() {
            SendRaw(buffer_.data(), buffer_.size());
            buffer_.clear();
        }

        void ReceiveRaw(void *data, size_t size);

        template<typename T> void Receive(T& data) {
            ReceiveRaw(&data, sizeof (T));
        }

        void Run();

        bool Handshake();
        bool Initialise();

        bool ServeClientMessage();
        bool ServeSetPixelFormat();
        bool ServeSetEncodings();
        bool ServeFramebufferUpdateRequest();
        bool ServeKeyEvent();
        bool ServePointerEvent();
        bool ServeClientCutText();

        void UpdateFB(const struct fb_update_request &request);

        void SetPixelFormat(struct FB_PixelFormat &new_format) {
            pixel_format_ = new_format;
        }

        struct FB_PixelFormat pixel_format_;
        std::thread thread_;
        std::mutex lock_;
        net::Socket *client_socket_;
        bool open_;
        State state_;
        int subversion_;
        Server *server_;

        std::vector<char> buffer_;
    };
}
