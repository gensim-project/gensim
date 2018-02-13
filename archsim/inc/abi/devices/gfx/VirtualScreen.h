/*
 * VirtualScreen.h
 *
 *  Created on: 21 Jul 2014
 *      Author: harry
 */

#ifndef VIRTUALSCREEN_H_
#define VIRTUALSCREEN_H_

#include "VirtualScreenManager.h"
#include "abi/devices/Component.h"

#include <string>

#include <cstdint>

namespace archsim
{

	namespace abi
	{

		namespace memory
		{
			class MemoryModel;
		}

		namespace devices
		{
			namespace generic
			{
				class Keyboard;
				class Mouse;
			}
			namespace gfx
			{

				enum VirtualScreenMode {
					VSM_None,
					VSM_8Bit,
					VSM_16bit,
					VSM_RGB,
					VSM_YUYV,
					VSM_DoomPalette,
					VSM_RGBA8888,
				};

				enum PaletteMode {
					PaletteDirect,
					PaletteInGuestMemory
				};

				class VirtualScreen : public Component
				{
				public:
					VirtualScreen(std::string id, memory::MemoryModel *mem);
					virtual ~VirtualScreen();

					inline std::string GetId()
					{
						return id;
					}

					/**
					 * Initialise this virtual screen. It must have been fully configured before this point:
					 * changes to the configuration are not allowed after the screen is initialised
					 */
					virtual bool Initialise() = 0;

					/**
					 * Reset and release any resources held by this screen. This can be used either to tidy
					 * the screen up, or to allow the screen to be reconfigured. Any existing configuration
					 * information is preserved.
					 */
					virtual bool Reset() = 0;

					/**
					 * Set the properties of the virtual screen. The screen can be reconfigured arbitrarily
					 * as long as it has not yet been initialised.
					 */
					virtual bool Configure(uint32_t width, uint32_t height, VirtualScreenMode mode);

					/**
					 * Set the frame buffer pointer to the given guest address. Note that this address is
					 * in whatever address space is supported by the target memory model i.e. it may
					 * be virtual or physical. This may be changeable without resetting the screen,
					 * depending on the implementation.
					 */
					virtual bool SetFramebufferPointer(uint32_t guest_addr);

					/**
					 * Set the palette pointer to the given guest address. Note that this address is
					 * in whatever address space is supported by the target memory model i.e. it may
					 * be virtual or physical. In non paletted display modes, this does nothing and
					 * returns FALSE, otherwise it returns TRUE. This may be changeable without resetting
					 * the screen, depending on the implementation.
					 */
					virtual bool SetPalettePointer(uint32_t guest_addr);

					virtual bool SetPaletteMode(PaletteMode new_mode);
					virtual bool SetPaletteEntry(uint32_t entry, uint32_t data);
					virtual bool SetPaletteSize(uint32_t new_size);

					virtual void SetKeyboard(generic::Keyboard& kbd) = 0;
					virtual void SetMouse(generic::Mouse& mouse) = 0;

					inline uint32_t GetWidth() const
					{
						return width;
					}
					inline uint32_t GetHeight() const
					{
						return height;
					}

					inline VirtualScreenMode GetMode() const
					{
						return mode;
					}

				protected:
					inline bool SetWidth(uint32_t nwidth)
					{
						if(initialised) return false;
						width = nwidth;
						return true;
					}
					inline bool SetHeight(uint32_t nheight)
					{
						if(initialised) return false;
						height = nheight;
						return true;
					}
					inline bool SetMode(VirtualScreenMode nmode)
					{
						if(initialised) return false;
						mode = nmode;
						return true;
					}

					inline void SetInitialised()
					{
						initialised = true;
					}
					inline void ClearInitialised()
					{
						initialised = false;
					}

					uint32_t fb_ptr, p_ptr;

					inline memory::MemoryModel *GetMemory()
					{
						return memory;
					}

				private:
					std::string id;
					memory::MemoryModel *memory;

					bool initialised;

					uint32_t width, height;
					VirtualScreenMode mode;

					PaletteMode palette_mode;
					uint32_t palette_size;
					uint32_t *palette_entries;
				};

				class NullScreen : public VirtualScreen
				{
				public:
					NullScreen(std::string id, memory::MemoryModel *mem_model, System *);
					~NullScreen();

					bool Initialise() override;
					bool Reset() override;

					void SetKeyboard(generic::Keyboard&) override;
					void SetMouse(generic::Mouse&) override;
				};

				extern VirtualScreenManager<NullScreen> NullScreenManager;

			}
		}
	}
}


#endif /* VIRTUALSCREEN_H_ */
