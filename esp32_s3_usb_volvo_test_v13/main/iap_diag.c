#include "iap_diag.h"
#include <string.h>

/* iAP1 HID frame: optional 00, 55, length, lingo, command, data, checksum.
 * General lingo has one-byte commands; Extended Interface lingo 04 two.
 */
bool iap_decode(const uint8_t *buf, size_t len, iap_frame_t *out) {
  if (!buf || !out || len < 5) return false;
  size_t at = (buf[0] == 0 && len > 1) ? 1u : 0u;
  if (buf[at] != 0x55 || len - at < 5u) return false;
  uint8_t blen = buf[at + 1];
  if (blen < 2 || blen > 58) return false;
  size_t used = at + 2u + blen + 1u;
  if (used > len) return false;
  uint8_t sum = 0;
  for (size_t i = at + 1; i < used; ++i) sum = (uint8_t)(sum + buf[i]);
  if (sum) return false;
  memset(out, 0, sizeof(*out));
  out->lingo = buf[at + 2];
  const bool ext = out->lingo == 4;
  unsigned cs = ext ? 2u : 1u;
  if (blen < 1u + cs) return false;
  out->command = ext ? ((uint16_t)buf[at+3]<<8 | buf[at+4]) : buf[at+3];
  out->payload_len = blen - 1u - cs;
  if (out->payload_len > sizeof(out->payload)) return false;
  if (out->payload_len) memcpy(out->payload,buf+at+3+cs,out->payload_len);
  return true;
}

static bool reply(uint8_t lingo, uint16_t cmd, const uint8_t *data,
                  size_t sz, iap_tx_t *out) {
  size_t cs = (lingo == 4) ? 2u : 1u;
  if (!out || sz > IAP_MAX_FRAME - (5u + cs)) return false;
  memset(out,0,sizeof(*out));
  size_t at = 0;
  out->bytes[at++] = 0;
  out->bytes[at++] = 0x55;
  out->bytes[at++] = (uint8_t)(1u+cs+sz);
  out->bytes[at++] = lingo;
  if (cs == 2) out->bytes[at++] = (uint8_t)(cmd >> 8);
  out->bytes[at++] = (uint8_t)cmd;
  if (sz) { memcpy(out->bytes+at,data,sz); at += sz; }
  uint8_t sum = 0;
  for (size_t i=2;i<at;i++) sum=(uint8_t)(sum+out->bytes[i]);
  out->bytes[at++] = (uint8_t)(0-sum);
  out->len=at;
  return true;
}

bool iap_volvo_reply(const iap_frame_t *in, iap_tx_t *out) {
  if (!in || !out) return false;
  if (in->lingo == 0) {
    static const uint8_t name[] = "berbelucha";
    static const uint8_t version[] = {2,0,1};
    static const uint8_t serial[] = "S3VOLVO13";
    static const uint8_t model[] = {0,0,0,0,'i','P','o','d',0};
    switch (in->command) {
      case 0x07: return reply(0,0x08,name,sizeof(name),out);
      case 0x09: return reply(0,0x0A,version,sizeof(version),out);
      case 0x0B: return reply(0,0x0C,serial,sizeof(serial),out);
      case 0x0D: return reply(0,0x0E,model,sizeof(model),out);
      default: return false;
    }
  }
  if (in->lingo != 4) return false;
  /* Partial Extended Interface lingo.  Single virtual song with index 0.
   * These responses are intentionally small. More identification/authentication
   * messages may be required by a car head unit before it shows any title.
   */
  static const uint8_t one[] = {0,0,0,1};
  static const uint8_t zero[] = {0,0,0,0};
  static const uint8_t track[] = "berbelucha";
  static const uint8_t artist[] = "ESP32-S3";
  static const uint8_t album[] = "Volvo USB Test";
  switch (in->command) {
    case 0x0018: /* GetNumberCategorizedDBRecords(category...) */
      if (in->payload_len < 1) return false;
      /* 0x01 playlists, 0x02 artists, 0x03 albums, 0x05 tracks */
      if (in->payload[0] == 1 || in->payload[0] == 2 ||
          in->payload[0] == 3 || in->payload[0] == 5)
        return reply(4,0x0019,one,sizeof(one),out);
      return reply(4,0x0019,zero,sizeof(zero),out);
    case 0x001E: /* GetCurrentPlayingTrackIndex */
      return reply(4,0x001F,zero,sizeof(zero),out);
    case 0x0020: /* GetIndexedPlayingTrackTitle */
      if (in->payload_len < 4 || memcmp(in->payload,zero,4)) return false;
      return reply(4,0x0021,track,sizeof(track),out);
    case 0x0022: /* GetIndexedPlayingTrackArtistName */
      if (in->payload_len < 4 || memcmp(in->payload,zero,4)) return false;
      return reply(4,0x0023,artist,sizeof(artist),out);
    case 0x0024: /* GetIndexedPlayingTrackAlbumName */
      if (in->payload_len < 4 || memcmp(in->payload,zero,4)) return false;
      return reply(4,0x0025,album,sizeof(album),out);
    case 0x001C: { /* GetPlayStatus -> ReturnPlayStatus (duration,position,playing) */
      static const uint8_t status[] = {0,0,0,0, 0,0,0,0, 1};
      return reply(4,0x001D,status,sizeof(status),out);
    }
    default: return false;
  }
}
