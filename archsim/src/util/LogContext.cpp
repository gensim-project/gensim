/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LogContext.cpp
 *
 *  Created on: 24 Jun 2014
 *      Author: harry
 */

#include "util/LogContext.h"
#include "util/LogTargets.h"
#include "util/SimOptions.h"

#include <cassert>
#include <algorithm>
#include <stack>

#include <unistd.h>

#define MAX_LOG 1024

using namespace archsim::util;

// Give the RootLogManager a higher relative priority than the log contexts
LogManager LogManager::RootLogManager __attribute__((init_priority(150)));

DeclareLogContext(LogLogging, "Logs");

/*
 * Need to be careful when constructing as although the parent pointer will be valid, the data may not be as most log contexts
 * are statically initialized. So, we put off actually doing any real initialization until the LogManager decides we are ready.
 */
LogContext::LogContext(std::string name, LogManager& manager, LogContext *parent) : name(name), target(NULL), enabled(false), parent(parent)
{
	manager.RegisterContext(this);
}

/**
 * Initialises the log-context.
 */
void LogContext::Initialize(LogTarget *default_log_target)
{
	// Enable the context, set our default logging target and logging level
	enabled = true;
	target = default_log_target;
	level = (LogEvent::LogLevel)archsim::options::LogLevel.GetValue();

	// Create the fully-qualified name of this context.
	LogContext *ctx = this->parent;

	qualifiedname = name;
	while (ctx != NULL) {
		qualifiedname = ctx->name + "." + qualifiedname;
		ctx = ctx->parent;
	}

	// Register ourselves with our parent (if we have one).
	if (parent) parent->RegisterChild(this);
}

void LogContext::LogN(LogEvent::LogLevel level, const char *message, ...)
{
	if (!enabled) return;

	char buffer[MAX_LOG];
	va_list args;
	va_start(args, message);
	vsnprintf(buffer, MAX_LOG, message, args);
	va_end(args);

	Log(level, buffer);
}

void LogContext::Log(LogEvent::LogLevel level, const char *message)
{
	if (!enabled) return;

	LogEvent evt;
	gettimeofday(&evt.tv, NULL);

	evt.level = level;
	evt.subsystem = qualifiedname.c_str();
	evt.message = message;
	evt.filename = "[unknown]";
	evt.file_line_nr = -1;

	Log(evt);
}

void LogContext::Log(const LogEvent& evt)
{
	if (!enabled) return;
	target->Log(evt);
}

LogContext *LogContext::GetInnerLogContext(std::string searchName) const
{
	if(searchName.find('.') != searchName.npos) {
		std::string myChild = std::string(searchName.begin(), searchName.begin()+searchName.find('.'));
		std::string path = std::string(searchName.begin()+searchName.find('.')+1, searchName.end());
		if(GetChildByName(myChild) == NULL) return NULL;
		return GetChildByName(myChild)->GetInnerLogContext(path);
	} else return GetChildByName(searchName);
}

LogContext *LogContext::GetChildByName(std::string name) const
{
	if(!children.count(name)) return NULL;
	return children.at(name);
}

void LogContext::DumpString()
{
	fprintf(stderr, "%s\n", qualifiedname.c_str());
	for(auto child : children) child.second->DumpString();
}

/**
 * Enables this log-context, optionally recursively.
 * @param recursive Enable all descendants.
 * @param new_level The level at which this log-context should produce output.
 */
void LogContext::Enable(bool recursive, LogEvent::LogLevel new_level)
{
	enabled = true;
	level = new_level;

	if (recursive) for(auto child : children) child.second->Enable(recursive, level);
}

/**
 * Disables this log-context, optionally recursively.
 * @param recursive Disable all descendants.
 */
void LogContext::Disable(bool recursive)
{
	enabled = false;
	if(recursive) for(auto child : children) child.second->Disable(recursive);
}

/**
 * Registers a child log-context.
 * @param newchild The child log-context to register with us.
 */
void LogContext::RegisterChild(LogContext *newchild)
{
	children[newchild->name] = newchild;
}

/**
 * Sets the logging target for this log-context.
 * @param new_target The log-target object to use when this context logs an event.
 * @param recursive Apply the same log-target to all descendants.
 */
void LogContext::SetTarget(LogTarget *new_target, bool recursive)
{
	target = new_target;

	if (recursive) {
		for (auto child : children) {
			child.second->SetTarget(new_target, true);
		}
	}
}

LogStream::LogStream(LogEvent::LogLevel level_, std::string filename, int line_nr, LogContext &ctx) : level(level_), targetCtx(ctx), filename(filename), line_nr(line_nr) { }

LogStream::~LogStream()
{
	LogEvent evt;
	gettimeofday(&evt.tv, NULL);

	evt.level = level;
	evt.subsystem = targetCtx.GetQualifiedName().c_str();
	evt.message = this->str().c_str();
	evt.filename = filename.c_str();
	evt.file_line_nr = line_nr;

	targetCtx.Log(evt);
}

