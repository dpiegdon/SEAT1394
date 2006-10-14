
#ifndef __ENDIAN_SWAP_H__
# define __ENDIAN_SWAP_H__

inline uint64_t endian_swap64(uint64_t val)
{
	uint64_t sval;

	((uint8_t*)&sval)[0]	= ((uint8_t*)&sval)[7];
	((uint8_t*)&sval)[1]	= ((uint8_t*)&sval)[6];
	((uint8_t*)&sval)[2]	= ((uint8_t*)&sval)[5];
	((uint8_t*)&sval)[3]	= ((uint8_t*)&sval)[4];
	((uint8_t*)&sval)[4]	= ((uint8_t*)&sval)[3];
	((uint8_t*)&sval)[5]	= ((uint8_t*)&sval)[2];
	((uint8_t*)&sval)[6]	= ((uint8_t*)&sval)[1];
	((uint8_t*)&sval)[7]	= ((uint8_t*)&sval)[0];

	return sval;
}

inline uint32_t endian_swap32(uint32_t val)
{
	uint32_t sval;

	((uint8_t*)&sval)[0]	= ((uint8_t*)&sval)[3];
	((uint8_t*)&sval)[1]	= ((uint8_t*)&sval)[2];
	((uint8_t*)&sval)[2]	= ((uint8_t*)&sval)[1];
	((uint8_t*)&sval)[3]	= ((uint8_t*)&sval)[0];

	return sval;
}

inline uint16_t endian_swap16(uint32_t val)
{
	uint16_t sval;

	((uint8_t*)&sval)[0]	= ((uint8_t*)&sval)[3];
	((uint8_t*)&sval)[1]	= ((uint8_t*)&sval)[2];

	return sval;
}

#endif // __ENDIAN_SWAP_H__

