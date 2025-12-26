/**************************************************************************
 * This is a test program which tests RAM memory. 
 **************************************************************************/
#include "hal_data.h"

#ifndef printf
#define printf	xprintf		// printf関数は環境に応じて差し替える 
#endif


#if BSP_CFG_DCACHE_ENABLED
#define DCACHE_FLUSH(_addr, _size)	SCB_CleanInvalidateDCache_by_Addr((volatile void *)(_addr), (int32_t)(_size))
#else
#define DCACHE_FLUSH(_addr, _size)
#endif

#define IORD_32DIRECT(_addr, _offset)			*((uint32_t *)((_addr)+(_offset)))
#define IORD_16DIRECT(_addr, _offset)			*((uint16_t *)((_addr)+(_offset)))
#define IORD_8DIRECT(_addr, _offset)			*((uint8_t *)((_addr)+(_offset)))
#define IOWR_32DIRECT(_addr, _offset, _data)	do{ *((uint32_t *)((_addr)+(_offset)))=(uint32_t)(_data); DCACHE_FLUSH((_addr)+(_offset), 4); }while(0)
#define IOWR_16DIRECT(_addr, _offset, _data)	do{ *((uint16_t *)((_addr)+(_offset)))=(uint16_t)(_data); DCACHE_FLUSH((_addr)+(_offset), 2); }while(0)
#define IOWR_8DIRECT(_addr, _offset, _data)		do{ *((uint8_t *)((_addr)+(_offset)))=(uint8_t)(_data); DCACHE_FLUSH((_addr)+(_offset), 1); }while(0)


/******************************************************************
*  Function: MemTestDataBus
*
*  Purpose: Tests that the data bus is connected with no 
*           stuck-at's, shorts, or open circuits.
*
******************************************************************/
static uint32_t MemTestDataBus(uint32_t address)
{
  uint32_t pattern;
  uint32_t ret_code = 0x0;

  /* Perform a walking 1's test at the given address. */
  for (pattern = 1; pattern != 0; pattern <<= 1)
  {
    /* Write the test pattern. */
    IOWR_32DIRECT(address, 0, pattern);

    /* Read it back (immediately is okay for this test). */
    if (IORD_32DIRECT(address, 0) != pattern)
    {
      ret_code = pattern;
      break;
    }
  }
  return ret_code;
}


/******************************************************************
*  Function: MemTestAddressBus
*
*  Purpose: Tests that the address bus is connected with no 
*           stuck-at's, shorts, or open circuits.
*
******************************************************************/
static uint32_t MemTestAddressBus(uint32_t memory_base, uint32_t nBytes)
{
  uint32_t address_mask = (nBytes - 1);
  uint32_t offset;
  uint32_t test_offset;

  uint32_t pattern     = 0xAAAAAAAA;
  uint32_t antipattern  = 0x55555555;

  uint32_t ret_code = 0x0;

  /* Write the default pattern at each of the power-of-two offsets. */
  for (offset = sizeof(uint32_t); (offset & address_mask) != 0; offset <<= 1)
  {
    IOWR_32DIRECT(memory_base, offset, pattern);
  }

  /* Check for address bits stuck high. */
  test_offset = 0;
  IOWR_32DIRECT(memory_base, test_offset, antipattern);
  for (offset = sizeof(uint32_t); (offset & address_mask) != 0; offset <<= 1)
  {
     if (IORD_32DIRECT(memory_base, offset) != pattern)
     {
        ret_code = (memory_base+offset);
        break;
     }
  }

  /* Check for address bits stuck low or shorted. */
  IOWR_32DIRECT(memory_base, test_offset, pattern);
  for (test_offset = sizeof(uint32_t); (test_offset & address_mask) != 0; test_offset <<= 1)
  {
    if (!ret_code)
    {
      IOWR_32DIRECT(memory_base, test_offset, antipattern);
      for (offset = sizeof(uint32_t); (offset & address_mask) != 0; offset <<= 1)
      {
        if ((IORD_32DIRECT(memory_base, offset) != pattern) && (offset != test_offset))
        {
          ret_code = (memory_base + test_offset);
          break;
        }
      }
      IOWR_32DIRECT(memory_base, test_offset, pattern);
    }
  }

  return ret_code;
}


