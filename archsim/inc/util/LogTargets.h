/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LogTargets.h
 * Author: s0457958
 *
 * Created on 02 July 2014, 08:36
 */

#ifndef LOGTARGETS_H
#define	LOGTARGETS_H

namespace archsim
{
	namespace util
	{
		class LogTarget
		{
		public:
			LogTarget();
			virtual ~LogTarget();
			virtual void Log(const LogEvent& evt) = 0;

		private:
			LogTarget(const LogTarget&) = delete;
		};

		class FileBackedLogTarget : public LogTarget
		{
		public:
			FileBackedLogTarget(FILE *f);
			void Log(const LogEvent& evt) override;

		private:
			FILE *file;
		};

		class ConsoleBackedLogTarget : public LogTarget
		{
		public:
			void Log(const LogEvent &evt) override;
		};

		class SocketBackedLogTarget : public LogTarget
		{
		public:
			SocketBackedLogTarget(int socket_fd);
			~SocketBackedLogTarget();

			void Log(const LogEvent& evt) override;

			static SocketBackedLogTarget *ConnectUnixSocket(std::string path);
			static SocketBackedLogTarget *ConnectTcpSocket(std::string host, int port);

		private:
			int socket_fd;
		};
	}
}


#endif	/* LOGTARGETS_H */

