/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   PerfMap.h
 * Author: harry
 *
 * Created on 03 February 2017, 15:20
 */

#ifndef PERFMAP_H
#define PERFMAP_H

#include "util/SimOptions.h"

#include <fstream>
#include <mutex>
#include <sstream>

#include <cassert>
#include <unistd.h>

class PerfMap
{
public:
	PerfMap() : _busy(false), _perf_map(nullptr) {}

	void Acquire()
	{
		_mtx.lock();
		_busy = true;
	}
	std::ofstream &Stream()
	{
		CheckOpen();
		assert(_busy);
		return *_perf_map;
	}
	void Release()
	{
		_mtx.unlock();
		_busy = false;
	}

	bool Enabled()
	{
		return archsim::options::EnablePerfMap;
	}

	static PerfMap Singleton;
private:
	void CheckOpen()
	{
		if(_perf_map == nullptr) Open();
	}

	void Open()
	{
		uint32_t pid = getpid();
		std::stringstream str;
		str << "/tmp/perf-" << pid << ".map";
		_perf_map = new std::ofstream(str.str().c_str());
	}

	bool _enabled;
	bool _busy;
	std::mutex _mtx;
	std::ofstream *_perf_map;
};

#endif /* PERFMAP_H */

