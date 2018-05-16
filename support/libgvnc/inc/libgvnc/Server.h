#ifndef LIBGVNC_SERVER_H_
#define LIBGVNC_SERVER_H_

#include <mutex>
#include <condition_variable>
#include <thread>
#include <set>

#include "Client.h"
#include "Framebuffer.h"
#include "Keyboard.h"
#include "Pointer.h"
#include "net/Socket.h"

namespace libgvnc {
	
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
		void AddClient(Client *client);
		
		Framebuffer *GetFB() { return fb_; }
		Keyboard &GetKeyboard() { return keyboard_; }
		Pointer &GetPointer() { return pointer_; }
		
	private:
		static void *server_thread_tramp(Server *server) { return server->ServerThread(); }
		void *ServerThread();
                void NotifyServerReady();
		
		State state_;
		std::mutex lock_;
                std::condition_variable ready_cv_;
		net::Socket *listen_socket_;
		std::thread server_thread_;
		std::set<Client*> clients_;
		Framebuffer *fb_;
		Keyboard keyboard_;
		Pointer pointer_;
	};
	
}

#endif
