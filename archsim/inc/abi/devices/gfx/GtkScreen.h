/*
 * GLScreen.h
 *
 *  Created on: 20 Apr 2015
 *      Author: harry
 */

#ifndef GLSCREEN_H_
#define GLSCREEN_H_

#include "abi/devices/gfx/VirtualScreen.h"
#include "abi/devices/gfx/VirtualScreenManager.h"

#include "concurrent/Thread.h"

#include <gtk-2.0/gdk/gdk.h>
#include <gtk-2.0/gtk/gtk.h>

#include <mutex>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace generic
			{
				class Keyboard;
				class Mouse;
			}
			namespace gfx
			{

				void key_press_event(GtkWidget *widget, GdkEventKey *event, void *screen);
				void key_release_event(GtkWidget *widget, GdkEventKey *event, void *screen);
				void motion_notify_event(GtkWidget *widget, GdkEventMotion *event, void *screen);
				void button_press_event(GtkWidget *widget, GdkEventButton *event, void *screen);

				class GtkScreen : public archsim::abi::devices::gfx::VirtualScreen, public archsim::concurrent::Thread
				{
				public:
					GtkScreen(std::string id, memory::MemoryModel *mem_model, System* sys);
					~GtkScreen();

					bool Initialise() override;
					bool Reset() override;

					void SetKeyboard(generic::Keyboard& kbd) override
					{
						this->kbd = &kbd;
					}
					void SetMouse(generic::Mouse& mouse) override
					{
						this->mouse = &mouse;
					}
				private:
					void run() override;

					GtkWidget *window, *draw_area;
					uint8_t *framebuffer, *guest_fb;

					generic::Keyboard *kbd;
					generic::Mouse *mouse;

					uint32_t last_mouse_x, last_mouse_y;
					uint32_t mouse_x, mouse_y;
					
					bool running;

					void draw_framebuffer();

					friend void key_press_event(GtkWidget *, GdkEventKey *, void *screen);
					friend void key_release_event(GtkWidget *, GdkEventKey *, void *screen);
					friend void motion_notify_event(GtkWidget *widget, GdkEventMotion *event, void *screen);
					friend void button_press_event(GtkWidget *widget, GdkEventButton *event, void *screen);
					
					std::mutex gtk_lock_;
				};

				extern VirtualScreenManager<GtkScreen> GtkScreenManager;

			}
		}
	}
}

#endif /* GLSCREEN_H_ */