/******************************************************************
*  Function: MemTest8_16BitAccess
*
*  Purpose: Tests that the memory at the specified base address
*           can be read and written in both byte and half-word 
*           modes.
*
******************************************************************/
static uint32_t MemTest8_16BitAccess(uint32_t memory_base)
{
  uint32_t ret_code = 0x0;

  /* Write 4 bytes */
  IOWR_8DIRECT(memory_base, 0, 0x0A);
  IOWR_8DIRECT(memory_base, 1, 0x05);
  IOWR_8DIRECT(memory_base, 2, 0xA0);
  IOWR_8DIRECT(memory_base, 3, 0x50);

  /* Read it back as one word */
  if(IORD_32DIRECT(memory_base, 0) != 0x50A0050A)
  {
    ret_code = memory_base;
  }

  /* Read it back as two half-words */
  if (!ret_code)
  {
    if ((IORD_16DIRECT(memory_base, 2) != 0x50A0) ||
        (IORD_16DIRECT(memory_base, 0) != 0x050A))
    {
      ret_code = memory_base;
    }
  }

  /* Read it back as 4 bytes */
  if (!ret_code)
  {
    if ((IORD_8DIRECT(memory_base, 3) != 0x50) ||
        (IORD_8DIRECT(memory_base, 2) != 0xA0) ||
        (IORD_8DIRECT(memory_base, 1) != 0x05) ||
        (IORD_8DIRECT(memory_base, 0) != 0x0A))
    {
    ret_code = memory_base;
    }
  }

  /* Write 2 half-words */
  if (!ret_code)
  {
    IOWR_16DIRECT(memory_base, 0, 0x50A0);
    IOWR_16DIRECT(memory_base, 2, 0x050A);

    /* Read it back as one word */
    if(IORD_32DIRECT(memory_base, 0) != 0x050A50A0)
    {
      ret_code = memory_base;
    }
  }

  /* Read it back as two half-words */
  if (!ret_code)
  {
    if ((IORD_16DIRECT(memory_base, 2) != 0x050A) ||
        (IORD_16DIRECT(memory_base, 0) != 0x50A0))
    {
      ret_code = memory_base;
    }
  }

  /* Read it back as 4 bytes */
  if (!ret_code)
  {
    if ((IORD_8DIRECT(memory_base, 3) != 0x05) ||
        (IORD_8DIRECT(memory_base, 2) != 0x0A) ||
        (IORD_8DIRECT(memory_base, 1) != 0x50) ||
        (IORD_8DIRECT(memory_base, 0) != 0xA0))
    {
      ret_code = memory_base;
    }
  }

  return(ret_code);
}


/******************************************************************
*  Function: MemTestDevice
*
*  Purpose: Tests that every bit in the memory device within the 
*           specified address range can store both a '1' and a '0'.
*
******************************************************************/
static uint32_t MemTestDevice(uint32_t memory_base, uint32_t nBytes)
{
  uint32_t offset;
  uint32_t pattern;
  uint32_t antipattern;
  uint32_t ret_code = 0x0;

  /* Fill memory with a known pattern. */
  for (pattern = 1, offset = 0; offset < nBytes; pattern++, offset+=4)
  {
    IOWR_32DIRECT(memory_base, offset, pattern);
  }

  printf(" .");

  /* Check each location and invert it for the second pass. */
  for (pattern = 1, offset = 0; offset < nBytes; pattern++, offset+=4)
  {
    if (IORD_32DIRECT(memory_base, offset) != pattern)
    {
      ret_code = (memory_base + offset);
      break;
    }
    antipattern = ~pattern;
    IOWR_32DIRECT(memory_base, offset, antipattern);
  }

  printf(" .");

  /* Check each location for the inverted pattern and zero it. */
  for (pattern = 1, offset = 0; offset < nBytes; pattern++, offset+=4)
  {
    antipattern = ~pattern;
    if (IORD_32DIRECT(memory_base, offset) != antipattern)
    {
      ret_code = (memory_base + offset);
      break;
    }
    IOWR_32DIRECT(memory_base, offset, 0x0);
  }
  return ret_code;
}

/******************************************************************
*  Function: memtest(uint32_t memory_base, utint32_t memory_end)
*
*  Purpose: Performs a full-test on the RAM specified.  The tests
*           run are:
*             - MemTestDataBus
*             - MemTestAddressBus
*             - MemTest8_16BitAccess
*             - MemTestDevice
*
******************************************************************/
int memtest(uint32_t memory_base, uint32_t memory_end)
{
	uint32_t memory_size = (memory_end - memory_base) + 1;
	uint32_t ret_code = 0x0;

	printf("Testing RAM from 0x%x to 0x%x\n", memory_base, memory_end);

	/* Test Data Bus. */
	ret_code = MemTestDataBus(memory_base);

	if (ret_code) {
		printf("  - Data bus test failed at bit 0x%x\n", ret_code);
	} else {
		printf("  - Data bus test passed\n");
	}

	/* Test Address Bus. */
	if (!ret_code) {
		ret_code  = MemTestAddressBus(memory_base, memory_size);
    	if (ret_code) {
			printf("  - Address bus test failed at address 0x%x\n", ret_code);
		} else {
			printf("  - Address bus test passed\n");
		}
	}

	/* Test byte and half-word access. */
	if (!ret_code) {
		ret_code = MemTest8_16BitAccess(memory_base);
		if (ret_code) {
			printf("  - Byte and half-word access test failed at address 0x%x\n", ret_code);
		} else {
			printf("  - Byte and half-word access test passed\n");
		}
	}

	/* Test that each bit in the device can store both 1 and 0. */
	if (!ret_code) {
		printf("  - Testing each bit in memory device.");
		ret_code = MemTestDevice(memory_base, memory_size);
		if (ret_code) {
			printf(" failed at address 0x%x\n", ret_code);
		} else {
			printf(" passed\n");
		}
	}

	return (!ret_code)? 0 : 1;
}



