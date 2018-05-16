#include <libgvnc/net/Socket.h>
#include <libgvnc/net/SocketException.h>
#include <libgvnc/net/EndPoint.h>

#include <sys/socket.h>
#include <unistd.h>

using namespace libgvnc::net;

/**
 * Creates a new Socket object, with the given socket parameters.
 * @param addressFamily The address family of the socket.
 * @param socketType The type of socket.
 * @param protocolType The protocol associated with the socket.
 */
Socket::Socket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType)
		: addressFamily_(addressFamily),
		socketType_(socketType),
		protocolType_(protocolType_),
		fd_(-1)
{
	// Construct the native socket object.
	fd_ = ::socket((int)addressFamily, (int)socketType, (int)protocolType);
	
	// Check the file descriptor, and throw an exception if the operation failed.
	if (fd_ < 0) {
		throw SocketException("Unable to create socket");
	}
}

Socket::Socket(int native_fd, const EndPoint& remoteEndPoint)
		: addressFamily_(remoteEndPoint.GetAddressFamily()),
		socketType_(SocketType::DATAGRAM),
		protocolType_(ProtocolType::NONE),
		fd_(native_fd)
{
	
}

/**
 * Ensures the native socket is closed when the object is destroyed.
 */
Socket::~Socket()
{
	// Run the close method - which is safe even if the socket is already closed.
	Close();
}

/**
 * Closes the socket.
 */
void Socket::Close()
{
	// If there is a valid file descriptor, close it and invalid the file descriptor.
	if (fd_ >= 0) {
		::close(fd_);
		fd_ = -1;
	}
}

/**
 * Accepts a new connection on a listening socket.
 * @return A socket object representing an accepted connection.
 */
Socket* Socket::Accept()
{
	EnsureNotClosed();
	
	int client_fd = ::accept(fd_, nullptr, 0);
	if (client_fd < 0) {
		throw SocketException("Accept Failed");
	}
	
	IPEndPoint rep(IPAddress::Any, 0);
	return new Socket(client_fd, rep);
}

/**
 * Binds the socket to a particular endpoint.  The endpoint object must have the
 * same address family as the socket.
 * @param endpoint The endpoint to bind the socket to.
 */
void Socket::Bind(const EndPoint& endpoint)
{
	EnsureNotClosed();
	
	// Check that the address family of the endpoint is equal to the address
	// family of this listening socket.
	if (endpoint.GetAddressFamily() != addressFamily_) {
		throw SocketException("Unable to bind to endpoint with incompatible address family");
	}
	
	// Obtain the raw sockaddr object, and bind the native socket.
	SockAddrContainer sa = endpoint.GetSockAddr();
	if (::bind(fd_, sa.GetSockAddr(), sa.GetSize()) < 0) {
		throw SocketException("Unable to bind to endpoint");
	}
}

/**
 * Sets up this socket for listening.
 * @param backlog The maximum number of pending client sockets.
 */
void Socket::Listen(int backlog)
{
	EnsureNotClosed();
	
	// Enable listening on the socket.
	if (::listen(fd_, backlog) < 0) {
		throw SocketException("Unable to listen");
	}
}

/**
 * Reads data of at most size from the socket into the buffer.  This function returns
 * the number of bytes actually read into the buffer.
 * @param buffer The buffer to read into.
 * @param size The maximum size of the buffer.
 * @return Returns the number of bytes read into the buffer, zero if the socket
 * is closed, and -1 if an error occurred.
 */
int Socket::Read(void* buffer, size_t size)
{
	EnsureNotClosed();
	return ::read(fd_, buffer, size);
}

/**
 * Writes data of at most size from the buffer into the socket.  This function returns
 * the number of bytes actually written to the socket.
 * @param buffer The buffer containing the data to write.
 * @param size The size of the buffer.
 * @return Returns the number of bytes written to the socket, zero if the socket
 * was closed, and -1 if an error occurred.
 */
int Socket::Write(const void* buffer, size_t size)
{
	EnsureNotClosed();
	return ::write(fd_, buffer, size);
}

/**
 * Ensures that the socket is not in a closed state.
 */
void Socket::EnsureNotClosed()
{
	if (fd_ < 0) {
		throw SocketException("Socket is closed");
	}
}
