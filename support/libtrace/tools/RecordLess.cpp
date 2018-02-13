#include "libtrace/RecordFile.h"
#include "libtrace/InstructionPrinter.h"

#include <map>
#include <string>
#include <vector>

#include <cstdio>
#include <iostream>

#include <ncurses.h>

using namespace libtrace;

#define BOOKMARK_WIDTH 10000

int64_t top_index = 0;
int64_t left_offset = 0;

RecordFile *open_file = nullptr;

uint32_t terminal_height, terminal_width;

std::map<uint64_t, uint64_t> instruction_header_bookmarks;

std::string input_buffer;

std::vector<std::string> search_history;
int32_t search_history_index;

enum InputMode {
	Input_Command,
	Input_Goto,
	Input_Goto_Waiting,
	Input_Search
};

enum DisplayMode {
	Display_All,
	Display_OnlyMem
};

InputMode mode = Input_Command;
DisplayMode display_mode = Display_All;

bool DrawStatus();
bool ExecuteSearch(bool);

bool GetInstructionHeaderIndex(uint64_t instruction_idx, uint64_t &record_idx)
{
	uint64_t bookmark = (instruction_idx / BOOKMARK_WIDTH) * BOOKMARK_WIDTH;
	record_idx = 0;
	
	auto bookmark_it = instruction_header_bookmarks.upper_bound(bookmark);
	bookmark_it--;
	
	uint64_t current_idx = bookmark_it->first;
	record_idx = bookmark_it->second;
	
	while(current_idx < instruction_idx) {
		if((current_idx % BOOKMARK_WIDTH) == 0) instruction_header_bookmarks[current_idx] = record_idx;
		if(open_file->Size() <= record_idx) return false;
		
		Record r = open_file->Get(record_idx);
		TraceRecord *tr = (TraceRecord*)&r;

		assert(tr->GetType() == InstructionHeader);
		record_idx++;
		while(true) {
			if(open_file->Size() <= record_idx) return false;
			r = open_file->Get(record_idx);
			
			if(tr->GetType() == InstructionHeader) break;
			record_idx++;
		}

		current_idx++;
	}
	
	//fprintf(stderr, "For I %lu, got index %lu\n", instruction_idx, record_idx);
	
	assert(current_idx == instruction_idx);
	
	return true;
}

bool SetupScreen()
{
	initscr();
	noecho();
	//raw();
	keypad(stdscr, 1);
	
	// This is not true if we have metadata
	instruction_header_bookmarks[0] = 0;
	return true;
}

bool ReleaseScreen()
{
	endwin();
	return true;
}

void ScanToEnd()
{
	// first, check to see what our last bookmark is
	top_index = instruction_header_bookmarks.rbegin()->first;
	
	uint64_t record_idx = instruction_header_bookmarks.rbegin()->second;
	
	
	while(true) {
		if(record_idx >= open_file->Size()) break;
		
		Record r = open_file->Get(record_idx);
		TraceRecord *tr = (TraceRecord*)&r;
		if(tr->GetType() == InstructionHeader) {
			if((top_index % BOOKMARK_WIDTH) == 0) instruction_header_bookmarks[top_index] = record_idx;
			top_index++;
		}
		record_idx++;
	}
}

bool HandleInputCommand()
{
	int ch = getch();
	
	switch(ch) {
		case KEY_UP: 
			if(top_index > 0) top_index--;
			break;
		case KEY_DOWN: 
			top_index++;
			break;
		case KEY_LEFT: 
			left_offset -= 10;
			if(left_offset < 0) left_offset = 0;
			break;
		case KEY_RIGHT: 
			left_offset += 10;
			break;
		
		case KEY_NPAGE: 
			top_index += terminal_height;
			break;
		case KEY_PPAGE: 
			top_index -= terminal_height;
			if(top_index < 0) top_index = 0;
			break;
		
		case KEY_HOME:
			top_index = 0;
			break;
		case KEY_END:
			ScanToEnd();
			break;
		
		case 'm':
			if(display_mode == Display_All) display_mode = Display_OnlyMem;
			else display_mode = Display_All;
			break;
		
		case 'q':
			return false;
		
		case '0'...'9':
			mode = Input_Goto;
			input_buffer = "";
			input_buffer.push_back(ch);
			break;
		
		case '/':
			mode = Input_Search;
			search_history_index = search_history.size();
			input_buffer = "";
			break;
		case 'b':
		case 'n':
			if(search_history.size()) {
				input_buffer = search_history.back();
				top_index++;
				ExecuteSearch(ch == 'b');
			}
			break;
		
	}
	return true;
}

bool HandleInputGoto()
{
	int ch = getch();
	
	switch(ch) {
		case KEY_BACKSPACE:
			if(input_buffer.empty()) mode = Input_Command;
			else input_buffer.pop_back();
			break;
		
		case '0'...'9':
			input_buffer.push_back(ch);
			break;
		
		case '\n':
			top_index = strtol(input_buffer.c_str(), NULL, 10)-1;
			mode = Input_Goto_Waiting;
			
			// redraw status while we wait for the goto to complete
			DrawStatus();
			refresh();
			
			break;
		default:
			break;
	}
	return true;
}

