#ifndef TICK_TIMER_H
#define TICK_TIMER_H

#include <vector>
#include <x86intrin.h>
#include <malloc.h>
#include <string.h>

class tick_timer
{
public:
	tick_timer(bool enabled = 1, FILE *outfile=NULL) : enabled(enabled), _outfile(outfile)
	{
		if(_outfile == NULL) _outfile = stdout;
	}
	~tick_timer()
	{
		for(auto i : names) {
			if(i) free(i);
		}
	}

	void reset()
	{
		if(!enabled) return;
		ticks.clear();
		tick("Start");
	}
	void tick(const char *tickname = NULL)
	{
		if(!enabled) return;
		ticks.push_back(__rdtsc());
		if(tickname)names.push_back(strdup(tickname));
		else names.push_back(NULL);
	}

	void dump(const char *prefix=NULL)
	{
		if(!enabled) return;
		if(prefix) fprintf(_outfile, "%s", prefix);
		uint64_t last = ticks[0];
		for(uint32_t i = 1; i < ticks.size(); ++i) {
			uint64_t current = ticks[i];
			if(names[i])fprintf(_outfile, "%s: %lu\t", names[i], current-last);
			last = current;
		}

		fprintf(_outfile, "Total: %lu\n", ticks[ticks.size()-1] - ticks[0]);
	}

private:
	std::vector<uint64_t> ticks;
	std::vector<char *> names;
	bool enabled;
	FILE *_outfile;
};

#endif
