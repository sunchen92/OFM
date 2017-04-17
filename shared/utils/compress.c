#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "compress.h"

static int decode(Gzb64* gzb64, bool last);

/* report a zlib or i/o error */
static void zerr(int ret)
{
    switch (ret) {
    case Z_ERRNO:
		fputs("zlib: ", stderr);
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
		exit(EXIT_FAILURE);
        break;
    case Z_STREAM_ERROR:
		fputs("zlib: ", stderr);
        fputs("invalid compression level\n", stderr);
		exit(EXIT_FAILURE);
        break;
    case Z_DATA_ERROR:
		fputs("zlib: ", stderr);
        fputs("invalid or incomplete deflate data\n", stderr);
		exit(EXIT_FAILURE);
        break;
    case Z_MEM_ERROR:
		fputs("zlib: ", stderr);
        fputs("out of memory\n", stderr);
		exit(EXIT_FAILURE);
        break;
    case Z_VERSION_ERROR:
		fputs("zlib: ", stderr);
        fputs("zlib version mismatch!\n", stderr);
		exit(EXIT_FAILURE);
    }
}

static void resetDecoder(Gzb64* gzb64)
{
	int zret = inflateReset(& gzb64->gz_decode_strm);
	if(zret < 0) zerr(zret);
	zret = BIO_reset(gzb64->b64_decoder);
	gzb64->decoded_last_chunk = false;
	if(DEBUG) printf("Decoder reset\n");
}

static void resetEncoder(Gzb64* gzb64)
{
    int zret = deflateReset(& gzb64->gz_encode_strm);
	if(zret < 0) zerr(zret);
	zret = BIO_reset(gzb64->encode_output_buffer);
	zret = BIO_reset(gzb64->b64_encoder);
	gzb64->encoded_last_chunk = false;
	if(DEBUG) printf("Encoder reset\n");
}

static void hex_debug(const unsigned char* buf, const int length) 
{
	for(int i = 0; i < length; i++)
		printf("%02X", buf[i]);

	if(length > 0 ) printf("\n");
}