bool ExecuteSearch(bool reverse)
{
	search_history.push_back(input_buffer);
	
	std::string search_bits, search_mask_bits;
	for(auto i : input_buffer) {
		i = tolower(i);
		switch(i){
			case '0'...'9':
			case 'a'...'f':
				search_bits.push_back(i);
				search_mask_bits.push_back('f');
				break;
			case 'x':
				search_mask_bits.push_back('0');
				search_bits.push_back('0');
				break;
			default:
				return false;
		}
	}
	
	uint32_t search_data = strtol(search_bits.c_str(), NULL, 16);
	uint32_t search_mask = strtol(search_mask_bits.c_str(), NULL, 16);
	
	int8_t addend = reverse ? -1 : 1;
	
	// start scanning through records for a match
	uint64_t record_idx;
	
	GetInstructionHeaderIndex(top_index, record_idx);
	record_idx += addend;
	
	uint64_t instruction_match_idx = top_index;
	
	while(true) {
		if(record_idx >= open_file->Size()) return false;
		Record r = open_file->Get(record_idx);
		TraceRecord *tr = (TraceRecord*)&r;
		
		if(tr->GetType() ==  InstructionHeader) instruction_match_idx += addend;
		if((tr->GetData32() & search_mask) == search_data) break;
		
		record_idx += addend;		
	}
	
	top_index = instruction_match_idx;
	mode = Input_Command;
	
	return true;
}

bool HandleInputSearch()
{
	int ch = getch();
	
	if(ch >= 32 && ch < 256) input_buffer.push_back(ch);
	else {
		switch(ch) {
			case KEY_BACKSPACE:
				if(input_buffer.empty()) mode = Input_Command;
				else input_buffer.pop_back();
				break;
			case '\n':
				ExecuteSearch(false);
				break;
				
			case KEY_UP:
				search_history_index--;
				if(search_history_index < 0) search_history_index = 0;
				if(search_history.size() != 0) 
					input_buffer = search_history[search_history_index];
				break;
			case KEY_DOWN:
				search_history_index++;
				if(search_history_index > search_history.size()) search_history_index = search_history.size();
				if(search_history_index == search_history.size()) input_buffer = "";
				else input_buffer = search_history[search_history_index];
				break;
				
			default:
				break;
		}
	}
	
	return true;
}

bool HandleInput()
{
	switch(mode) {
		case Input_Goto_Waiting:
			mode = Input_Command; //fall through
		case Input_Command:
			return HandleInputCommand();
			
		case Input_Goto:
			return HandleInputGoto();

		case Input_Search:
			return HandleInputSearch();

		default:
			assert(false && "Unknown mode");
	}
	return false;
}

bool DrawStatus()
{
	// Draw input/status bar
	move(terminal_height-1, 0);
	switch(mode) {
		case Input_Command:	printw(":"); break;
		
		case Input_Goto: printw("# %s", input_buffer.c_str()); break;
		case Input_Goto_Waiting: printw("# %s...", input_buffer.c_str()); break;
		
		case Input_Search: printw("/ %s", input_buffer.c_str()); break;
	}
	
	// Draw current top line number (+1 since index is 0 based but humans are 1-based)
	char buffer[16];
	int chars = snprintf(buffer, 16, "%lu", top_index+1);
	move(terminal_height-1, terminal_width-chars);
	printw(buffer);	
	return true;
}

bool DrawScreen()
{
	clear();
	getmaxyx(stdscr, terminal_height, terminal_width);
	
	// Draw instruction info
	InstructionPrinter ip;
	
	if(display_mode == Display_OnlyMem) {
		ip.SetDisplayNone();
		ip.SetDisplayMem();
	} else {
		ip.SetDisplayAll();
	}
	
	for(uint64_t line = 0; line < terminal_height-1; ++line) {
		uint64_t i = line + top_index;
		
		uint64_t target_idx = 0;
		bool exists = GetInstructionHeaderIndex(i, target_idx);
		
		if(exists) {
			RecordBufferStreamAdaptor adaptor (open_file);
			adaptor.Skip(target_idx);
			TracePacketStreamAdaptor tpsa(&adaptor);
			move(line, 0);
			
			std::string insn = ip(&tpsa);
			
			if(insn.size() > left_offset)
				printw("%s", insn.substr(left_offset, terminal_width).c_str());
		}
	}
	
	DrawStatus();
	
	refresh();
	
	return HandleInput();
}

int main(int argc, char **argv)
{
	if(argc == 1) {
		fprintf(stderr, "Usage: %s [record file]\n", argv[0]);
		return 1;
	}
	
	FILE *file = fopen(argv[1], "r");
	if(!file) {
		perror("Could not open file");
		return 1;
	}
	
	open_file = new RecordFile(file);
	
	SetupScreen();
	while(DrawScreen()) ;
	ReleaseScreen();
	
	delete open_file;
	return 0;
}
