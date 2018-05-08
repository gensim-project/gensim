/*
 * GLScreen.cpp
 *
 *  Created on: 20 Apr 2015
 *      Author: harry
 */

#include <gtk-3.0/gdk/gdk.h>
//#include <gtk-3.0/gdk/gdkkeysyms.h>
//#include <gtk-3.0/gtk/gtk.h>

#include "abi/devices/gfx/GtkScreen.h"
#include "abi/memory/MemoryModel.h"

#include "abi/devices/generic/Keyboard.h"
#include "abi/devices/generic/Mouse.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"

UseLogContext(LogVirtualScreen);
DeclareChildLogContext(LogGdkScreen, LogVirtualScreen, "GDK");

using namespace archsim::abi::devices::gfx;

#define MAP(a, b) case GDK_KEY_##a: scancode = b; break
static bool GtkKeyToScancode(uint32_t gtk, uint16_t &scancode)
{
	switch(gtk) {
			MAP(a, 0x1c);
			MAP(A, 0x1c);
			MAP(b, 0x32);
			MAP(B, 0x32);
			MAP(c, 0x21);
			MAP(C, 0x21);
			MAP(d, 0x23);
			MAP(D, 0x23);
			MAP(e, 0x24);
			MAP(E, 0x24);
			MAP(f, 0x2B);
			MAP(F, 0x2B);
			MAP(g, 0x34);
			MAP(G, 0x34);
			MAP(h, 0x33);
			MAP(H, 0x33);
			MAP(i, 0x43);
			MAP(I, 0x43);
			MAP(j, 0x3B);
			MAP(J, 0x3B);
			MAP(k, 0x42);
			MAP(K, 0x42);
			MAP(l, 0x4B);
			MAP(L, 0x4B);
			MAP(m, 0x3a);
			MAP(M, 0x3a);
			MAP(n, 0x31);
			MAP(N, 0x31);
			MAP(o, 0x44);
			MAP(O, 0x44);
			MAP(p, 0x4D);
			MAP(P, 0x4D);
			MAP(q, 0x15);
			MAP(Q, 0x15);
			MAP(r, 0x2d);
			MAP(R, 0x2d);
			MAP(s, 0x1b);
			MAP(S, 0x1b);
			MAP(t, 0x2c);
			MAP(T, 0x2c);
			MAP(u, 0x3c);
			MAP(U, 0x3c);
			MAP(v, 0x2A);
			MAP(V, 0x2A);
			MAP(w, 0x1d);
			MAP(W, 0x1d);
			MAP(x, 0x22);
			MAP(X, 0x22);
			MAP(y, 0x35);
			MAP(Y, 0x35);
			MAP(z, 0x1a);
			MAP(Z, 0x1a);

			MAP(0, 0x45);
			MAP(1, 0x16);
			MAP(2, 0x1E);
			MAP(3, 0x26);
			MAP(4, 0x25);
			MAP(5, 0x2E);
			MAP(6, 0x36);
			MAP(7, 0x3D);
			MAP(8, 0x3E);
			MAP(9, 0x46);
			MAP(asciitilde, 0x0e);
			MAP(hyphen, 0x2d);
			MAP(equal, 0x55);
			MAP(backslash, 0x5d);
			MAP(BackSpace, 0x66);
			MAP(space, 0x29);
			MAP(Tab, 0x0d);
			MAP(Caps_Lock, 0x58);
			MAP(Shift_L, 0x12);
			MAP(Control_L, 0x14);
			MAP(Alt_L, 0x11);
			MAP(Shift_R, 0x59);
			MAP(Control_R, 0xe014);
			MAP(Alt_R, 0xe011);
			MAP(Return, 0x5a);
			MAP(Escape, 0x76);
			MAP(F1, 0x05);
			MAP(F2, 0x06);
			MAP(F3, 0x04);
			MAP(F4, 0x0c);
			MAP(F5, 0x03);
			MAP(F6, 0x0b);
			MAP(F7, 0x83);
			MAP(F8, 0x0a);
			MAP(F9, 0x01);
			MAP(F10, 0x09);
			MAP(F11, 0x78);
			MAP(F12, 0x07);
			//Print Screen
			MAP(Scroll_Lock, 0x7e);
			MAP(bracketleft, 0x54);
			MAP(Insert, 0xe070);
			MAP(Home, 0xe06c);
			MAP(Page_Up, 0xe07d);
			MAP(Delete, 0xE071);
			MAP(End, 0xe069);
			MAP(Page_Down, 0xe07a);
			MAP(Up, 0xe075);
			MAP(Left, 0xe06b);
			MAP(Down, 0xe072);
			MAP(Right, 0xe074);
			MAP(Num_Lock, 0x77);
			MAP(KP_Divide, 0xe04a);
			MAP(KP_Multiply, 0x7c);
			MAP(KP_Subtract, 0x7b);
			MAP(KP_Add, 0x79);
			MAP(KP_Enter, 0xe05a);
			MAP(KP_Separator, 0x71);
			MAP(KP_0, 0x70);
			MAP(KP_1, 0x69);
			MAP(KP_2, 0x72);
			MAP(KP_3, 0x7a);
			MAP(KP_4, 0x6b);
			MAP(KP_5, 0x73);
			MAP(KP_6, 0x74);
			MAP(KP_7, 0x6c);
			MAP(KP_8, 0x75);
			MAP(KP_9, 0x7D);
			MAP(bracketright, 0x5B);
			MAP(semicolon, 0x4c);
			MAP(apostrophe, 0x52);
			MAP(comma, 0x41);
			MAP(period, 0x49);
			MAP(slash, 0x4a);

		default:
			return false;
	}
	return true;
}
#undef MAP


