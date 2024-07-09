# cb64
C library for Base64 encoding/decoding

## How to use

Encoding example

```C
unsigned char* enc = b64_encode((const unsigned char*) "Hello", 5, NULL);
printf((const char*) enc);
free(enc);
```

Decoding example

```C
unsigned char* dec = b64_decode((const unsigned char*) "SGVsbG8=", 8, NULL);
printf((const char*) dec);
free(dec);
```
