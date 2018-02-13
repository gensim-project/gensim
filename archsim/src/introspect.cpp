/*
 * introspect.cpp
 */
#include <cassert>
#include <stdio.h>
#include <malloc.h>
#include <execinfo.h>

#include <chrono>
#include <unordered_map>
#include <list>
#include <mutex>

struct AllocInfo {
	void *base_ptr;
	size_t size;
	std::chrono::high_resolution_clock::time_point allocation_time;
	std::chrono::high_resolution_clock::time_point free_time;
	char **backtrace;
	int backtrace_size;
};

typedef std::unordered_map<void *, AllocInfo> allocations_map;
typedef std::list<AllocInfo> allocations_list;

allocations_map *allocations;
allocations_list *old_allocations;

std::recursive_mutex allocations_lock;

static volatile int ctor_state = 0;

void *operator new(size_t size)
{
	std::unique_lock<std::recursive_mutex> lock(allocations_lock);

	if (ctor_state == 0) {
		ctor_state = 1;
		allocations = new allocations_map();
		old_allocations = new allocations_list();
		ctor_state = 2;
	} else if (ctor_state == 1) {
		return malloc(size);
	}

	void *ptr = malloc(size);
	if (!ptr)
		return NULL;

	AllocInfo info;
	info.base_ptr = ptr;
	info.size = size;
	info.allocation_time = std::chrono::high_resolution_clock::now();

	void *buffer[1024];
	int count = backtrace(buffer, 1024);

	info.backtrace_size = count;
	info.backtrace = backtrace_symbols(buffer, count);

	ctor_state = 1;
	allocations->insert(std::pair<void *, AllocInfo>(ptr, info));
	ctor_state = 2;

	return ptr;
}

void operator delete(void *ptr, size_t s)
{
	std::unique_lock<std::recursive_mutex> lock(allocations_lock);

	if (ctor_state == 1) {
		free(ptr);
		return;
	}

	ctor_state = 1;
	allocations_map::const_iterator info = allocations->find(ptr);

	if (info == allocations->end()) {
	} else {
		AllocInfo old_info = info->second;
		allocations->erase(info);

		old_info.free_time = std::chrono::high_resolution_clock::now();
		old_allocations->push_back(old_info);
	}

	free(ptr);
	ctor_state = 2;
}

void DumpLiveObjects()
{
	std::unique_lock<std::recursive_mutex> lock(allocations_lock);

	FILE *f = fopen("objects.txt", "wt");
	fprintf(f, "Undead Objects:\n");

	std::chrono::high_resolution_clock::time_point ref = std::chrono::high_resolution_clock::now();
	for (auto alloc : *allocations) {
		int age = (int)std::chrono::duration_cast<std::chrono::seconds>(ref - alloc.second.allocation_time).count();
		fprintf(f, "ALLOC %p %#lx %ds\n", alloc.second.base_ptr, alloc.second.size, age);

		for (int count = 0; count < alloc.second.backtrace_size; count++) {
			fprintf(f, "  %s\n", alloc.second.backtrace[count]);
		}
	}

	fclose(f);
}