/**
 * Constructs a new instance of a log manager.
 */
LogManager::LogManager() : stdout_log_target(new FileBackedLogTarget(stdout)), stderr_log_target(new FileBackedLogTarget(stderr)) { }

/**
 * Cleans-up after the log manager's work is done.
 */
LogManager::~LogManager()
{
	if (default_log_target != stdout_log_target && default_log_target != stderr_log_target)
		delete default_log_target;
	delete stdout_log_target;
	delete stderr_log_target;
}

/**
 * Initialises the log manager
 */
bool LogManager::Initialize()
{
	if (archsim::options::LogTarget == "console") {
		default_log_target = new ConsoleBackedLogTarget();
	} else if (archsim::options::LogTarget == "stderr") {
		default_log_target = stderr_log_target;
	} else if (archsim::options::LogTarget.GetValue().substr(0, 5) == "unix:") {
		default_log_target = SocketBackedLogTarget::ConnectUnixSocket(archsim::options::LogTarget.GetValue().substr(5));
		if (!default_log_target) {
			fprintf(stderr, "error: unable to create unix socket for logging\n");
			return false;
		}
	} else if (archsim::options::LogTarget.GetValue().substr(0, 4) == "tcp:") {
		std::string host = archsim::options::LogTarget.GetValue().substr(4);
		int port = 1234;

		auto port_separator = host.find(":");
		if (port_separator != host.npos) {
			port = atoi(host.substr(port_separator + 1).c_str());
			host = host.substr(0, port_separator);
		}

		default_log_target = SocketBackedLogTarget::ConnectTcpSocket(host, port);
		if (!default_log_target) {
			fprintf(stderr, "error: unable to create tcp socket for logging\n");
			return false;
		}
	} else {
		FILE *target_file = fopen(archsim::options::LogTarget.GetValue().c_str(), "wt");

		if (!target_file) {
			fprintf(stderr, "error: unable to open logging target file %s\n", archsim::options::LogTarget.GetValue().c_str());
			return false;
		}

		default_log_target = new FileBackedLogTarget(target_file);
	}

	// Initialise every log-context that we manage.
	for (auto ctx : all_contexts)
		ctx->Initialize(default_log_target);

	return true;
}

/**
 * Registers a log-context with the log manager.
 * @param ctx
 */
void LogManager::RegisterContext(LogContext *ctx)
{
	// If this is a root context, store it in the log roots map.
	if (ctx->GetParent() == NULL)
		log_roots[ctx->GetName()] = ctx;

	// Add the context to the all contexts list.
	all_contexts.push_back(ctx);
}

/**
 * Retrieves the logcontext for the given qualified name
 * @param qname The fully-qualified name of the log context to retrieve
 * @return The logcontext declared as the given qualified name, or null if no context matched.
 */
LogContext *LogManager::GetLogContext(std::string qname) const
{
	size_t init_name_end = qname.find('.');
	std::string init_name;

	if(init_name_end == qname.npos) init_name = qname;
	else init_name = std::string(qname.begin(), qname.begin()+init_name_end);

	if(!log_roots.count(init_name)) return NULL;
	LogContext *root = log_roots.at(init_name);

	if(qname.find('.') != qname.npos) return root->GetInnerLogContext(std::string(qname.begin()+qname.find('.')+1, qname.end()));
	else return root;
}

/**
 * Loads and parses a logspec file, to control how logging happens.
 * @param spec_file The path to the file containing the logspec.
 * @return True is the file was successfully loaded, parsed and activated.
 */
bool LogManager::LoadLogSpec(std::string spec_file)
{
	LogSpecParser parser;

	// Load and parse the logspec file.
	if(!parser.Parse(*this, spec_file)) {
		DumpString();
		return false;
	}
	return true;
}

/**
 * Dumps a string representation of the logging tree.
 */
void LogManager::DumpString()
{
	for (auto a : log_roots) {
		a.second->DumpString();
	}
}

void LogManager::DumpGraphContext(LogContext *ctx)
{
	for (auto child : ctx->children) {
		fprintf(stderr, "\tP%016lx -> P%016lx\n", (unsigned long)ctx, (unsigned long)child.second);
		DumpGraphContext(child.second);
	}
}

/**
 * Dumps a DOT-graph representation of the logging tree.
 */
void LogManager::DumpGraph()
{
	fprintf(stderr, "digraph a {\n");
	for (auto a : all_contexts) {
		fprintf(stderr, "\tP%016lx [style=rounded, shape=record, label=\"{%s | %d}\",color=%s]\n", (unsigned long)&*a, a->GetName().c_str(), a->level, a->enabled ? "green" : "red");
	}

	for (auto a : log_roots) {
		DumpGraphContext(a.second);
	}

	fprintf(stderr, "}\n");
}