void archsim::abi::devices::gfx::key_press_event(GtkWidget *widget, GdkEventKey *event, void *screen)
{
	GtkScreen *scr = (GtkScreen *)screen;
	uint16_t scancode = 0;
	
	if(event->keyval == GDK_KEY_Control_R) {
		scr->ungrab();
	}

	if(GtkKeyToScancode(event->keyval, scancode)) {
		scr->kbd->KeyDown(scancode);
	} else {
		LC_ERROR(LogGdkScreen) << "Unrecognized screen scancode " << event->keyval;
	}
}

void archsim::abi::devices::gfx::key_release_event(GtkWidget *widget, GdkEventKey *event, void *screen)
{
	GtkScreen *scr = (GtkScreen *)screen;
	uint16_t scancode = 0;

	if(GtkKeyToScancode(event->keyval, scancode)) {
		scr->kbd->KeyUp(scancode);
	} else {
		LC_ERROR(LogGdkScreen) << "Unrecognized screen scancode " << event->keyval;
	}
}

void archsim::abi::devices::gfx::motion_notify_event(GtkWidget *widget, GdkEventMotion *event, void *screen)
{
	GtkScreen *scr = (GtkScreen*)screen;
	if(scr->mouse_y == event->x && scr->mouse_y == event->y) {
		return;
	}
	
	if(scr->grabbed_) {
		if(scr->ignore_next_) {
			scr->ignore_next_ = false;
		} else {
			scr->mouse_x = event->x;
			scr->mouse_y = event->y;
		}
	}
}

void archsim::abi::devices::gfx::button_press_event(GtkWidget *widget, GdkEventButton *event, void *screen)
{
	GtkScreen *scr = (GtkScreen*)screen;

	if(!scr->grabbed_) {
		// grab the mouse if it isn't grabbed
		scr->grab();
	} else {
	
		//convert button indices
		uint32_t ps2_index;
		switch(event->button) {
			case 1: // left
				ps2_index = 0;
				break;
			case 2: // middle
				ps2_index = 2;
				break;
			case 3: // right
				ps2_index = 1;
				break;
			default:
				assert(false);
		}

		switch(event->type) {
			case GDK_BUTTON_PRESS:
				scr->mouse->ButtonDown(ps2_index);
				break;

			case GDK_BUTTON_RELEASE:
				scr->mouse->ButtonUp(ps2_index);
				break;

			default:
				break;
		}
		
	}
}

GtkScreen::GtkScreen(std::string id, memory::MemoryModel *mem_model, System* sys) : VirtualScreen(id, mem_model), Thread("Gtk Screen"), running(false), framebuffer(NULL), draw_area(NULL), window(NULL), last_mouse_x(0), last_mouse_y(0)
{

}

GtkScreen::~GtkScreen()
{
	if(running)Reset();
}

void archsim::abi::devices::gfx::draw_callback(GtkWidget *widget, cairo_t *cr, void *data) 
{
	GtkScreen *screen = (GtkScreen*)data;
	
	
	uint16_t *fbp = (uint16_t*)screen->guest_fb;
	uint16_t *fbp_end = fbp + (screen->GetHeight() * screen->GetWidth());
	
	GdkPixbuf *pb = screen->pb_;
	guchar *pixels = gdk_pixbuf_get_pixels(pb);
	
	for(; fbp != fbp_end;) {
		uint16_t pxl = *fbp++;
		uint16_t r, g, b;

		// Extract the RGB565 data
		r = pxl >> 11;
		g = (pxl >> 5) & 0x3f;
		b = pxl & 0x1f;

		// Repack it into 24-bit RGB
		uint32_t out = (b << 19) | (g << 10) | (r << 3);

		*(uint32_t*)pixels = out;
		pixels += 3;
	}
	
	
	gdk_cairo_set_source_pixbuf(cr, pb, 0, 0);
	cairo_paint(cr);
}

