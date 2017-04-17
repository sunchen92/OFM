#ifndef compress_H_GUARD
#define compress_H_GUARD

#include <stdbool.h>
#include <zlib.h>
#include <event2/buffer.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#define BUFF_SIZE 4096

#define DEBUG 0

/*
 * Encode chain
 *                             #ensures multiple of 3 (unless last chunk)
 * user_in_buff|gz_encode_strm|encode_gzout_buffer(evbuffer)|b64_encoder|encode_output_buffer(BIO)|user_out_buff
 *
 *
 * Decode chain
 *              #ensures multiple of 4 (unless last chunk)
 * user_in_buff|decode_input_buffer|b64_decoder|decode_output_buffer(evbuffer)|gz_decode_strm|user_out_buff 
 *
 * notes:
 * Whenever data is buffered, the prefered format is compressed. So when the encode_in function is called, the data is immediately
 * gzipped and stored until the the encode_out is called. When the encode_out is called, only then is the compressed data base64 
 * encoded.
 * The evbuffer functions are similar to the simple block functions, however they take advantage of the ability to move memory 
 * from one buffer to another minimizing memory copies on the user interface functions where possible. The input is assumed to 
 * be part of a json stream and positioned at a 'value'. Meaning it expects the actual data to be enclosed in double quotes (")
 * which will be stripped. It will move data from the user's evbuffer up to the terminating quote (removing it) of the json value. 
 * The output functions will write a complete json value with the sentinal quotes. 
 */

typedef struct gzb64_t {

	/* Encode chain components, in order of processing */
	z_stream gz_encode_strm;
	struct evbuffer *encode_gzout_buffer;
	BIO *b64_encoder;
	BIO *encode_output_buffer;

	/* Decode chain components, in order of processing */
	struct evbuffer *decode_input_buffer;
	BIO *b64_decoder;
	struct evbuffer *decode_output_buffer;
	z_stream gz_decode_strm;

	/* status bits */
	bool encoded_last_chunk;
	bool decoded_last_chunk;

} Gzb64;

/* These are parameters to deflateInit2. See
   http://zlib.net/manual.html for the exact meanings. */
#define windowBits 15
#define GZIP_ENCODING 16
#define ENABLE_ZLIB_GZIP 32

/** 
 * Interface object for gzip/base64 encoding/decoding
 */
Gzb64* gzb64_init();

/**
 * @param buf The data to encode
 * @param length The used portion of the input buffer
 * @param last indicate if last buffer in the stream
 * @return error code
 */
int encode_in(Gzb64* gzb64, const unsigned char* buf, const size_t length, const bool last);

int encode_in_ev(Gzb64* gzb64, struct evbuffer* input);

/* Encoded output 
 * @param buf The empty buffer to fill with encoded data
 * @param length The size of the empty buffer
 * @return Used size of the returned buffer, keep calling until 0
 * or < length
 */
int encode_out(Gzb64* gzb64, unsigned char* buf, const size_t length);

int encode_out_ev(Gzb64* gzb64, struct evbuffer* output);

/**
 * @param buf The buffer with data to decode
 * @param length The amount of the data in this buffer
 * @param last Indicate of last buffer to decode
 * @return return error code
 */
int decode_in(Gzb64* gzb64, const unsigned char* buf, const size_t length, bool last);

int decode_in_ev(Gzb64* gzb64, struct evbuffer* input);

/**
 * @param buf The buffer to fill with decoded data
 * @param length The size of the buffer to fill
 * @return The used size of the empty buffer, keep calling until
 * the return is 0 or < length
 */
int decode_out(Gzb64* gzb64, unsigned char* buf, const size_t length);

int decode_out_ev(Gzb64* gzb64, struct evbuffer* output);

/* Free structures */
void gzb64_free(Gzb64* gzb64);


#endif
