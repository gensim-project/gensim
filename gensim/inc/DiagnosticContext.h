#pragma once

#include <stdexcept>
#include <ostream>
#include <string>
#include <vector>
#include "genC/DiagNode.h"

namespace gensim
{
	namespace genc
	{
		class IRStatement;
	}

	namespace DiagnosticSeverity
	{
		enum DiagnosticSeverity {
			Info,
			Warning,
			Error
		};
	}

	class DiagnosticSource
	{
	public:
		DiagnosticSource(const std::string& name) : name_(name) { }

		const std::string& GetName() const
		{
			return name_;
		}

	private:
		const std::string name_;
	};

	class DiagnosticClass
	{
	public:
		static const DiagnosticClass Error;
		static const DiagnosticClass Warning;
		static const DiagnosticClass Info;

		DiagnosticClass(const std::string& name, DiagnosticSeverity::DiagnosticSeverity severity) : name_(name), severity_(severity) { }

		const std::string& GetName() const
		{
			return name_;
		}
		DiagnosticSeverity::DiagnosticSeverity GetSeverity() const
		{
			return severity_;
		}

	private:
		const std::string name_;
		DiagnosticSeverity::DiagnosticSeverity severity_;
	};

	class DiagnosticEntry
	{
	public:
		DiagnosticEntry(const DiagnosticSource& source, const DiagnosticClass& dclass, const std::string& message)
			: source_(source), dclass_(dclass), message_(message), diag_node_(), node_valid_(false) { }

		DiagnosticEntry(const DiagnosticSource& source, const DiagnosticClass& dclass, const std::string& message, const DiagNode& diag_node)
			: source_(source), dclass_(dclass), message_(message), diag_node_(diag_node), node_valid_(true) { }

		DiagnosticEntry(const DiagnosticEntry& o) : source_(o.source_), dclass_(o.dclass_), message_(o.message_), diag_node_(o.diag_node_), node_valid_(o.node_valid_) { }
		DiagnosticEntry(DiagnosticEntry&& o) : source_(o.source_), dclass_(o.dclass_), message_(std::move(o.message_)), diag_node_(o.diag_node_), node_valid_(o.node_valid_) { }

		const DiagnosticSource& GetSource() const
		{
			return source_;
		}

		const DiagnosticClass& GetClass() const
		{
			return dclass_;
		}

		std::string GetMessage() const
		{
			return message_;
		}

		bool IsNodeValid() const
		{
			return node_valid_;
		}

		const DiagNode& GetNode() const
		{
			if (!node_valid_) throw std::logic_error("Diagnostic node is not valid");
			return diag_node_;
		}

		const std::string& GetFilename() const
		{
			if (!node_valid_) throw std::logic_error("Diagnostic node is not valid");
			return diag_node_.Filename();
		}

		uint32_t GetLineNo() const
		{
			if (!node_valid_) throw std::logic_error("Diagnostic node is not valid");
			return diag_node_.Line();
		}

	private:
		const DiagnosticSource& source_;
		const DiagnosticClass& dclass_;
		const std::string message_;
		const DiagNode diag_node_;
		bool node_valid_;
	};

	class DiagnosticContext
	{
	public:
		typedef std::vector<DiagnosticEntry> entry_container_t;

		DiagnosticContext(const DiagnosticSource& source) : source_(source) { }

		void AddEntry(const DiagnosticClass& dclass, const std::string& message);
		void AddEntry(const DiagnosticClass& dclass, const std::string& message, const DiagNode& node);

		void Info(const std::string& message)
		{
			AddEntry(DiagnosticClass::Info, message);
		}
		void Warning(const std::string& message)
		{
			AddEntry(DiagnosticClass::Warning, message);
		}
		void Error(const std::string& message)
		{
			AddEntry(DiagnosticClass::Error, message);
		}

		void Info(const std::string& message, const DiagNode &node)
		{
			AddEntry(DiagnosticClass::Info, message, node);
		}
		void Warning(const std::string& message, const DiagNode &node)
		{
			AddEntry(DiagnosticClass::Warning, message, node);
		}
		void Error(const std::string& message, const DiagNode &node)
		{
			AddEntry(DiagnosticClass::Error, message, node);
		}

		void Info(const std::string &message, const std::string &filename)
		{
			Info(message, DiagNode(filename));
		}
		void Warning(const std::string &message, const std::string &filename)
		{
			Warning(message, DiagNode(filename));
		}
		void Error(const std::string &message, const std::string &filename)
		{
			Error(message, DiagNode(filename));
		}

		const entry_container_t &GetEntries() const
		{
			return entries_;
		}

	private:
		DiagnosticContext(const DiagnosticContext&) = delete;
		DiagnosticContext(DiagnosticContext&&) = delete;

		const DiagnosticSource& source_;
		std::vector<DiagnosticEntry> entries_;
	};

}

std::ostream &operator<<(std::ostream &str, const gensim::DiagnosticSource& source);
std::ostream &operator<<(std::ostream &str, const gensim::DiagnosticEntry& entry);
std::ostream &operator<<(std::ostream &str, const gensim::DiagnosticContext& context);

std::string Format(const std::string& str, ...);
