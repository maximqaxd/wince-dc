//
// vmustore.h - tiny persistence to the Dreamcast VMU via the CE Maple persistent store.
// Saves/loads one small blob (<= 512 bytes, one VMU block) in a file named DCWINSHC.BIN.
// Both return 1 on success and 0 on ANY failure (no VMU, unformatted, COM error); callers
// must treat 0 as "no saved data" and carry on with defaults.
//
#ifndef VMUSTORE_H
#define VMUSTORE_H

int VmuSave(const void *data, int len);             // len <= 512
int VmuLoad(void *data, int maxlen, int *outlen);   // *outlen = bytes read (may be NULL)

#endif // VMUSTORE_H
