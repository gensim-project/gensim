/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "DiagnosticContext.h"
#include "genC/ir/IR.h"

#include <cassert>

using namespace gensim;

const DiagnosticClass DiagnosticClass::Error("error", DiagnosticSeverity::Error);
const DiagnosticClass DiagnosticClass::Warning("warning", DiagnosticSeverity::Warning);
const DiagnosticClass DiagnosticClass::Info("info", DiagnosticSeverity::Info);

void DiagnosticContext::AddEntry(const DiagnosticClass& dclass, const std::string& message)
{
	entries_.push_back(DiagnosticEntry(source_, dclass, message));
}

void DiagnosticContext::AddEntry(const DiagnosticClass& dclass, const std::string& message, const DiagNode& node)
{
	entries_.push_back(DiagnosticEntry(source_, dclass, message, node));
}

std::ostream &operator<<(std::ostream &str, const gensim::DiagnosticSource& source)
{
	str << source.GetName();
	return str;
}

std::ostream &operator<<(std::ostream &str, const gensim::DiagnosticEntry& entry)
{
	static bool colourise_output = true;

	if (colourise_output) str << "\x1b[0m";

	str << "[" << entry.GetSource() << "] ";

	if (entry.IsNodeValid()) {
		str << entry.GetFilename() << ":" << entry.GetLineNo() << ": ";
	}

	switch (entry.GetClass().GetSeverity()) {
		case DiagnosticSeverity::Info:
			if (colourise_output) str << "\x1b[36;1m";
			str << "Info";
			if (colourise_output) str << "\x1b[0m";
			break;
		case DiagnosticSeverity::Warning:
			if (colourise_output) str << "\x1b[33;1m";
			str << "Warning";
			if (colourise_output) str << "\x1b[0m";
			break;
		case DiagnosticSeverity::Error:
			if (colourise_output) str << "\x1b[31;1m";
			str << "Error";
			if (colourise_output) str << "\x1b[0m";
			break;
		default:
			str << "(unknown severity)";
			break;
	}

	str << ": " << entry.GetMessage();
	return str;
}

std::ostream &operator<<(std::ostream &str, const gensim::DiagnosticContext& context)
{
	for (const auto &entry : context.GetEntries()) {
		str << entry << std::endl;
	}

	return str;
}

static char format_buffer[512];

std::string Format(const std::string& str, ...)
{
	va_list args;
	va_start(args, str);

	vsnprintf(format_buffer, sizeof(format_buffer)-1, str.c_str(), args);

	va_end(args);

	return std::string(format_buffer);
}