bool GtkScreen::Initialise()
{
	{
		std::lock_guard<std::mutex> lock(gtk_lock_);

		gtk_init(NULL, NULL);

		cursor_ = gdk_cursor_new(GDK_BLANK_CURSOR);
		
		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		draw_area = gtk_drawing_area_new();
		pb_ = gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, GetWidth(), GetHeight());

		gtk_widget_set_events(window, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
		gtk_widget_set_double_buffered(draw_area, false);

		gtk_widget_set_size_request(draw_area, GetWidth(), GetHeight());
		gtk_container_add(GTK_CONTAINER(window), draw_area);

		gtk_widget_show_all(window);
		gtk_window_set_title((GtkWindow*)window, GetId().c_str());
		gtk_window_set_resizable((GtkWindow*)window,false);
		
		g_signal_connect(window, "key-press-event", G_CALLBACK(key_press_event), this);
		g_signal_connect(window, "key-release-event", G_CALLBACK(key_release_event), this);

//		g_signal_connect(window, "motion-notify-event", G_CALLBACK(motion_notify_event), this);
		g_signal_connect(window, "button-press-event", G_CALLBACK(button_press_event), this);
		g_signal_connect(window, "button-release-event", G_CALLBACK(button_press_event), this);

		g_signal_connect(draw_area, "draw", G_CALLBACK(draw_callback), this);
		
		framebuffer = (uint8_t*)malloc(3 * GetWidth() * GetHeight());

		host_addr_t guest_fb_ptr;
		GetMemory()->LockRegion(fb_ptr, GetWidth() * GetHeight(), guest_fb_ptr);
		guest_fb = (uint8_t*)guest_fb_ptr;

		running = true;
	}


	start();

	return true;
}

bool GtkScreen::Reset()
{
	running = false;

	std::lock_guard<std::mutex> lock(gtk_lock_);

	if(framebuffer)free(framebuffer);
	framebuffer = NULL;

	if(draw_area) gtk_widget_destroy(draw_area);
	draw_area = NULL;

	if(window) gtk_widget_destroy(window);
	window = NULL;

	return true;
}

void GtkScreen::draw_framebuffer()
{
	
	
}

void GtkScreen::grab()
{
	auto display = gtk_widget_get_display(window);
	auto seat = gdk_display_get_default_seat(display);
	
	gdk_seat_grab(seat, gtk_widget_get_window(window), GDK_SEAT_CAPABILITY_ALL, false, cursor_, nullptr, nullptr, nullptr);
	
	set_cursor_position(GetWidth()/2, GetHeight()/2);
	
	grabbed_ = true;
}

void GtkScreen::ungrab()
{
	auto display = gtk_widget_get_display(window);
	auto seat = gdk_display_get_default_seat(display);
	gdk_seat_ungrab(seat);
	
	grabbed_ = false;
}

void GtkScreen::set_cursor_position(int x, int y)
{
	int root_x, root_y;
	gdk_window_get_root_origin(gtk_widget_get_window(window), &root_x, &root_y);
	auto device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(window)));
	gdk_device_warp(device, gtk_widget_get_screen(window), root_x + x, root_y + y);
}


void GtkScreen::run()
{
	while(running) {
		{
			std::lock_guard<std::mutex> lock(gtk_lock_);

			while(gtk_events_pending()) {
				gtk_main_iteration_do(FALSE);
			}

			draw_framebuffer();

			if(grabbed_) {
				GdkScreen *scr;
				gint x, y;
				
				auto seat = gdk_display_get_default_seat(gtk_widget_get_display(window));
				
				gdk_device_get_position(gdk_seat_get_pointer(seat), &scr, &x, &y);
				x -= GetWidth()/2;
				y -= GetHeight()/2;
				mouse_x += x;
				mouse_y += y;
				
				int inverted_y = GetHeight() - mouse_y;
				mouse->Move(mouse_x, inverted_y);

				auto device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(window)));
				gdk_device_warp(device, gtk_widget_get_screen(window), GetWidth()/2, GetHeight()/2);
			}

			gtk_widget_queue_draw(draw_area);
		}

		usleep(20000);
	}
}

class GtkScreenManager : public VirtualScreenManager<GtkScreen> {};

RegisterComponent(VirtualScreenManagerBase, GtkScreenManager, "Gtk", "Gtk based screen manager")

