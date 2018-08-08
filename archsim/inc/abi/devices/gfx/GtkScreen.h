/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
#include "abi/memory/MemoryModel.h"

#include "concurrent/Thread.h"

#include <gtk-3.0/gdk/gdk.h>
#include <gtk-3.0/gtk/gtk.h>

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
				void draw_callback(GtkWidget *widget, cairo_t *cr, void *screen);
				void configure_callback(GtkWidget *widget, GdkEventConfigure *cr, void *screen);

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

					bool running;

				private:
					uint32_t GetPixelRGB(uint32_t x, uint32_t y);

					void run() override;

					GtkWidget *window, *draw_area;
					uint8_t *framebuffer, *guest_fb;

					generic::Keyboard *kbd;
					generic::Mouse *mouse;

					uint32_t last_mouse_x, last_mouse_y;
					uint32_t mouse_x, mouse_y;

					// The 'target' size (i.e., what should the guest image be resized to
					uint32_t target_width_, target_height_;

					void draw_framebuffer();

					friend void key_press_event(GtkWidget *, GdkEventKey *, void *screen);
					friend void key_release_event(GtkWidget *, GdkEventKey *, void *screen);
					friend void motion_notify_event(GtkWidget *widget, GdkEventMotion *event, void *screen);
					friend void button_press_event(GtkWidget *widget, GdkEventButton *event, void *screen);
					friend void draw_callback(GtkWidget *widget, cairo_t *cr, void *screen);
					friend void configure_callback(GtkWidget *widget, GdkEventConfigure *cr, void *screen);

					void grab();
					void ungrab();
					void set_cursor_position(int x, int y);

					std::mutex gtk_lock_;
					bool grabbed_;
					bool ignore_next_;
					GdkPixbuf *pb_;
					GdkCursor *cursor_;
					archsim::abi::memory::LockedMemoryRegion regions_;
				};

				extern VirtualScreenManager<GtkScreen> GtkScreenManager;

			}
		}
	}
}

#endif /* GLSCREEN_H_ */
