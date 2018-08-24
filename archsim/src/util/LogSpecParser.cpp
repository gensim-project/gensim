/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "util/LogContext.h"
#include "util/LogTargets.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <cctype>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

UseLogContext(LogLogging);

using namespace archsim::util;

struct BufferLocation {
	int line, col;
};

class Buffer
{
public:
	Buffer(std::istream& f) : file(f)
	{
		loc.col = 0;
		loc.line = 1;

		ReadChar();
	}

	char ReadChar()
	{
		if (file.good()) {
			file >> std::noskipws >> ch;

			if (ch == '\n') {
				loc.line++;
				loc.col = 1;
			} else {
				loc.col++;
			}

			return ch;
		} else {
			return 0;
		}
	}

	inline bool eof() const
	{
		return file.eof();
	}

	inline char GetCurrentChar() const
	{
		return ch;
	}

	inline BufferLocation GetLocation() const
	{
		return loc;
	}

private:
	char ch;
	std::istream& file;
	BufferLocation loc;
};

static const char *token_kinds[] = {
	"identifier",
	"path",

	"enable",
	"disable",
	"stdout",
	"stderr",

	".",
	"=",
	"~",
	"!",
	">",
	"/",
	"*",
	"@",

	"[newline]",

	"[eof]",
	"[unknown]",
};

struct Token {
	enum TokenKind {
		T_IDENT = 0,
		T_PATH,

		T_ENABLE,
		T_DISABLE,
		T_STDOUT,
		T_STDERR,

		T_DOT,
		T_EQUALS,
		T_TILDE,
		T_PLING,
		T_RCHEV,
		T_FSLASH,
		T_STAR,
		T_AT,

		T_NEWLINE,

		T_EOF,
		T_UNKNOWN
	};

	TokenKind kind;
	std::string value;
	BufferLocation location;

	inline void dump() const
	{
		fprintf(stderr, "token: kind=%s, value=%s, location=%d:%d\n", token_kinds[kind], value.c_str(), location.line, location.col);
	}
};

class Lexer
{
public:
	Lexer(Buffer& b) : buffer(b) { }

	Token ReadToken()
	{
		SkipWhitespace();
		SkipComments();
		SkipWhitespace();

		token.location = buffer.GetLocation();

		if (buffer.eof()) {
			token.kind = Token::T_EOF;
			token.value = std::string();
			return token;
		}

		if (isalpha(buffer.GetCurrentChar())) {
			ReadIdentifier();
		} else if (buffer.GetCurrentChar() == '\'') {
			ReadPath();
		} else {
			switch(buffer.GetCurrentChar()) {
				case '.':
					token.kind = Token::T_DOT;
					token.value = ".";
					break;
				case '=':
					token.kind = Token::T_EQUALS;
					token.value = "=";
					break;
				case '!':
					token.kind = Token::T_PLING;
					token.value = "!";
					break;
				case '>':
					token.kind = Token::T_RCHEV;
					token.value = ">";
					break;
				case '/':
					token.kind = Token::T_FSLASH;
					token.value = "/";
					break;
				case '*':
					token.kind = Token::T_STAR;
					token.value = "*";
					break;
				case '@':
					token.kind = Token::T_AT;
					token.value = "@";
					break;
				case '~':
					token.kind = Token::T_TILDE;
					token.value = "~";
					break;
				case '\n':
					token.kind = Token::T_NEWLINE;
					token.value = "";
					break;
				default:
					token.kind = Token::T_UNKNOWN;
					token.value = std::string(1, buffer.GetCurrentChar());
					break;
			}

			buffer.ReadChar();
		}

		return token;
	}

	inline Token GetCurrentToken() const
	{
		return token;
	}

private:
	Buffer& buffer;
	Token token;

	void SkipWhitespace()
	{
		while (!buffer.eof() && isblank(buffer.GetCurrentChar())) {
			buffer.ReadChar();
		}
	}

