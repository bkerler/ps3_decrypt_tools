/*
 * Copyright (c) 2011-2012 by ps3dev.net
 * This file is released under the GPLv2.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "aes.h"
#include "util.h"
#include "keys.h"
#include "eid.h"
#include "kgen.h"
#include "types.h"
#include "indiv.h"
#include "aes_xts.h"

void generate_hdd_individuals() {
    u8 ata_k1[0x20], ata_k2[0x20], edec_k1[0x20], edec_k2[0x20];

    //fetching root_key
    eid_root_key = _read_buffer((s8*) "data/eid_root_key", NULL);

    //Generate keys.
    generate_ata_keys(eid_root_key, eid_root_key + 0x20, ata_k1, ata_k2);
    generate_encdec_keys(eid_root_key, eid_root_key + 0x20, edec_k1, edec_k2);

    _hexdump(stdout, "ATA-DATA-KEY    ", 0, ata_k1, 0x20, 0);
    _hexdump(stdout, "ATA-TWEAK-KEY   ", 0, ata_k2, 0x20, 0);
    _hexdump(stdout, "ENCDEC-DATA-KEY ", 0, edec_k1, 0x20, 0);
    _hexdump(stdout, "ENCDEC-TWEAK-KEY", 0, edec_k2, 0x20, 0);
}

void decrypt_eid() {
    //fetching root_key
    eid_root_key = _read_buffer((s8*) "data/eid_root_key", NULL);

    //unpacking eid
    eid_unpack((s8*) "eid/eid");

    //decrypting
    eid0_decrypt((s8*) "eid/eid0", (s8*) "eid/eid0decrypted");
    eid1_decrypt((s8*) "eid/eid1", (s8*) "eid/eid1decrypted.bin");
    eid2_generate_block((s8*) "eid/eid2", EID2_BLOCKTYPE_P, (s8*) "eid/eid2pblock.bin");
    eid2_generate_block((s8*) "eid/eid2", EID2_BLOCKTYPE_S, (s8*) "eid/eid2sblock.bin");
    u8* pblock = _read_buffer("eid/eid2pblock.bin", NULL);
    eid2_decrypt_block(pblock, 0x80);
    memcpy(pblock, pblock + 0x10, 0x60);
    _write_buffer((s8*) "eid/pblockdec.bin", pblock, 0x60);
    u8* sblock = _read_buffer("eid/eid2sblock.bin", NULL);
    eid2_decrypt_block(sblock, 0x690);
    memcpy(sblock, sblock + 0x10, 0x670);
    _write_buffer((s8*) "eid/sblockdec.bin", sblock, 0x670);
    eid3_decrypt((s8*) "eid/eid3", (s8*) "eid/eid3decrypted.bin");
    eid4_decrypt((s8*) "eid/eid4", (s8*) "eid/eid4decrypted.bin");
}

void encrypt_eid0_section_A() {
    //fetching root_key
    eid_root_key = _read_buffer((s8*) "data/eid_root_key", NULL);

    //encrypting
    eid0_encrypt_section_A((s8*) "eid/eid0decrypted", (s8*) "eid/eid0encrypted");
}

void syscon_auth() {
    aes_context aes_ctxt;
    u8 indiv[0x40];
    u8 indiv_key[0x20];
    u8 zero_iv[0x10] = {0};
    u8 enc_key_seed[INDIV_SIZE];

    //fetching root_key
    eid_root_key = _read_buffer((s8*) "data/eid_root_key", NULL);

    //Generate individuals.
    indiv_gen(eid1_indiv_seed, NULL, NULL, NULL, indiv);
    _write_buffer((s8*) "syscon/indiv", indiv, 0x40);

    //Generate seeds
    memcpy(indiv_key, indiv + 0x20, 0x20);
    aes_setkey_enc(&aes_ctxt, indiv_key, KEY_BITS(0x20));
    aes_crypt_cbc(&aes_ctxt, AES_ENCRYPT, INDIV_SIZE, zero_iv, syscon_key_seed, enc_key_seed);
    _write_buffer((s8*) "syscon/enc_key_seed", enc_key_seed, INDIV_SIZE);
}

void gen_vtrm(){
	//fetching root_key
    eid_root_key = _read_buffer((s8*) "data/eid_root_key", NULL);
	
	aes_context aes_ctxt;
    u8 iv[0x10];
	u8 indiv[0x40];
	u8 key[0x10];
	u8 block_key[0x10];
	u8 block_iv[0x10]={0x0};
	
	
	indiv_gen(eid1_indiv_seed, NULL, NULL, NULL, indiv);
	
    //Generate VTRM Block Key.
	memcpy(key, indiv + 0x20, 0x10);
    aes_setkey_enc(&aes_ctxt, key, KEY_BITS(0x10));
    memcpy(iv, indiv + 0x10, 0x10);
    aes_crypt_cbc(&aes_ctxt, AES_ENCRYPT, 0x10, iv, keyseed_for_srk2, block_key);
	
	_hexdump(stdout, "Block Key:    ", 0, block_key, 0x10, 0);
	_hexdump(stdout, "Block Iv:    ", 0, block_iv, 0x10, 0);
	
}

void gen_backup(){
	//fetching root_key
    eid_root_key = _read_buffer((s8*) "data/eid_root_key", NULL);
	
	aes_context aes_ctxt;
    u8 iv[0x10];
	u8 indiv[0x40];
	u8 key[0x10];
	u8 backup_key[0x10];
	
	
	indiv_gen(eid1_indiv_seed, NULL, NULL, NULL, indiv);
	
    //Generate Backup Key.
	memcpy(key, indiv + 0x20, 0x10);
    aes_setkey_enc(&aes_ctxt, key, KEY_BITS(0x10));
    memcpy(iv, indiv + 0x10, 0x10);
    aes_crypt_cbc(&aes_ctxt, AES_ENCRYPT, 0x10, iv, seed_for_backup, backup_key);
	memcpy(iv, indiv + 0x10, 0x10);
	
	_hexdump(stdout, "Backup Key:    ", 0, backup_key, 0x10, 0);
	_hexdump(stdout, "Backup Iv:    ", 0, iv, 0x10, 0);
	
}

int main() {
    int i;
    printf("Select an option\n1-Decrypt eEID(missing eid5)\n2-Encrypt EID0 Section A\n3-Generate Syscon AUTH seeds(Acording to wiki)\n4-Generate HDD Keys\n5-Generate VTRM Keys\n6-Generate Backup Keys\n0-Exit\n");
    scanf("%d", &i);
    switch (i) {
        case 1:
            decrypt_eid();
			getchar();
            break;
        case 2:
            encrypt_eid0_section_A();
			getchar();
            break;
        case 3:
            syscon_auth();
			getchar();
            break;
        case 4:
            generate_hdd_individuals();
			getchar();
            break;
		case 5:
            gen_vtrm();
			getchar();
            break;
		case 6:
            gen_backup();
			getchar();
        case 0:
            break;
        default:
            printf("Incorrect Option Selected! Try Again.");
            break;
    }
    return 0;
}
