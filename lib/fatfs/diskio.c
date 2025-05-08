/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

#include <main.h>
#include <stm32h5xx_hal_sd.h>

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */)
{
    uint32_t state = HAL_SD_GetCardState(sdCardHandle());
    if (state == HAL_SD_CARD_READY)
    {
        return 0;
    }

    return STA_NODISK;
}


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
    sdCardInit();
    return 0;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE  pdrv,   /* Physical drive nmuber to identify the drive */
                  BYTE* buff,   /* Data buffer to store read data */
                  LBA_t sector, /* Start sector in LBA */
                  UINT  count   /* Number of sectors to read */
)
{
    __disable_irq();
    HAL_StatusTypeDef ret = HAL_SD_ReadBlocks(sdCardHandle(), buff, sector, count, 1000);
    __enable_irq();
    if (ret != HAL_OK)
    {
        return ret;
    }

    return HAL_OK;

    // HAL_StatusTypeDef ret = HAL_SD_ReadBlocks(sdCardHandle(), buff, sector, count, 0xFFFFFFFF);
    // return ret == HAL_OK ? RES_OK : RES_ERROR;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE        pdrv,   /* Physical drive nmuber to identify the drive */
                   const BYTE* buff,   /* Data to be written */
                   LBA_t       sector, /* Start sector in LBA */
                   UINT        count   /* Number of sectors to write */
)
{
    // HAL_StatusTypeDef ret = HAL_SD_WriteBlocks(sdCardHandle(), buff, sector, count, 0xFFFFFFFF);
    // return ret == HAL_OK ? RES_OK : RES_ERROR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE  pdrv, /* Physical drive nmuber (0..) */
                   BYTE  cmd,  /* Control code */
                   void* buff  /* Buffer to send/receive control data */
)
{
    HAL_SD_CardInfoTypeDef cardInfo;
    HAL_SD_GetCardInfo(sdCardHandle(), &cardInfo);

    switch (cmd)
    {
        case CTRL_SYNC:
            {
                return RES_OK;
            }

        case GET_SECTOR_COUNT:
            {
                *(LBA_t*)buff = cardInfo.BlockNbr;
                return RES_OK;
            }

        case GET_SECTOR_SIZE:
            {
                *(WORD*)buff = 0x200;
                return RES_OK;
            }

        case GET_BLOCK_SIZE:
            {
                *(DWORD*)buff = 0x200;
                return RES_OK;
            }

        case CTRL_TRIM:
            {
                return RES_OK;
            }

        default:
            {
                return RES_ERROR;
            }
    }

    return RES_PARERR;
}
