/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2022  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File        : FS_EFS.h
Purpose     : EFS File System Layer header
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef FS_EFS_H            // Avoid recursive and multiple inclusion
#define FS_EFS_H

#include "FS_Int.h"

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       EFS_Read
*/
U32  FS_EFS_Read                (      FS_FILE    * pFile, void * pData, U32 NumBytesReq);

/*********************************************************************
*
*       EFS_Write
*/
U32  FS_EFS_Write               (      FS_FILE    * pFile, const void * pData, U32 NumBytes);
int  FS_EFS_CloseFile           (      FS_FILE    * pFile);
void FS_EFS_Clean               (      FS_VOLUME  * pVolume);

/*********************************************************************
*
*       EFS_Open
*/
int  FS_EFS_OpenFile            (const char       * sFileName, FS_FILE * pFile, int DoDel, int DoOpen, int DoCreate);

/*********************************************************************
*
*       EFS_Misc
*/
int  FS_EFS_CheckInfoSector     (      FS_VOLUME  * pVolume);
int  FS_EFS_CreateJournalFile   (      FS_VOLUME  * pVolume, U32 NumBytes, U32 * pFirstSector, U32 * pNumSectors);
int  FS_EFS_OpenJournalFile     (      FS_VOLUME  * pVolume);
U32  FS_EFS_GetIndexOfLastSector(      FS_VOLUME  * pVolume);
int  FS_EFS_FreeSectors         (      FS_VOLUME  * pVolume);
int  FS_EFS_GetFreeSpace        (      FS_VOLUME  * pVolume, void * pBuffer, int SizeOfBuffer, U32 FirstClusterId, U32 * pNumClustersFree, U32 * pNumClustersChecked);
int  FS_EFS_GetATInfo           (      FS_VOLUME  * pVolume, FS_AT_INFO * pATInfo);
I32  FS_EFS_ReadATEntry         (      FS_VOLUME  * pVolume, U32 ClusterId);
void FS_EFS_Save                (      FS_CONTEXT * pContext);
void FS_EFS_Restore             (const FS_CONTEXT * pContext);

/*********************************************************************
*
*       EFS_Format
*/
int  FS_EFS_Format              (      FS_VOLUME * pVolume, const FS_FORMAT_INFO  * pFormatInfo);
int  FS_EFS_GetDiskInfo         (      FS_VOLUME * pVolume, FS_DISK_INFO    * pDiskInfo, int Flags);
int  FS_EFS_GetVolumeLabel      (      FS_VOLUME * pVolume, char * pVolumeLabel, unsigned VolumeLabelSize);
int  FS_EFS_SetVolumeLabel      (      FS_VOLUME * pVolume, const char * pVolumeLabel);

/*********************************************************************
*
*       EFS_Move
*/
int  FS_EFS_Move                (      FS_VOLUME * pVolume, const char * sNameSrc, const char * sNameDest);

/*********************************************************************
*
*       FS_EFS_DirEntry
*/
int  FS_EFS_SetDirEntryInfo     (      FS_VOLUME  * pVolume, const char * sName, const void * p, int Mask);
int  FS_EFS_GetDirEntryInfo     (      FS_VOLUME  * pVolume, const char * sName,       void * p, int Mask);
int  FS_EFS_SetDirEntryInfoEx   (      FS_VOLUME  * pVolume, const FS_DIRENTRY_POS * pDirEntryPos, const void * p, int Mask);
int  FS_EFS_GetDirEntryInfoEx   (      FS_VOLUME  * pVolume, const FS_DIRENTRY_POS * pDirEntryPos,       void * p, int Mask);

/*********************************************************************
*
*       EFS_Rename
*/
int  FS_EFS_Rename              (      FS_VOLUME  * pVolume, const char * sOldName, const char * sNewName);

/*********************************************************************
*
*       EFS_Dir
*/
int  FS_EFS_OpenDir             (const char       * sDirName, FS_DIR_OBJ * pDirObj);
int  FS_EFS_CloseDir            (      FS_DIR_OBJ * pDirObj);
int  FS_EFS_ReadDir             (      FS_DIR_OBJ * pDirObj, FS_DIRENTRY_INFO * pDirEntryInfo);
int  FS_EFS_RemoveDir           (      FS_VOLUME  * pVolume, const char * sDirName);
int  FS_EFS_CreateDir           (      FS_VOLUME  * pVolume, const char * sDirName);
int  FS_EFS_DeleteDir           (      FS_VOLUME  * pVolume, const char * sDirName, int MaxRecursionLevel);

/*********************************************************************
*
*       EFS_SetEndOfFile
*/
int  FS_EFS_SetEndOfFile        (      FS_FILE    * pFile);
int  FS_EFS_SetFileSize         (      FS_FILE    * pFile, FS_FILE_SIZE NumBytes);

/*********************************************************************
*
*       FS_EFS_CheckDisk
*/
int  FS_EFS_CheckVolume         (      FS_VOLUME  * pVolume, void * pBuffer, U32 BufferSize, int MaxRecursionLevel, FS_CHECKDISK_ON_ERROR_CALLBACK * pfOnError);
int  FS_EFS_CheckDir            (      FS_VOLUME  * pVolume, const char * sPath, FS_CLUSTER_MAP * pClusterMap, FS_CHECKDISK_ON_ERROR_CALLBACK * pfOnError);
int  FS_EFS_CheckAT             (      FS_VOLUME  * pVolume, const FS_CLUSTER_MAP * pClusterMap, FS_CHECKDISK_ON_ERROR_CALLBACK * pfOnError);

#if defined(__cplusplus)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // EFS_H

/*************************** End of file ****************************/
