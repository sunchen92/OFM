// Implements non-blocking chunk-wise I/O.

#ifndef CHUNKEDIO_H
#define CHUNKEDIO_H

#include "config.h"
#include "List.h"
#include "util.h"

#include <list>

#ifdef NEED_KRB5_H
# include <krb5.h>
#endif 

#include <openssl/ssl.h>
#include <openssl/err.h>

class CompressedChunkedIO;

// #define DEBUG_COMMUNICATION 10

// Abstract base class.
class ChunkedIO {
public:
	ChunkedIO();
	virtual ~ChunkedIO()	{ }

	typedef struct {
		char* data;
		uint32 len;
	} Chunk;

	// Initialization before any I/O operation is performed. Returns false
	// on any form of error.
	virtual bool Init()	{ return true; }

	// Tries to read the next chunk of data. If it can be read completely,
	// a pointer to it is returned in 'chunk' (ownership of chunk is
	// passed).  If not, 'chunk' is set to nil.  Returns false if any
	// I/O error occurred (use Eof() to see if it's an end-of-file).
	// If 'may_block' is true, we explicitly allow blocking.
	virtual bool Read(Chunk** chunk, bool may_block = false) = 0;

	// Puts the chunk into the write queue and writes as much data
	// as possible (takes ownership of chunk).
	// Returns false on any I/O error.
	virtual bool Write(Chunk* chunk) = 0;

	// Tries to write as much as currently possible.
	// Returns false on any I/O error.
	virtual bool Flush() = 0;

	// If an I/O error has been encountered, returns a string describing it.
	virtual const char* Error() = 0;

	// Return true if there is currently at least one chunk available
	// for reading.
	virtual bool CanRead() = 0;

	// Return true if there is currently at least one chunk waiting to be
	// written.
	virtual bool CanWrite() = 0;

	// Returns true if source believes that there won't be much data soon.
	virtual bool IsIdle() = 0;

	// Returns true if internal write buffers are about to fill up.
	virtual bool IsFillingUp() = 0;

	// Throws away buffered data.
	virtual void Clear() = 0;

	// Returns true,if end-of-file has been reached.
	virtual bool Eof() = 0;

	// Returns underlying fd if available, -1 otherwise.
	virtual int Fd()	{ return -1; }

	// Makes sure that no additional protocol data is written into
	// the output stream.  If this is activated, the output cannot
	// be read again by any of these classes!
	void MakePure()	{ pure = true; }
	bool IsPure()	{ return pure; }

	// Writes a log message to the error_fd.
	void Log(const char* str);

	struct Statistics {
		Statistics()
			{
			bytes_read = 0;
			bytes_written = 0;
			chunks_read = 0;
			chunks_written = 0;
			reads = 0;
			writes = 0;
			pending = 0;
			}

		unsigned long bytes_read;
		unsigned long bytes_written;
		unsigned long chunks_read;
		unsigned long chunks_written;
		unsigned long reads;	// # calls which transferred > 0 bytes
		unsigned long writes;
		unsigned long pending;
		};

	// Returns raw statistics.
	const Statistics* Stats() const 	{ return &stats; }

	// Puts a formatted string containing statistics into buffer.
	virtual void Stats(char* buffer, int length);

#ifdef DEBUG_COMMUNICATION
	void DumpDebugData(const char* basefnname, bool want_reads);
#endif

protected:
	void InternalError(const char* msg)
		// We can't use the reporter here as we might be running in a
		// sub-process.
		{ fprintf(stderr, "%s", msg); abort(); }

	Statistics stats;
	const char* tag;

#ifdef DEBUG_COMMUNICATION
	void AddToBuffer(char* data, bool is_read)
		{ AddToBuffer(strlen(data), data, is_read); }
	void AddToBuffer(uint32 len, char* data, bool is_read);
	void AddToBuffer(Chunk* chunk, bool is_read);
	std::list<Chunk*> data_read;
	std::list<Chunk*> data_written;
#endif

private:
	bool pure;
};

// Chunked I/O using a file descriptor.
class ChunkedIOFd : public ChunkedIO {
public:
	// fd is an open bidirectional file descriptor, tag is used in error
	// messages, and pid gives a pid to monitor (if the process dies, we
	// return EOF).
	ChunkedIOFd(int fd, const char* tag, pid_t pid = 0);
	virtual ~ChunkedIOFd();

	virtual bool Read(Chunk** chunk, bool may_block = false);
	virtual bool Write(Chunk* chunk);
	virtual bool Flush();
	virtual const char* Error();
	virtual bool CanRead();
	virtual bool CanWrite();
	virtual bool IsIdle();
	virtual bool IsFillingUp();
	virtual void Clear();
	virtual bool Eof()	{ return eof; }
	virtual int Fd()	{ return fd; }
	virtual void Stats(char* buffer, int length);

private:

	bool PutIntoWriteBuffer(Chunk* chunk);
	bool FlushWriteBuffer();
	Chunk* ExtractChunk();

	// Returns size of next chunk in buffer or 0 if none.
	uint32 ChunkAvailable();

	// Flushes if it thinks it is time to.
	bool OptionalFlush();

