#include "translate/profile/Block.h"
#include "util/LogContext.h"

UseLogContext(LogLifetime);

using namespace archsim::translate::profile;

Block::Block(Region& parent, Address offset, uint8_t isa_mode) : parent(parent), offset(offset), isa_mode(isa_mode), root(false), status(NotTranslated) { }