	void SkipComments()
	{
		while (buffer.GetCurrentChar() == '#') {
			while (buffer.GetCurrentChar() != '\n') {
				buffer.ReadChar();
			}

			SkipWhitespace();
		}
	}

	void ReadIdentifier()
	{
		std::stringstream str;

		while (!buffer.eof() && (isalpha(buffer.GetCurrentChar()) || buffer.GetCurrentChar() == '-' || isdigit(buffer.GetCurrentChar()))) {
			str << buffer.GetCurrentChar();
			buffer.ReadChar();
		}

		token.value = str.str();

		if (token.value == "enable")
			token.kind = Token::T_ENABLE;
		else if (token.value == "disable")
			token.kind = Token::T_DISABLE;
		else if (token.value == "stdout")
			token.kind = Token::T_STDOUT;
		else if (token.value == "stderr")
			token.kind = Token::T_STDERR;
		else
			token.kind = Token::T_IDENT;
	}

	void ReadPath()
	{
		std::stringstream str;

		// Consume the quote mark
		buffer.ReadChar();

		// Read characters until the ending quote mark
		while (!buffer.eof() && buffer.GetCurrentChar() != '\'') {
			str << buffer.GetCurrentChar();
			buffer.ReadChar();
		}

		// Consume the ending quotation mark
		buffer.ReadChar();

		token.value = str.str();
		token.kind = Token::T_PATH;
	}
};

/*
 * The LogManager is the main configuration center for the various logs. Configuration is mainly done via 'Log Spec' strings,
 * which describe an operation to take on a particular log:
 *
 * The first kind of configuration options are specifying the target for a log. This can be a file path or a special file e.g.:
 *
 * fully.qualified.logname=stdout - set the output of the log to stdout
 * fully.qualified.logname=stderr - set the output of the log to stderr
 * fully.qualified.logname=/some/file/path - set the output of the log to /some/file/path
 *
 * If required, the log target can also be specified as recursive by adding an ! to the start of the path name e.g.:
 * fully.qualified.logname=!/some/file/path
 *
 * The second kind of option enable or disable logs. As with target selection, they can be specified as recursive:
 *
 * fully.qualified.logname>enable - enable just this log context
 * fully.qualified.logname>!enable - recursively enable this log context and its children
 * fully.qualified.logname>disable - disable just this log context
 * fully.qualified.logname>!disable - recursively enable this log context and its children
 *
 */

class Parser
{
public:
	Parser(Lexer& l, LogManager& mgr) : lexer(l), manager(mgr) { }

	bool Parse()
	{
		lexer.ReadToken();
		return start();
	}

private:
	Lexer& lexer;
	LogManager& manager;

	bool match(Token::TokenKind kind)
	{
		return lexer.GetCurrentToken().kind == kind;
	}

	bool match(Token::TokenKind kind, std::string& value)
	{
		value = lexer.GetCurrentToken().value;
		return lexer.GetCurrentToken().kind == kind;
	}

	bool expect(Token::TokenKind kind, std::string& value)
	{
		bool matched = match(kind, value);

		if (!matched) {
			LC_ERROR(LogLogging) << "Parse Error: Line " << lexer.GetCurrentToken().location.line << ", Expected " << token_kinds[kind] << ", read " << token_kinds[lexer.GetCurrentToken().kind];
		}

		lexer.ReadToken();

		return matched;
	}

	bool expect(Token::TokenKind kind)
	{
		std::string v;
		return expect(kind, v);
	}

	// start := defs eof
	bool start()
	{
		if (!defs())
			return false;
		return expect(Token::T_EOF);
	}

	// defs :=
	// defs := def defs
	bool defs()
	{
		if (match(Token::T_IDENT) || match(Token::T_STAR) || match(Token::T_NEWLINE)) {
			if (!def())
				return false;
			return defs();
		} else {
			return true;
		}
	}

