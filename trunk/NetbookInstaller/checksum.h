/*
 *  checksum.h
 *  NetbookInstaller
 *
 *  Created by Evan Lojewski on 5/15/09.
 *  Copyright 2009. All rights reserved.
 *
 */


#define NUM_SUPPORTED_BOOTLOADERS		2

const UInt64 bootLoaderMD5[NUM_SUPPORTED_BOOTLOADERS][2] = 
{
	0xd593439fcf1479a4, 0x03054db491c0e928,
	0xa17b6a642a53a661, 0xbed651c83be369ed
};

const char *bootLoaderName[NUM_SUPPORTED_BOOTLOADERS] =
{
	"Chameleon 2.0 RC1 r431",
	"PCEFI v9"
};