Gzb64* gzb64_init()
{
	int zret;

	Gzb64* gzb64 = (Gzb64*) malloc(sizeof(Gzb64));
	if( gzb64 == NULL ) { fprintf(stderr, "Failed to malloc\n"); exit(EXIT_FAILURE); }

	/* Encode */
	/* setup zlib encode struc */
    gzb64->gz_encode_strm.zalloc = Z_NULL;
    gzb64->gz_encode_strm.zfree  = Z_NULL;
    gzb64->gz_encode_strm.opaque = Z_NULL;
    zret = deflateInit2 (& gzb64->gz_encode_strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                             windowBits | GZIP_ENCODING, 8,
                             Z_DEFAULT_STRATEGY);
	if(zret < 0) zerr(zret);

	gzb64->encode_gzout_buffer = evbuffer_new();

	/* setup base64 encoder */
	gzb64->b64_encoder = BIO_new(BIO_f_base64());
	BIO_set_flags(gzb64->b64_encoder, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
	gzb64->encode_output_buffer = BIO_new(BIO_s_mem());
	BIO_set_mem_eof_return(gzb64->encode_output_buffer, 0);
    gzb64->b64_encoder = BIO_push(gzb64->b64_encoder, gzb64->encode_output_buffer);

	/* Decode */
	gzb64->decode_input_buffer = evbuffer_new();

	/* setup base64 decoder */
	gzb64->b64_decoder = BIO_new(BIO_f_base64());
	BIO_set_flags(gzb64->b64_decoder, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer

	gzb64->decode_output_buffer = evbuffer_new();

	/* setup zlib decode struc */
    gzb64->gz_decode_strm.avail_in = 0;
    gzb64->gz_decode_strm.next_in = Z_NULL;
	zret = inflateInit2 (& gzb64->gz_decode_strm, windowBits | ENABLE_ZLIB_GZIP);
	if(zret < 0) zerr(zret);

	/* State */
	gzb64->encoded_last_chunk = false;
	gzb64->decoded_last_chunk = false;

	return gzb64;
}

int encode_in_ev(Gzb64* gzb64, struct evbuffer* input)
{
	int contiguous;
	unsigned char* input_block;

	while(evbuffer_get_length(input) > 0) 
	{
		contiguous = evbuffer_get_contiguous_space(input);	
		input_block = evbuffer_pullup(input, contiguous);

		if( contiguous < evbuffer_get_length(input) ) 
		{
			encode_in(gzb64, &input_block[0], contiguous, false);
			evbuffer_drain(input, contiguous);			
		} else 
		{
			/* last (only) block */
			encode_in(gzb64, &input_block[0], contiguous, true);
			evbuffer_drain(input, contiguous);
		}
	}
	
	return 0;
}

int encode_in(Gzb64* gzb64, const unsigned char* buf, const size_t length, const bool last)
{
	int flush_type, zret;
	struct evbuffer_iovec v[2];
	int n, i, written;
	size_t n_to_add = BUFF_SIZE;

	if(last) {
		flush_type = Z_FINISH;
		gzb64->encoded_last_chunk = true;
	} else {
		flush_type = Z_NO_FLUSH;	
	}

	gzb64->gz_encode_strm.next_in = buf;
	gzb64->gz_encode_strm.avail_in = length;

	/* loop as long as more input available */
	while( 0 != gzb64->gz_encode_strm.avail_in || Z_FINISH == flush_type )
	{
		/* Reserve BUFF_SIZE bytes.*/
		n = evbuffer_reserve_space(gzb64->encode_gzout_buffer, n_to_add, v, 2);
		if (n<=0)
		   return -1; /* Unable to reserve the space for some reason. */

		for (i=0; i<n && n_to_add > 0; ++i) {
		   size_t len = v[i].iov_len;
		   if (len > n_to_add) /* Don't write more than n_to_add bytes. */
			  len = n_to_add;

			gzb64->gz_encode_strm.avail_out = len;
			gzb64->gz_encode_strm.next_out = v[i].iov_base;
			
			zret = deflate (& gzb64->gz_encode_strm, flush_type);
			if(zret < 0) zerr(zret);

			written = len - gzb64->gz_encode_strm.avail_out;
			if(DEBUG) printf("Deflate Out:%d\n", written);

		  /* If there was a problem during data generation, we can just stop
			 here; no data will be committed to the buffer. */

		   /* Set iov_len to the number of bytes we actually wrote, so we
			  don't commit too much. */
		   v[i].iov_len = written;
		}

		/* We commit the space here.  Note that we give it 'i' (the number of
		   vectors we actually used) rather than 'n' (the number of vectors we
		   had available. */
		if (evbuffer_commit_space(gzb64->encode_gzout_buffer, v, i) < 0)
		   return -1; /* Error committing */

		if( Z_FINISH == flush_type && 0 == written ) break;

	} 

	return 0;	
}

int decode_in_ev(Gzb64* gzb64, struct evbuffer* input)
{
	int contiguous;

	while(evbuffer_get_length(input) > 0 ) 
	{
		contiguous = evbuffer_get_contiguous_space(input);	

		if( contiguous < evbuffer_get_length(input) ) 
		{
			evbuffer_remove_buffer(input, gzb64->decode_input_buffer, contiguous);
			decode(gzb64, false);			
		} else 
		{
			/* last (only) block */
			evbuffer_remove_buffer(input, gzb64->decode_input_buffer, contiguous);
		}
	}

	return decode(gzb64, true);
}

int decode_in(Gzb64* gzb64, const unsigned char* buf, const size_t length, bool last)
{
	evbuffer_add(gzb64->decode_input_buffer, buf, length);
	return decode(gzb64, last);
}

static int decode(Gzb64* gzb64, bool last)
{
	int b64in_size;
	struct evbuffer_iovec v[2];
	int n, i;
	size_t n_to_add = BUFF_SIZE;

	if( last ) {
		b64in_size = evbuffer_get_length(gzb64->decode_input_buffer); /* dump everything in */		
		gzb64->decoded_last_chunk = true;	
	} else {
		int contiguous = evbuffer_get_contiguous_space(gzb64->decode_input_buffer);	
		b64in_size = contiguous - (contiguous % 4); /* must be mutliple of 4 */	 
	}

	if(DEBUG) printf("Decode In:%zu\n", evbuffer_get_length(gzb64->decode_input_buffer));

	unsigned char* b64in_buf = evbuffer_pullup(gzb64->decode_input_buffer, b64in_size);
	BIO *b64_src_temp = BIO_new_mem_buf(b64in_buf, b64in_size);
	gzb64->b64_decoder = BIO_push(gzb64->b64_decoder, b64_src_temp);

	int b64_output_len = BUFF_SIZE;

	while(b64_output_len == BUFF_SIZE) {

		/* Reserve BUFF_SIZE bytes.*/
		n = evbuffer_reserve_space(gzb64->decode_output_buffer, n_to_add, v, 2);
		if (n<=0)
		   return -1; /* Unable to reserve the space for some reason. */

		for (i=0; i<n && n_to_add > 0; ++i) {
		   size_t len = v[i].iov_len;
		   if (len > n_to_add) /* Don't write more than n_to_add bytes. */
			  len = n_to_add;

		   b64_output_len = BIO_read(gzb64->b64_decoder, v[i].iov_base, len);

		   if ( b64_output_len < 0) {
			  /* If there was a problem during data generation, we can just stop
				 here; no data will be committed to the buffer. */
			  return -1;
		   }
		   /* Set iov_len to the number of bytes we actually wrote, so we
			  don't commit too much. */
		   v[i].iov_len = b64_output_len;
			if(DEBUG) printf("Decode B64 Out:%d\n", b64_output_len);
		}

		/* We commit the space here.  Note that we give it 'i' (the number of
		   vectors we actually used) rather than 'n' (the number of vectors we
		   had available. */
		if (evbuffer_commit_space(gzb64->decode_output_buffer, v, i) < 0)
		   return -1; /* Error committing */
	}

	if(DEBUG) printf("Inflate In:%zu\n", evbuffer_get_length(gzb64->decode_output_buffer));

	BIO_pop(gzb64->b64_decoder);
	BIO_free(b64_src_temp);
	BIO_reset(gzb64->b64_decoder);

	evbuffer_drain(gzb64->decode_input_buffer, b64in_size);
	
	return 0;
}

int decode_out_ev(Gzb64* gzb64, struct evbuffer* output)
{
	struct evbuffer_iovec v[2];
	int n, i, written;
	size_t n_to_add = BUFF_SIZE;

	/* Reserve BUFF_SIZE bytes.*/
	n = evbuffer_reserve_space(output, n_to_add, v, 2);
	if (n<=0)
	   return -1; /* Unable to reserve the space for some reason. */

	for (i=0; i<n && n_to_add > 0; ++i) {
	   size_t len = v[i].iov_len;
	   if (len > n_to_add) /* Don't write more than n_to_add bytes. */
		  len = n_to_add;
	   if ( (written = decode_out(gzb64, v[i].iov_base, len)) < 0) {
		  /* If there was a problem during data generation, we can just stop
		     here; no data will be committed to the buffer. */
		  return -1;
	   }
	   /* Set iov_len to the number of bytes we actually wrote, so we
		  don't commit too much. */
	   v[i].iov_len = written;
	}

	/* We commit the space here.  Note that we give it 'i' (the number of
	   vectors we actually used) rather than 'n' (the number of vectors we
	   had available. */
	if (evbuffer_commit_space(output, v, i) < 0)
	   return -1; /* Error committing */
	
	/* recurse if some data was recieved */
	if( written > 0 )
		return decode_out_ev(gzb64, output);

	resetDecoder(gzb64);

	return 0;
}

int decode_out(Gzb64* gzb64, unsigned char* buf, const size_t length)
{
	int zret, ret;

	/* should be guaranteed at least length after inflate */
	int gz_in_bytes = evbuffer_get_contiguous_space(gzb64->decode_output_buffer);
	unsigned char* gzin_buf = evbuffer_pullup(gzb64->decode_output_buffer, gz_in_bytes );

	if(DEBUG) hex_debug(gzin_buf, gz_in_bytes);
	
	gzb64->gz_decode_strm.next_in = gzin_buf;
	gzb64->gz_decode_strm.avail_in = gz_in_bytes;
	gzb64->gz_decode_strm.next_out = buf;
    gzb64->gz_decode_strm.avail_out = length;
	zret = inflate (& gzb64->gz_decode_strm, Z_NO_FLUSH);
	if(zret < 0) zerr(zret);

	evbuffer_drain(gzb64->decode_output_buffer, gz_in_bytes - gzb64->gz_decode_strm.avail_in);

	ret = length - gzb64->gz_decode_strm.avail_out;
	
	if(gzb64->decoded_last_chunk && 0 == ret) {
		resetDecoder(gzb64);
	}

	return ret;
}

int encode_out_ev(Gzb64* gzb64, struct evbuffer* output)
{

	struct evbuffer_iovec v[2];
	int n, i, written;
	size_t n_to_add = BUFF_SIZE;

	/* Reserve BUFF_SIZE bytes.*/
	n = evbuffer_reserve_space(output, n_to_add, v, 2);
	if (n<=0)
	   return -1; /* Unable to reserve the space for some reason. */

	for (i=0; i<n && n_to_add > 0; ++i) {
	   size_t len = v[i].iov_len;
	   if (len > n_to_add) /* Don't write more than n_to_add bytes. */
		  len = n_to_add;
	   if ( (written = encode_out(gzb64, v[i].iov_base, len)) < 0) {
		  /* If there was a problem during data generation, we can just stop
		     here; no data will be committed to the buffer. */
		  return -1;
	   }
	   /* Set iov_len to the number of bytes we actually wrote, so we
		  don't commit too much. */
	   v[i].iov_len = written;
	}

	/* We commit the space here.  Note that we give it 'i' (the number of
	   vectors we actually used) rather than 'n' (the number of vectors we
	   had available. */
	if (evbuffer_commit_space(output, v, i) < 0)
	   return -1; /* Error committing */

	/* recurse if some bytes were retrieved */
	if( written > 0 )
		return encode_out_ev(gzb64, output);
	
	resetEncoder(gzb64);

	return 0;
}

int encode_out(Gzb64* gzb64, unsigned char* buf, const size_t length)
{
	int gzout_size, ret;

	if( gzb64->encoded_last_chunk ) {
		gzout_size = evbuffer_get_length(gzb64->encode_gzout_buffer); /* dump everything in */		
	} else {
		int contiguous = evbuffer_get_contiguous_space(gzb64->encode_gzout_buffer);	
		gzout_size = contiguous - (contiguous % 3); /* must be mutliple of 3 */	 
	}

	if(gzout_size > 0) {
		unsigned char* gzout_buf = evbuffer_pullup(gzb64->encode_gzout_buffer, gzout_size);	
		int bytes_written = BIO_write(gzb64->b64_encoder, gzout_buf, gzout_size);
		evbuffer_drain(gzb64->encode_gzout_buffer, bytes_written);

		if(gzb64->encoded_last_chunk) ret = BIO_flush(gzb64->b64_encoder);	/* flush if last */
	}
		
	ret = BIO_read(gzb64->encode_output_buffer, buf, length);

	/* reset for next stream */
	if(gzb64->encoded_last_chunk && 0 == ret) {
		resetEncoder(gzb64);
	}

	return ret;
}

void gzb64_free(Gzb64* gzb64) 
{
	evbuffer_free(gzb64->encode_gzout_buffer);
	evbuffer_free(gzb64->decode_input_buffer);
	evbuffer_free(gzb64->decode_output_buffer);
	inflateEnd(&gzb64->gz_decode_strm);
	deflateEnd(&gzb64->gz_encode_strm);
	BIO_free_all(gzb64->b64_encoder);
	BIO_free_all(gzb64->b64_decoder);
	free(gzb64);	
}
