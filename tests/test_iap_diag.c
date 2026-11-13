#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "iap_diag.h"
static size_t request(uint8_t *b, uint8_t cmd) {
    b[0]=0; b[1]=0x55; b[2]=2; b[3]=0; b[4]=cmd;
    b[5]=(uint8_t)(0-(uint8_t)(2+cmd));
    return 6;
}
int main(void) {
    uint8_t b[64] = {0};
    iap_frame_t in; iap_tx_t out;
    size_t n=request(b,0x07);
    assert(iap_decode(b,n,&in)); assert(in.lingo==0 && in.command==7);
    assert(iap_identity_reply(&in,&out));
    assert(out.bytes[4]==0x08 && out.len <= 63);
    iap_frame_t decoded;
    assert(iap_decode(out.bytes,out.len,&decoded));
    assert(decoded.command == 0x08);
    b[5] ^= 1; assert(!iap_decode(b,n,&in));
    n=request(b,0x09); assert(iap_decode(b,n,&in));
    assert(iap_identity_reply(&in,&out));
    assert(out.bytes[4]==0x0A);
    n=request(b,0x44); assert(iap_decode(b,n,&in));
    assert(!iap_identity_reply(&in,&out));
    b[2]=62; assert(!iap_decode(b,n,&in));
    puts("iAP diagnostic parser tests OK");
    return 0;
}
