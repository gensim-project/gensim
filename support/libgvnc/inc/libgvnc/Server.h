#ifndef LIBGVNC_SERVER_H_
#define LIBGVNC_SERVER_H_

#include <mutex>
#include <thread>
#include <set>

#include "Client.h"
#include "Framebuffer.h"
#include "Keyboard.h"
#include "Pointer.h"

namespace libgvnc {
	
	class Server {
		enum class State {
			INVALID,
			Closed,
			Open
		};
		
	public:
		Server(Framebuffer *fb);
		
		bool Open(int listen_port);
		void AddClient(Client *client);
		
		Framebuffer *GetFB() { return fb_; }
		Keyboard &GetKeyboard() { return keyboard_; }
		Pointer &GetPointer() { return pointer_; }
		
	private:
		static void server_thread(Server *server);
		
		State state_;
		std::mutex lock_;
		int listen_socket_;
		std::thread server_thread_;
		std::set<Client*> clients_;
		Framebuffer *fb_;
		Keyboard keyboard_;
		Pointer pointer_;
	};
	
}

#endif
