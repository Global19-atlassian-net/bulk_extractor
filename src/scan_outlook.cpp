/*
 * scan_outlook: optimistically decrypt Outlook Compressible Encryption
 * author:   Simson L. Garfinkel
 * created:  2014-01-28
 */
#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "utils.h"

/*
 * Below is the decoding array, i.e.:
 *
 * unsigned char plaintext_c = libpff_encryption_compressible[ ciphertext_c ];
 *
 * This comes from Herr Metz's libpff/libpff_encryption.c, under LGPL.
 * 
 * Outlook also has a "high compression" scheme, that's basically a
 * salted three-rotor scheme.
 *
 * We do not implement it here.
 *
 * Inverse map to encrypt:
 */

static const uint8_t libpff_encryption_compressible[] = {
    0x41, 0x36, 0x13, 0x62, 0xA8, 0x21, 0x6E, 0xBB,
    0xF4, 0x16, 0xCC, 0x04, 0x7F, 0x64, 0xE8, 0x5D,
    0x1E, 0xF2, 0xCB, 0x2A, 0x74, 0xC5, 0x5E, 0x35,
    0xD2, 0x95, 0x47, 0x9E, 0x96, 0x2D, 0x9A, 0x88,
    0x4C, 0x7D, 0x84, 0x3F, 0xDB, 0xAC, 0x31, 0xB6,
    0x48, 0x5F, 0xF6, 0xC4, 0xD8, 0x39, 0x8B, 0xE7,
    0x23, 0x3B, 0x38, 0x8E, 0xC8, 0xC1, 0xDF, 0x25,
    0xB1, 0x20, 0xA5, 0x46, 0x60, 0x4E, 0x9C, 0xFB,
    0xAA, 0xD3, 0x56, 0x51, 0x45, 0x7C, 0x55, 0x00,
    0x07, 0xC9, 0x2B, 0x9D, 0x85, 0x9B, 0x09, 0xA0,
    0x8F, 0xAD, 0xB3, 0x0F, 0x63, 0xAB, 0x89, 0x4B,
    0xD7, 0xA7, 0x15, 0x5A, 0x71, 0x66, 0x42, 0xBF,
    0x26, 0x4A, 0x6B, 0x98, 0xFA, 0xEA, 0x77, 0x53,
    0xB2, 0x70, 0x05, 0x2C, 0xFD, 0x59, 0x3A, 0x86,
    0x7E, 0xCE, 0x06, 0xEB, 0x82, 0x78, 0x57, 0xC7,
    0x8D, 0x43, 0xAF, 0xB4, 0x1C, 0xD4, 0x5B, 0xCD,
    0xE2, 0xE9, 0x27, 0x4F, 0xC3, 0x08, 0x72, 0x80,
    0xCF, 0xB0, 0xEF, 0xF5, 0x28, 0x6D, 0xBE, 0x30,
    0x4D, 0x34, 0x92, 0xD5, 0x0E, 0x3C, 0x22, 0x32,
    0xE5, 0xE4, 0xF9, 0x9F, 0xC2, 0xD1, 0x0A, 0x81,
    0x12, 0xE1, 0xEE, 0x91, 0x83, 0x76, 0xE3, 0x97,
    0xE6, 0x61, 0x8A, 0x17, 0x79, 0xA4, 0xB7, 0xDC,
    0x90, 0x7A, 0x5C, 0x8C, 0x02, 0xA6, 0xCA, 0x69,
    0xDE, 0x50, 0x1A, 0x11, 0x93, 0xB9, 0x52, 0x87,
    0x58, 0xFC, 0xED, 0x1D, 0x37, 0x49, 0x1B, 0x6A,
    0xE0, 0x29, 0x33, 0x99, 0xBD, 0x6C, 0xD9, 0x94,
    0xF3, 0x40, 0x54, 0x6F, 0xF0, 0xC6, 0x73, 0xB8,
    0xD6, 0x3E, 0x65, 0x18, 0x44, 0x1F, 0xDD, 0x67,
    0x10, 0xF1, 0x0C, 0x19, 0xEC, 0xAE, 0x03, 0xA1,
    0x14, 0x7B, 0xA9, 0x0B, 0xFF, 0xF8, 0xA3, 0xC0,
    0xA2, 0x01, 0xF7, 0x2E, 0xBC, 0x24, 0x68, 0x75,
    0x0D, 0xFE, 0xBA, 0x2F, 0xB5, 0xD0, 0xDA, 0x3D
};


extern "C"
void scan_outlook(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP) {
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "outlook";
	sp.info->author = "Simson L. Garfinkel";
	sp.info->description = "Outlook Compressible Encryption";
	sp.info->flags = scanner_info::SCANNER_DISABLED | scanner_info::SCANNER_RECURSE
            | scanner_info::SCANNER_DEPTH_0;
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN) {
	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;

        // dodge infinite recursion by refusing to operate on an OFE'd buffer
        if(rcb.partName == pos0.lastAddedPart()) {
            return;
        }

        // managed_malloc throws an exception if allocation fails.
        managed_malloc<uint8_t>dbuf(sbuf.bufsize);
        for(size_t ii = 0; ii < sbuf.bufsize; ii++) {
            uint8_t ch = sbuf.buf[ii];
            dbuf.buf[ii] = libpff_encryption_compressible[ ch ];
        }
        
        const pos0_t pos0_xor = pos0 + "OUTLOOK";
        const sbuf_t child_sbuf(pos0_xor, dbuf.buf, sbuf.bufsize, sbuf.pagesize, false);
        scanner_params child_params(sp, child_sbuf);
        (*rcb.callback)(child_params);    // recurse on deobfuscated buffer
    }
}
