#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "compress.h"

void testCompress(Gzb64* gzb64)
{
	char* test_string = "This is a simple test.\n";

	encode_in(gzb64, test_string, strlen(test_string), false);
	encode_in(gzb64, test_string, strlen(test_string), true);

	unsigned char encoded[4096];
	unsigned char decoded[4096];
	int len;

	while ( 0 != ( len = encode_out(gzb64, &encoded[0], sizeof(encoded)-1) ) ) {
		encoded[len] = '\0';
		printf("%s\n", &encoded[0]);

		/* pass encoded to decode routine */		
		decode_in(gzb64, &encoded[0], len, false);
	}
  
	decode_in(gzb64, &encoded[0], 0, true); /* end of decode input */

	while( 0 != ( len = decode_out(gzb64, &decoded[0], sizeof(decoded)-1) ) ) {
		decoded[len] = '\0';
		printf("%s", decoded);	
	} 

}

void testEVCompress(Gzb64* gzb64) 
{
	struct evbuffer* input_buff = evbuffer_new();
	struct evbuffer* output_buff = evbuffer_new();

	evbuffer_add_printf(input_buff, "This is an evbuffer test!\n");

	int length = evbuffer_get_length(input_buff);

	encode_in_ev(gzb64, input_buff);
	assert(0 == evbuffer_get_length(input_buff));

	encode_out_ev(gzb64, output_buff);
	assert(evbuffer_get_length(output_buff) > 0);

	/* print encoded */
	fwrite(evbuffer_pullup(output_buff, evbuffer_get_length(output_buff)), 
           evbuffer_get_length(output_buff),
           1, stdout);
	printf("\n");
	fflush(stdout);

	decode_in_ev(gzb64, output_buff);
	assert(0 == evbuffer_get_length(output_buff));

	decode_out_ev(gzb64, output_buff);
	assert(evbuffer_get_length(output_buff) == length);

	/* print decoded */
	fwrite(evbuffer_pullup(output_buff, evbuffer_get_length(output_buff)), 
           evbuffer_get_length(output_buff),
           1, stdout);
	fflush(stdout);

	evbuffer_free(input_buff);
	evbuffer_free(output_buff);
  
}

int
main(int argc, char *argv[])
{

	int n,out_bytes, ret;
	unsigned char bucketA[4096];
	unsigned char bucketB[4096];

	Gzb64* gzb64 = gzb64_init();

	if(argc == 1) {
		/* no args, simple tests */
		testCompress(gzb64);
		testCompress(gzb64);

		testEVCompress(gzb64);
		testEVCompress(gzb64);
	} else if (argc == 2 && strcmp(argv[1], "-d") == 0 ) {

		/* decode */	
		while(  (n = fread(bucketA, sizeof(unsigned char), sizeof(bucketA), stdin) ) > 0 ) {
			if(n < sizeof(bucketA)) {
				decode_in(gzb64, bucketA, n, true);
			} else {
				decode_in(gzb64, bucketA, n, false);
			}
			
			while( (out_bytes = decode_out(gzb64, bucketB, sizeof(bucketB))) > 0 ){
				fwrite(bucketB, sizeof(unsigned char), out_bytes, stdout);
				fflush(stdout);
			}
			
		}

		/* end of input */
		decode_in(gzb64, NULL, 0, true);	
		while( (out_bytes = decode_out(gzb64, bucketB, sizeof(bucketB))) > 0 ){
			fwrite(bucketB, sizeof(unsigned char), out_bytes, stdout);
			fflush(stdout);
		}
	
	} else if (argc == 2 && strcmp(argv[1], "-e") == 0 ) {
		
		/* encode */	
		while(  (n = fread(bucketA, sizeof(unsigned char), sizeof(bucketA), stdin) ) > 0 ) {
			if(n < sizeof(bucketA)) {
				ret = encode_in(gzb64, bucketA, n, true);
			} else {
				ret = encode_in(gzb64, bucketA, n, false);
			}
			
			while( (out_bytes = encode_out(gzb64, bucketB, sizeof(bucketB))) > 0 ){
				fwrite(bucketB, sizeof(unsigned char), out_bytes, stdout);
				fflush(stdout);
			}				
		}
		
		/* end of input */
		encode_in(gzb64, NULL, 0, true);
	
		/* grab any remaining output */	
		while( (out_bytes = encode_out(gzb64, bucketB, sizeof(bucketB))) > 0 ){
			fwrite(bucketB, sizeof(unsigned char), out_bytes, stdout);
			fflush(stdout);
		}	
			
		printf("\n");
	} else {
		printf("Usage: %s [-d] [-e]\n", argv[0]);
		printf("    -d       decode - stdin -> stdout\n");
		printf("    -e       encode - stdin -> stdout\n");
		printf("    no args  simple tests\n");
	}

	gzb64_free(gzb64);
	
	return 0;
}
