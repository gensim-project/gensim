/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LogContext.h
 *
 *  Created on: 24 Jun 2014
 *      Author: harry
 */

#ifndef LOGCONTEXT_H_
#define LOGCONTEXT_H_

#include <cstdarg>
#include <cstdio>
#include <sys/time.h>

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "concurrent/Mutex.h"
#include "define.h"
#include "SimOptions.h"

namespace archsim
{
	namespace util
	{

		class LogEvent
		{
		public:
			enum LogLevel {
				LL_DEBUG4,
				LL_DEBUG3,
				LL_DEBUG2,
				LL_DEBUG1,
				LL_INFO,
				LL_WARNING,
				LL_ERROR,
			};

			LogLevel level;
			std::string subsystem;
			std::string message;
			std::string filename;
			unsigned int file_line_nr;
			struct timeval tv;
		};

		class LogTarget;
		class LogManager;
		class LogContext;

		class LogStream : public std::stringstream
		{
		public:
			LogStream(LogEvent::LogLevel level, std::string filename, int line_nr, LogContext &ctx);
			~LogStream();

			inline std::string get_filename() const
			{
				return filename;
			}
			inline int get_line_nr() const
			{
				return line_nr;
			}

		private:
			LogEvent::LogLevel level;
			LogContext &targetCtx;
			std::string filename;
			int line_nr;
		};

		class LogContext
		{
			friend class LogManager;

		public:
			LogContext(std::string name, LogManager& manager, LogContext *parent = NULL);

			void Log(const LogEvent& evt);
			void Log(LogEvent::LogLevel level, const char *message);
			void LogN(LogEvent::LogLevel level, const char *message, ...);

			void Enable(bool recursive, LogEvent::LogLevel level);
			void Disable(bool recursive = true);

			void RegisterChild(LogContext *child);
			void SetTarget(LogTarget *newTarget, bool recursive = true);

			LogContext *GetInnerLogContext(std::string qualifiedName) const;
			LogContext *GetChildByName(std::string name) const;

			inline bool IsEnabled() const
			{
				return enabled;
			}

			inline bool IsEnabledForLevel(LogEvent::LogLevel requested_level) const
			{
				return enabled && level <= requested_level;
			}

			void Initialize(LogTarget *default_log_target);

			inline LogContext *GetParent() const
			{
				return parent;
			}

			inline std::string GetName() const
			{
				return name;
			}

			inline std::string GetQualifiedName() const
			{
				return qualifiedname;
			}

			void DumpString();

		private:
			std::string name;
			std::string qualifiedname;

			LogTarget *target;
			bool enabled;
			LogEvent::LogLevel level;

			LogContext *parent;
			std::unordered_map<std::string, LogContext*> children;
		};

		class LogSpecParser
		{
		public:
			bool Parse(LogManager& mgr, std::string file);
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
		class LogManager
		{
			friend class LogSpecParser;

		public:
			LogManager();
			~LogManager();

			bool Initialize();
			void RegisterContext(LogContext *ctx);

			LogContext *GetLogContext(std::string qualifiedName) const;

			bool LoadLogSpec(std::string spec_file);

			void DumpString();
			void DumpGraph();

			inline const std::unordered_map<std::string, LogContext*>& GetLogRoots() const
			{
				return log_roots;
			}

			inline LogTarget *stdout_target() const
			{
				return stdout_log_target;
			}

			inline LogTarget *stderr_target() const
			{
				return stderr_log_target;
			}

			static LogManager RootLogManager;

		private:
			void DumpGraphContext(LogContext *ctx);

			LogTarget *default_log_target;
			LogTarget *stdout_log_target;
			LogTarget *stderr_log_target;

			std::unordered_map<std::string, LogContext*> log_roots;
			std::vector<LogContext*> all_contexts;
		};

	}
}

#define DeclareLogContext(ContextID, ContextName) archsim::util::LogContext ContextID __attribute__((init_priority(180))) (ContextName, archsim::util::LogManager::RootLogManager);
#define DeclareChildLogContext(ContextID, Parent, ContextName) archsim::util::LogContext ContextID __attribute__((init_priority(190))) (ContextName, archsim::util::LogManager::RootLogManager, &Parent);

#define UseLogContext(ContextID) extern archsim::util::LogContext ContextID;

#ifdef __DISABLE_LOGGING
# define LC_LOG(_ctx_id, _level) if (0) archsim::util::LogStream(_level, __FILE__, __LINE__, _ctx_id)
#else
# define LC_LOG(_ctx_id, _level) if(LIKELY(!(_ctx_id.IsEnabledForLevel(_level)))) ; else archsim::util::LogStream(_level, __FILE__, __LINE__, _ctx_id)
#endif

#define LC_ERROR(ContextID)		LC_LOG(ContextID, archsim::util::LogEvent::LL_ERROR)
#define LC_INFO(ContextID)		LC_LOG(ContextID, archsim::util::LogEvent::LL_INFO)
#define LC_WARNING(ContextID)	LC_LOG(ContextID, archsim::util::LogEvent::LL_WARNING)

#ifndef NDEBUG
#define LC_DEBUG1(ContextID)	LC_LOG(ContextID, archsim::util::LogEvent::LL_DEBUG1)
#define LC_DEBUG2(ContextID)	LC_LOG(ContextID, archsim::util::LogEvent::LL_DEBUG2)
#define LC_DEBUG3(ContextID)	LC_LOG(ContextID, archsim::util::LogEvent::LL_DEBUG3)
#define LC_DEBUG4(ContextID)	LC_LOG(ContextID, archsim::util::LogEvent::LL_DEBUG4)
#else
#define LC_DEBUG1(ContextID)	if (1); else LC_LOG(ContextID, archsim::util::LogEvent::LL_DEBUG1)
#define LC_DEBUG2(ContextID)	if (1); else LC_LOG(ContextID, archsim::util::LogEvent::LL_DEBUG1)
#define LC_DEBUG3(ContextID)	if (1); else LC_LOG(ContextID, archsim::util::LogEvent::LL_DEBUG1)
#define LC_DEBUG4(ContextID)	if (1); else LC_LOG(ContextID, archsim::util::LogEvent::LL_DEBUG1)
#endif

#endif /* LOGCONTEXT_H_ */
