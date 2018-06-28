/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/ir-sorter.h"

#include <cstring>

using namespace captive::arch::jit;
using namespace captive::arch::jit::algo;
using namespace captive::shared;

uint32_t find_run_size(uint32_t* ptr, uint32_t *end)
{
	uint32_t first_idx = ptr[0];
	uint32_t size = 0;
	while((ptr + size) < end && ptr[size] == first_idx + size) {
		size++;
	}

	return size;
}

IRInstruction *IRSorter::perform_sort()
{
	insn_idxs = (uint32_t*)malloc(count() * sizeof(uint32_t));
	uint32_t *insn_idxs_end = insn_idxs + count();

	for(uint32_t i = 0; i < ctx.count(); ++i) {
		insn_idxs[i] = i;
	}

	do_sort();

	IRInstruction *new_buffer = (IRInstruction*)malloc(ctx.count() * sizeof(IRInstruction));
	IRInstruction *buffer_ptr = new_buffer;

	uint32_t run_start = 0;
	while(run_start < count()) {
		uint32_t run_size = find_run_size(&insn_idxs[run_start], insn_idxs_end);
		memcpy(buffer_ptr, ctx.get_ir_buffer() + insn_idxs[run_start], run_size * sizeof(*buffer_ptr));
		buffer_ptr += run_size;
		run_start += run_size;
	}

	free(insn_idxs);

	insn_idxs = NULL;

	return new_buffer;
}

bool GnomeSort::do_sort()
{
	uint32_t pos = 1;

	while (pos < count()) {
		if (key(pos) >= key(pos - 1)) {
			pos += 1;
		} else {
			exchange(pos, pos - 1);

			if (pos > 1) {
				pos--;
			}
		}
	}

	return true;
}

bool MergeSort::do_sort()
{
	sort(0, count());
	return true;
}

bool MergeSort::sort(int from, int to)
{
	if ((to - from) < 200) {
		return insert_sort(from, to);
	}

	int middle = (from + to) / 2;
	sort(from, middle);
	sort(middle, to);
	merge(from, middle, to, middle - from, to - middle);

	return true;
}

void MergeSort::merge(int from, int pivot, int to, int len1, int len2)
{
	if (len1 == 0 || len2 == 0) return;

	if ((len1 + len2) == 2) {
		if (compare(pivot, from) < 0) {
			exchange(pivot, from);
		}

		return;
	}

	int first_cut, second_cut;
	int len11, len22;

	if (len1 > len2) {
		len11 = len1 / 2;
		first_cut = from + len11;
		second_cut = lower(pivot, to, first_cut);
		len22 = second_cut - pivot;
	} else {
		len22 = len2 / 2;
		second_cut = pivot + len22;
		first_cut = upper(from, pivot, second_cut);
		len11 = first_cut - from;
	}

	rotate(first_cut, pivot, second_cut);
	int new_mid = first_cut + len22;
	merge(from, first_cut, new_mid, len11, len22);
	merge(new_mid, second_cut, to, len1 - len11, len2 - len22);
}

void MergeSort::rotate(int from, int mid, int to)
{
	reverse(from, mid - 1);
	reverse(mid, to - 1);
	reverse(from, to - 1);

	/*
	if (from == mid || mid == to) return;
	int n = gcd(to - from, mid - from);
	while (n-- != 0) {
		IRInstruction val = *ctx.at(from + n);
		int shift = mid - from;
		int p1 = from + n, p2 = from + n + shift;
		while (p2 != (from + n)) {
			ctx.put(p1, *ctx.at(p2));
			p1 = p2;
			if (to - p2 > shift) p2 += shift;
			else p2 = from + (shift - (to - p2));
		}

		ctx.put(p1, val);
	}
	*/
}

void MergeSort::reverse(int from, int to)
{
	while (from < to) {
		exchange(from++, to--);
	}
}

bool MergeSort::insert_sort(int from, int to)
{
	if (to > (from + 1)) {
		for (int i = from + 1; i < to; i++) {
			for (int j = i; j > from; j--) {
				if (compare(j, j - 1) < 0) {
					exchange(j, j - 1);
				} else {
					break;
				}
			}
		}
	}

	return true;
}

int MergeSort::upper(int from, int to, int val)
{
	int len = to - from, half;
	while (len > 0) {
		half = len / 2;
		int mid = from + half;
		if (compare(val, mid) < 0) {
			len = half;
		} else {
			from = mid + 1;
			len = len - half - 1;
		}
	}

	return from;
}

int MergeSort::lower(int from, int to, int val)
{
	int len = to - from, half;
	while (len > 0) {
		half = len / 2;
		int mid = from + half;
		if (compare(mid, val) < 0) {
			from = mid + 1;
			len = len - half - 1;
		} else {
			len = half;
		}
	}

	return from;
}

bool NopFilter::do_sort()
{
	uint32_t fast_ptr, slow_ptr;
	fast_ptr = 1;

	// whenever slow_ptr encounters a nop slot, fast_ptr advances until it
	// finds a real instruction. Then, it swaps the instruction for the nop.

	for(slow_ptr = 0; slow_ptr < count(); ++slow_ptr) {
		if(key(slow_ptr) == NOP_BLOCK) {
			if(fast_ptr <= slow_ptr) {
				fast_ptr = slow_ptr + 1;
			}
			for(; fast_ptr < count(); ++fast_ptr) {
				if(key(fast_ptr) != NOP_BLOCK) {
					break;
				}
			}

			if(fast_ptr >= count()) {
				return true;
			}

			exchange(slow_ptr, fast_ptr);
		}
	}

	return true;
}
