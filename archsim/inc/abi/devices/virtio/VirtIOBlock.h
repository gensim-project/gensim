/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   VirtIOBlock.h
 * Author: s0457958
 *
 * Created on 16 September 2014, 12:44
 */

#ifndef VIRTIOBLOCK_H
#define	VIRTIOBLOCK_H

#include "abi/devices/virtio/VirtIO.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace generic
			{
				namespace block
				{
					class BlockDevice;
				}
			}

			namespace virtio
			{
				class VirtIOBlock : public VirtIO
				{
				public:
					VirtIOBlock(EmulationModel& parent_model, IRQLine& irq, Address base_address, std::string name, generic::block::BlockDevice& bdev);
					virtual ~VirtIOBlock();

					void ResetDevice();

					void WriteRegister(MemoryRegister& reg, uint32_t value) override;

				protected:
					uint8_t *GetConfigArea() const override;
					uint32_t GetConfigAreaSize() const override;

				private:
					generic::block::BlockDevice& bdev;

					struct virtio_blk_req {
						uint32_t type;
						uint32_t ioprio;
						uint64_t sector;
					};

					void ProcessEvent(VirtIOQueueEvent* evt) override;
					bool HandleRead(uint64_t sector, uint8_t *buffer, uint32_t len);
					bool HandleWrite(uint64_t sector, uint8_t *buffer, uint32_t len);

					struct {
						uint64_t capacity; // 0
						uint32_t size_max; // 8
						uint32_t seg_max; // c
						struct {
							uint16_t cylinders; // 10
							uint8_t heads;      // 12
							uint8_t sectors;    // 13
						} geometry __attribute__((packed));
						uint32_t block_size;   // 14

						// Topology
						uint8_t physical_block_exp; // 18
						uint8_t alignmenet_offset; // 19
						uint16_t min_io_size; // 1a
						uint32_t opt_io_size; // 1c

						uint8_t wce;         // 20
						uint8_t unused;      // 21

						uint16_t num_queues; // 22
					} config __attribute__((packed));
				};
			}
		}
	}
}

#endif	/* VIRTIOBLOCK_H */