	// Concatenates the the data of the two chunks forming a new one.
	// The old chunkds are deleted.
	Chunk* ConcatChunks(Chunk* c1, Chunk* c2);

	// Reads/writes on chunk of upto BUFFER_SIZE bytes.
	bool WriteChunk(Chunk* chunk, bool partial);
	bool ReadChunk(Chunk** chunk, bool may_block);

	int fd;
	bool eof;
	double last_flush;
	int failed_reads;

	// Optimally, this should match the file descriptor's
	// buffer size (for sockets, it may be helpful to
	// increase the send/receive buffers).
	static const unsigned int BUFFER_SIZE = 1024 * 1024 * 1;

	// We 'or' this to the length of a data chunk to mark
	// that it's part of a larger one. This has to be larger
	// than BUFFER_SIZE.
	static const uint32 FLAG_PARTIAL = 0x80000000;

	// We report that we're filling up when there are more than this number
	// of pending chunks.
	static const uint32 MAX_BUFFERED_CHUNKS_SOFT = 400000;

	// Maximum number of chunks we store in memory before rejecting writes.
	static const uint32 MAX_BUFFERED_CHUNKS = 500000;

	char* read_buffer;
	uint32 read_len;
	uint32 read_pos;
	Chunk* partial;	// when we read an oversized chunk, we store it here

	char* write_buffer;
	uint32 write_len;
	uint32 write_pos;

	struct ChunkQueue {
		Chunk* chunk;
		ChunkQueue* next;
	};

	// Chunks that don't fit into our write buffer.
	ChunkQueue* pending_head;
	ChunkQueue* pending_tail;

	pid_t pid;
};

// Chunked I/O using an SSL connection.
class ChunkedIOSSL : public ChunkedIO {
public:
	// Argument is an open socket and a flag indicating whether we are the
	// server side of the connection.
	ChunkedIOSSL(int socket, bool server);
	virtual ~ChunkedIOSSL();

	virtual bool Init();
	virtual bool Read(Chunk** chunk, bool mayblock = false);
	virtual bool Write(Chunk* chunk);
	virtual bool Flush();
	virtual const char* Error();
	virtual bool CanRead();
	virtual bool CanWrite();
	virtual bool IsIdle();
	virtual bool IsFillingUp();
	virtual void Clear();
	virtual bool Eof()	{ return eof; }
	virtual int Fd()	{ return socket; }
	virtual void Stats(char* buffer, int length);

private:
	// Maximum number of chunks we store in memory before rejecting writes.
	static const uint32 MAX_BUFFERED_CHUNKS = 500000;

	// Only returns true if all data has been read. If not, call
	// it again with the same parameters as long as error is not
	// set to true.
	bool ReadData(char* p, uint32 len, bool* error);
	// Same for writing.
	bool WriteData(char* p, uint32 len, bool* error);

	int socket;
	int last_ret;	// last error code
	bool eof;

	bool server;	// are we the server?
	bool setup;	// has the connection been setup successfully?

	SSL* ssl;

	// Write queue.
	struct Queue {
		Chunk* chunk;
		Queue* next;
	};

	// The chunk part we are reading/writing
	enum State { LEN, DATA };

	State write_state;
	Queue* write_head;
	Queue* write_tail;

	State read_state;
	Chunk* read_chunk;
	char* read_ptr;

	// One SSL for all connections.
	static SSL_CTX* ctx;
};

#include <zlib.h>

// Wrapper class around a another ChunkedIO which the (un-)compresses data.
class CompressedChunkedIO : public ChunkedIO {
public:
	CompressedChunkedIO(ChunkedIO* arg_io)
		: io(arg_io) {} // takes ownership
	virtual ~CompressedChunkedIO()	{ delete io; }

	virtual bool Init(); // does *not* call arg_io->Init()
	virtual bool Read(Chunk** chunk, bool may_block = false);
	virtual bool Write(Chunk* chunk);
	virtual bool Flush()	{ return io->Flush(); }
	virtual const char* Error()	{ return error ? error : io->Error(); }
	virtual bool CanRead()	{ return io->CanRead(); }
	virtual bool CanWrite()	{ return io->CanWrite(); }
	virtual bool IsIdle()	{ return io->IsIdle(); }
	virtual bool IsFillingUp()	{ return io->IsFillingUp(); }
	virtual void Clear()	{ return io->Clear(); }

	virtual bool Eof()	{ return io->Eof(); }
	virtual int Fd()	{ return io->Fd(); }
	virtual void Stats(char* buffer, int length);

	void EnableCompression(int level)
		{ deflateInit(&zout, level); compress = true; }
	void EnableDecompression()
		{ inflateInit(&zin); uncompress = true; }

protected:
	// Only compress block with size >= this.
	static const unsigned int MIN_COMPRESS_SIZE = 30;

	ChunkedIO* io;
	z_stream zin;
	z_stream zout;
	const char* error;

	bool compress;
	bool uncompress;

	// Keep some statistics.
	unsigned long uncompressed_bytes_read;
	unsigned long uncompressed_bytes_written;
};

#endif
