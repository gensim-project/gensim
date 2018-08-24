/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <cassert>

#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

#include "util/LogContext.h"
#include "util/LogTargets.h"

#define TEXT_BUFFER_SIZE 256
#define TIME_BUFFER_SIZE 64

using namespace archsim::util;

static const char *log_level_text[] = {
	"DBG4",
	"DBG3",
	"DBG2",
	"DBG1",
	"INFO",
	"WARN",
	"ERRR",
};

LogTarget::LogTarget() { }
LogTarget::~LogTarget() { }

FileBackedLogTarget::FileBackedLogTarget(FILE *f) : file(f)
{

}

void FileBackedLogTarget::Log(const LogEvent& evt)
{
	char time[TIME_BUFFER_SIZE];
	struct tm tm;

	// Sanity check for log_level_text purposes.
	assert(evt.level < 7);

	// Create a textual format
	localtime_r(&evt.tv.tv_sec, &tm);
	strftime(time, sizeof(time)-1, "%H:%M:%S", &tm);
	//fprintf(file, "%s.%06ld (%s:%d) [%s] (%s) %s\n", time, evt.tv.tv_usec, evt.filename, evt.file_line_nr, log_level_text[evt.level], evt.subsystem, evt.message);
	fprintf(file, "%lu %s.%06ld [%s] (%s) %s\n", pthread_self(), time, evt.tv.tv_usec, log_level_text[evt.level], evt.subsystem.c_str(), evt.message.c_str());
	fflush(file);
}

void ConsoleBackedLogTarget::Log(const LogEvent &evt)
{
	char time[TIME_BUFFER_SIZE];
	struct tm tm;

	// Sanity check for log_level_text purposes.
	assert(evt.level < 7);

	// Create a textual format
	localtime_r(&evt.tv.tv_sec, &tm);
	strftime(time, sizeof(time)-1, "%H:%M:%S", &tm);
	//fprintf(file, "%s.%06ld (%s:%d) [%s] (%s) %s\n", time, evt.tv.tv_usec, evt.filename, evt.file_line_nr, log_level_text[evt.level], evt.subsystem, evt.message);
	if(evt.level == LogEvent::LL_ERROR) {
		fprintf(stdout, "%lu %s.%06ld [%s] (%s) %s\n", pthread_self(), time, evt.tv.tv_usec, log_level_text[evt.level], evt.subsystem.c_str(), evt.message.c_str());
	} else {
		fprintf(stdout, "%lu %s.%06ld [%s] (%s) %s\n", pthread_self(), time, evt.tv.tv_usec, log_level_text[evt.level], evt.subsystem.c_str(), evt.message.c_str());
	}
}

SocketBackedLogTarget::SocketBackedLogTarget(int socket_fd_) : socket_fd(socket_fd_) { }

SocketBackedLogTarget::~SocketBackedLogTarget()
{
	close(socket_fd);
}

void SocketBackedLogTarget::Log(const LogEvent& evt)
{
	unsigned short v;

	// Send magic number
	v = 0x0110;
	send(socket_fd, &v, sizeof(v), 0);

	// Send packet size [MAGIC, TIMESTAMP, LEVEL, SZ + SUBSYS, SZ + MSG]
	v = (sizeof(v) * 4) + sizeof(evt.tv) + evt.subsystem.length() + evt.message.length();
	send(socket_fd, &v, sizeof(v), 0);

	// Send timestamp
	send(socket_fd, &evt.tv, sizeof(evt.tv), 0);

	// Send log-level
	v = (unsigned short)evt.level;
	send(socket_fd, &v, sizeof(v), 0);

	// Send subsystem text
	v = (unsigned short)evt.subsystem.length();
	send(socket_fd, &v, sizeof(v), 0);
	send(socket_fd, evt.subsystem.c_str(), v, 0);

	// Send message text
	v = (unsigned short)evt.message.length();
	send(socket_fd, &v, sizeof(v), 0);
	send(socket_fd, evt.message.c_str(), v, 0);
}

SocketBackedLogTarget *SocketBackedLogTarget::ConnectUnixSocket(std::string path)
{
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "unable to create unix domain socket\n");
		perror("socket");
		return NULL;
	}

	struct sockaddr_un addr;
	bzero(&addr, sizeof(addr));

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

	if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "unable to connect to unix domain socket %s\n", path.c_str());
		perror("connect");
		close(fd);
		return NULL;
	}

	return new SocketBackedLogTarget(fd);
}

SocketBackedLogTarget *SocketBackedLogTarget::ConnectTcpSocket(std::string host, int port)
{
	struct hostent *hostent = gethostbyname(host.c_str());
	if (!hostent) {
		fprintf(stderr, "unable to resolve host %s\n", host.c_str());
		return NULL;
	}

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "unable to create tcp socket\n");
		perror("socket");
		return NULL;
	}

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	memcpy((void *)&addr.sin_addr.s_addr, (void *)hostent->h_addr, hostent->h_length);

	if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "unable to connect to tcp socket %s:%d\n", host.c_str(), port);
		perror("connect");
		close(fd);
		return NULL;
	}

	return new SocketBackedLogTarget(fd);
}
