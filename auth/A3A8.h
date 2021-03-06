/*
 * random_number[0..15]: the challenge from the base station
 * key[0..15]: the SIM's A3/A8 long-term key Ki
 * simoutput[0..11]: what you'd get back if you fed random_number and key to a real
 * SIM.
 *
 *   The GSM spec states that simoutput[0..3] is SRES,
 *   and simoutput[4..11] is Kc (the A5 session key).
 *   (See GSM 11.11, Section 8.16.  See also the leaked document
 *   referenced below.)
 *   Note that Kc is bits 74..127 of the COMP128 output, followed by 10
 *   zeros.
 *   In other words, A5 is keyed with only 54 bits of entropy. This
 *   represents a deliberate weakening of the key used for voice privacy
 *   by a factor of over 1000.
 * 
 * Verified with a Pacific Bell Schlumberger SIM.  Your mileage may vary.
 *
 * Marc Briceno <marc@scard.org>, Ian Goldberg <iang@cs.berkeley.edu>,
 * and David Wagner <daw@cs.berkeley.edu>
 */
#ifndef _A3A8_INCLUDED_H
#include "..\defs.h"
#include "..\midgard\midgard.h"
#define FID_AUTHKEY			0x6F38

#define A3A8_OUTPUT_SIZE	0x0C
#define A3A8_INPUT_SIZE		0x10
#define A3A8_KEY_SIZE		0x10
//#define A3A8_DEBUG_KEY		"\x82\xf8\xa3\x96\xe3\xc4\x67\x81\x28\x8f\x3a\x69\x3e\x4c\x76\x18\x00"		

uchar auth_A3A8(/* in */ uchar rand[16], /* in */ uchar key[16], /* out */ uchar simoutput[12]) _REENTRANT_;
uchar Authenticate_GSM(uchar * rand, uchar * output) _REENTRANT_ ;

#define _A3A8_INCLUDED_H
#endif
 