	bool handle_state_def(std::string fqn, bool recursive, bool enable, LogEvent::LogLevel level)
	{
		if (fqn == "*") {
			for (auto root : manager.GetLogRoots()) {
				if (enable)
					root.second->Enable(recursive, level);
				else
					root.second->Disable(recursive);
			}
		} else {
			LogContext *ctx = manager.GetLogContext(fqn);
			if (ctx == NULL) {
				LC_ERROR(LogLogging) << "Parse Warning: Logging context " << fqn << " is not defined!";
				return false;
			}

			if (enable) {
				ctx->Enable(recursive, level);
			} else {
				ctx->Disable(recursive);
			}
		}

		return true;
	}

	bool handle_std_def(std::string fqn, bool recursive, bool stdout)
	{
		LogContext *ctx = manager.GetLogContext(fqn);
		if (ctx == NULL) {
			LC_ERROR(LogLogging) << "Parse Warning: Logging context " << fqn << " is not defined!";
			return false;
		}

		if (stdout)
			ctx->SetTarget(LogManager::RootLogManager.stdout_target(), recursive);
		else
			ctx->SetTarget(LogManager::RootLogManager.stderr_target(), recursive);

		return true;
	}

	bool handle_target_def(std::string fqn, bool recursive, std::string target_file)
	{
		if (target_file == "") {
			LC_ERROR(LogLogging) << "Parse Error: Unable to assign target";
			return false;
		}

		LogContext *ctx = manager.GetLogContext(fqn);
		if (ctx == NULL) {
			LC_ERROR(LogLogging) << "Parse Warning: Logging context " << fqn << " is not defined!";
			return false;
		}

		// FIXME: Need to clean-up all these targets at some point.
		FILE *f = fopen(target_file.c_str(), "wt");
		if (!f) {
			LC_ERROR(LogLogging) << "Parse Error: Unable to open output file";
			return false;
		}

		ctx->SetTarget(new FileBackedLogTarget(f), recursive);

		return true;
	}

	bool handle_socket_def(std::string fqn, bool recursive, std::string socket_path)
	{
		LogContext *ctx = manager.GetLogContext(fqn);
		if (ctx == NULL) {
			LC_ERROR(LogLogging) << "Parse Warning: Logging context " << fqn << " is not defined!";
			return false;
		}

		LC_ERROR(LogLogging) << "Socket based logging targets not implemented";
		return false;
	}

	// def := NEWLINE
	// def := fq_or_star EQ maybe_pling device_or_path NEWLINE
	// def := fq_or_star TILDE maybe_pling PATH NEWLINE
	// def := fq_or_star RCHEV maybe_pling enable_or_disable maybe_level NEWLINE
	bool def()
	{
		std::stringstream fqn;

		if (match(Token::T_NEWLINE))
			return expect(Token::T_NEWLINE);

		if (!fq_or_star(fqn))
			return false;

		if (match(Token::T_EQUALS)) {
			bool recursive;
			std::string name;

			if (!expect(Token::T_EQUALS))
				return false;

			if (!maybe_pling(recursive))
				return false;

			if (!device_or_path(name)) {
				return false;
			}

			if (!expect(Token::T_NEWLINE))
				return false;

			if (name == "stdout" || name == "stderr")
				return handle_std_def(fqn.str(), recursive, name == "stdout");
			else
				return handle_target_def(fqn.str(), recursive, name);
		} else if (match(Token::T_RCHEV)) {
			bool recursive = false, enable = false;

			if (!expect(Token::T_RCHEV))
				return false;

			if (!maybe_pling(recursive))
				return false;

			if (!enable_or_disable(enable))
				return false;

			LogEvent::LogLevel level = (LogEvent::LogLevel)archsim::options::LogLevel.GetValue();

			if (enable && match(Token::T_AT)) {
				if (!maybe_level(level))
					return false;
			}

			if (!expect(Token::T_NEWLINE))
				return false;

			return handle_state_def(fqn.str(), recursive, enable, level);
		} else if (match(Token::T_TILDE)) {
			bool recursive;
			std::string socket_path;

			if (!expect(Token::T_TILDE))
				return false;

			if (!maybe_pling(recursive))
				return false;

			if (!expect(Token::T_PATH, socket_path))
				return false;

			if (!expect(Token::T_NEWLINE))
				return false;

			return handle_socket_def(fqn.str(), recursive, socket_path);
		} else {
			LC_ERROR(LogLogging) << "Parse Error: Expected '=', '>' or '~', got a " << token_kinds[lexer.GetCurrentToken().kind];
			return false;
		}
	}

