/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   VirtQueue.h
 * Author: s0457958
 *
 * Created on 17 September 2014, 16:38
 */

#ifndef VIRTQUEUE_H
#define	VIRTQUEUE_H

#include "define.h"
#include "util/LogContext.h"

UseLogContext(LogVirtIO);

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace virtio
			{
				class VirtQueue;
				class VirtRing
				{
				public:
					struct VirtRingDesc {
						uint64_t paddr;
						uint32_t len;
						uint16_t flags;
						uint16_t next;
					} __attribute__((packed));

					struct VirtRingAvail {
						uint16_t flags;
						uint16_t idx;
						uint16_t ring[];
					} __attribute__((packed));

					struct VirtRingUsedElem {
						uint32_t id;
						uint32_t len;
					} __attribute__((packed));

					struct VirtRingUsed {
						uint16_t flags;
						uint16_t idx;
						VirtRingUsedElem ring[];
					} __attribute__((packed));

					inline struct VirtRingDesc *GetDescriptor(uint16_t index) const {
						return &descriptors[index];
					}

					inline struct VirtRingAvail *GetAvailable() const {
						return avail;
					}

					inline struct VirtRingUsed *GetUsed() const {
						return used;
					}

					inline void SetHostBaseAddress(host_addr_t base_address, uint32_t size)
					{
						assert(size);

						descriptors = (VirtRingDesc *)base_address;
						avail = (VirtRingAvail *)((unsigned long)base_address + size * sizeof(VirtRingDesc));

						uint32_t avail_size = (size * sizeof(uint16_t)) + 4;
						uint32_t padding = 4096 - (avail_size % 4096);

						used = (VirtRingUsed *)((unsigned long)avail + avail_size + padding);

					}

					inline void DebugAddresses(Address guest_addr)
					{

						LC_DEBUG1(LogVirtIO) << "Virting base guest(" << std::hex << guest_addr << ");";
						LC_DEBUG1(LogVirtIO) << "Descriptors: guest(" << std::hex << guest_addr << ");";
						LC_DEBUG1(LogVirtIO) << "Avail: guest(" << std::hex << guest_addr.Get() + (char*)avail - (char*)descriptors << ");";
						LC_DEBUG1(LogVirtIO) << "Used: guest(" << std::hex << guest_addr.Get() + (char*)used - (char*)descriptors << ");";

					}

				private:
					VirtRingDesc *descriptors;
					VirtRingAvail *avail;
					VirtRingUsed *used;
				};

				class VirtQueue
				{
				public:
					VirtQueue(uint32_t self_index) : _self_index(self_index), last_avail_idx(0), guest_base_address(0), host_base_address(NULL), size(0), align(0) { }

					inline void SetBaseAddress(Address guest_base_address, host_addr_t host_base_address)
					{
						assert(size);

						this->guest_base_address = guest_base_address;
						this->host_base_address = host_base_address;

						ring.SetHostBaseAddress(host_base_address, size);
						ring.DebugAddresses(guest_base_address);
					}

					inline Address GetPhysAddr() const
					{
						return guest_base_address;
					}

					inline void SetAlign(uint32_t align)
					{
						this->align = align;
					}

					inline void SetSize(uint32_t size)
					{
						this->size = size;
					}

					inline const VirtRing::VirtRingDesc *PopDescriptorChainHead(uint16_t& out_idx)
					{
						uint16_t num_heads = ring.GetAvailable()->idx - last_avail_idx;
						assert(num_heads < size);

						if (num_heads == 0)
							return NULL;

						uint16_t head = ring.GetAvailable()->ring[last_avail_idx % size];
						assert(head < size);

						out_idx = head;

						last_avail_idx++;
						return ring.GetDescriptor(head);
					}

					inline const VirtRing::VirtRingDesc *GetDescriptor(uint16_t index)
					{
						return ring.GetDescriptor(index);
					}

					inline void Push(uint16_t elem_idx, uint32_t len)
					{
						uint16_t idx = ring.GetUsed()->idx % size;

						ring.GetUsed()->ring[idx].id = elem_idx;
						ring.GetUsed()->ring[idx].len = len;

						uint16_t old = ring.GetUsed()->idx;
						ring.GetUsed()->idx = old + 1;
					}

					inline uint32_t Index()
					{
						return _self_index;
					}

				private:
					VirtRing ring;

					Address guest_base_address;
					host_addr_t host_base_address;

					uint32_t align;
					uint32_t size;

					uint16_t last_avail_idx;

					uint32_t _self_index;
				};
			}
		}
	}
}

#endif	/* VIRTQUEUE_H */

