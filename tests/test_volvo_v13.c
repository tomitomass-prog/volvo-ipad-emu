#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "iap_diag.h"

static void check_reply(const iap_frame_t *request, uint16_t response, const char *needle) {
 iap_tx_t tx = {0};
 assert(iap_volvo_reply(request,&tx));
 assert(tx.len >= 7 && tx.bytes[1]==0x55);
 iap_frame_t decoded;
 assert(iap_decode(tx.bytes,tx.len,&decoded));
 assert(decoded.lingo==request->lingo && decoded.command==response);
 if (needle) assert(strstr((char *)decoded.payload,needle)!=NULL);
}
int main(void){
 iap_frame_t r = {.lingo=0,.command=7};
 check_reply(&r,8,"berbelucha");
 r.lingo=4; r.command=0x0018; r.payload_len=1; r.payload[0]=5;
 check_reply(&r,0x0019,NULL);
 r.command=0x0020; r.payload_len=4; memset(r.payload,0,4);
 check_reply(&r,0x0021,"berbelucha");
 r.command=0x0022; check_reply(&r,0x0023,"ESP32");
 r.command=0x0024; check_reply(&r,0x0025,"Volvo");
 r.command=0x001E; r.payload_len=0; check_reply(&r,0x001F,NULL);
 r.command=0x001C; check_reply(&r,0x001D,NULL);
 /* Round-trip and checksum rejection */
 iap_tx_t tx; r.lingo=0; r.command=7;
 assert(iap_volvo_reply(&r,&tx));
 tx.bytes[tx.len-1]^=1;
 iap_frame_t bad;
 assert(!iap_decode(tx.bytes,tx.len,&bad));
 puts("iAP v1.3 tests PASS");
}