	// maybe_level :=
	// maybe_level := AT log_level
	bool maybe_level(LogEvent::LogLevel& level)
	{
		if (match(Token::T_AT)) {
			if (!expect(Token::T_AT))
				return false;

			std::string level_text;
			if (!log_level(level_text))
				return false;

			if (level_text == "error") {
				level = LogEvent::LL_ERROR;
			} else if (level_text == "warning") {
				level = LogEvent::LL_WARNING;
			} else if (level_text == "info") {
				level = LogEvent::LL_INFO;
			} else if (level_text == "debug1") {
				level = LogEvent::LL_DEBUG1;
			} else if (level_text == "debug2") {
				level = LogEvent::LL_DEBUG2;
			} else if (level_text == "debug3") {
				level = LogEvent::LL_DEBUG3;
			} else if (level_text == "debug4") {
				level = LogEvent::LL_DEBUG4;
			} else {
				LC_ERROR(LogLogging) << "Parse Error: Expected 'error', 'warning', 'info', 'debug1', 'debug2', 'debug3' or 'debug4'";
				return false;
			}

			return true;
		}

		return true;
	}

	// log_level := ID
	bool log_level(std::string& level_text)
	{
		return expect(Token::T_IDENT, level_text);
	}

	// device_or_path := T_STDOUT | T_STDERR | T_PATH
	bool device_or_path(std::string& name)
	{
		if (match(Token::T_STDOUT)) {
			name = "stdout";
			return expect(Token::T_STDOUT);
		} else if (match(Token::T_STDERR)) {
			name = "stderr";
			return expect(Token::T_STDERR);
		} else if (match(Token::T_PATH)) {
			if (!expect(Token::T_PATH, name))
				return false;

			return true;
		} else {
			LC_ERROR(LogLogging) << "Parse Error: Expected 'stdout', 'stderr', or file-path";
			return false;
		}
	}

	// enable_or_disable := T_ENABLE | T_DISABLE
	bool enable_or_disable(bool& enable)
	{
		if (match(Token::T_ENABLE)) {
			enable = true;
			return expect(Token::T_ENABLE);
		} else if (match(Token::T_DISABLE)) {
			enable = false;
			return expect(Token::T_DISABLE);
		} else {
			LC_ERROR(LogLogging) << "Parse Error: Expected 'enable' or 'disable'";
			return false;
		}
	}

	// fq_or_star := STAR | fq
	bool fq_or_star(std::stringstream& fqstr)
	{
		if (match(Token::T_STAR)) {
			if (!expect(Token::T_STAR))
				return false;

			fqstr << "*";
			return true;
		}

		return fq(fqstr);
	}

	// fq := ID
	// fq := ID DOT fq
	bool fq(std::stringstream& fqstr)
	{
		fqstr << lexer.GetCurrentToken().value;
		if (!expect(Token::T_IDENT))
			return false;

		if (match(Token::T_DOT)) {
			fqstr << ".";
			if (!expect(Token::T_DOT))
				return false;
			return fq(fqstr);
		}

		return true;
	}

	// maybe_pling :=
	// maybe_pling := PLING
	bool maybe_pling(bool& has_pling)
	{
		if (match(Token::T_PLING)) {
			has_pling = true;
			return expect(Token::T_PLING);
		} else {
			has_pling = false;
		}

		return true;
	}
};

bool LogSpecParser::Parse(LogManager& mgr, std::string filename)
{
	std::fstream file(filename);

	if (!file.good()) {
		LC_ERROR(LogLogging) << "Unable to open logspec file " << filename;
		return false;
	}

	Buffer buffer(file);
	Lexer lexer(buffer);
	Parser parser(lexer, mgr);

	return parser.Parse();
}
