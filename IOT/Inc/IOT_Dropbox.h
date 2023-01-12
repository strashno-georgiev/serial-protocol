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
-------------------------- END-OF-HEADER -----------------------------

File        : IOT_Dropbox.h
Purpose     : API functions for the Dropbox REST API.

*/

#ifndef IOT_DROPBOX_H
#define IOT_DROPBOX_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "IOT_Int.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       Dropbox client version number
*/
#define IOT_DROPBOX_VERSION           23200       // Format: Mmmrr. Example: 12305 is 1.23e

/*********************************************************************
*
*       Dropbox request flags
*
*  Description
*    Flags that a Dropbox request can honor.
*
*  See also
*    IOT_DROPBOX_SetFlag()
*/
#define IOT_DROPBOX_FLAG_MUTE         0           // Unused, deprecated by Dropbox REST API v2.
#define IOT_DROPBOX_FLAG_OVERWRITE    1           // Overwrite file on upload if it exists.
#define IOT_DROPBOX_FLAG_AUTORENAME   2           // Auto-rename file on conflict.

/*********************************************************************
*
*       Dropbox client errors
*
*  Description
*    Errors that the Dropbox Client can generate.
*
*  Additional information
*    Note that lower-level errors originating from the underlying
*    socket or HTTP Client (which the Dropbox Client uses as a
*    service) are also possible as errors returned by the API.
*
*    The Dropbox REST API responses are converted to Dropbox
*    Client errors and the table indicates the Dropbox response
*    that corresponds to the Dropbox Client error.
*/
#define IOT_DROPBOX_STATUS_BAD_INPUT_PARAMETER           -900  // Dropbox response: 400
#define IOT_DROPBOX_STATUS_BAD_OR_EXPIRED_TOKEN          -901  // Dropbox response: 401
#define IOT_DROPBOX_STATUS_BAD_OAUTH_REQUEST             -902  // Dropbox response: 403
#define IOT_DROPBOX_STATUS_FILE_OR_FOLDER_NOT_FOUND      -903  // Dropbox response: 404
#define IOT_DROPBOX_STATUS_REQUEST_METHOD_NOT_EXPECTED   -904  // Dropbox response: 405
#define IOT_DROPBOX_STATUS_NOT_ACCEPTABLE                -905  // Dropbox response: 406, 409, 429
#define IOT_DROPBOX_STATUS_RATE_LIMITED                  -906  // Dropbox response: 503
#define IOT_DROPBOX_STATUS_STORAGE_QUOTA_EXCEEDED        -907  // Dropbox response: 507
#define IOT_DROPBOX_STATUS_SERVER_ERROR                  -908  // Dropbox response: 5xx not otherwise covered
#define IOT_DROPBOX_STATUS_BAD_IMAGE                     -909  // Dropbox response: 415
#define IOT_DROPBOX_STATUS_CHUNKING_UNSUPPORTED          -910  // Dropbox response: 411
#define IOT_DROPBOX_STATUS_OTHER_ERROR                   -911  // Dropbox response: any response not otherwise covered

/*********************************************************************
*
*       Types required for API
*
**********************************************************************
*/

  typedef struct {
  IOT_HTTP_CONTEXT   HTTP;
  IOT_JSON_CONTEXT   JSON;
  const char       * sToken;
  const char       * sHost;
  void             * UserContext;
  unsigned           Flags;
  unsigned           Code;
  U8                 aBuf[128];  // To construct and parse headers
  IOT_HTTP_PARA      aPara[20];
} IOT_DROPBOX_CONTEXT;

/*********************************************************************
*
*       IOT_DROPBOX_METADATA
*
*  Description
*    Metadata extracted from a metadata query.
*
*  See also
*    IOT_DROPBOX_GetMetadata()
*/
typedef struct {
  char aTag[16];       // Tag, field=".tag"
  char aPath[128];     // Canonical file path, field="name"
  U64  Size;           // File size in bytes, field="size"
  U8   IsFolder;       // Nonzero for folders, derived, ".tag == folder"
  U8   IsDeleted;      // Nonzero for deleted files, field="is_deleted"
  U8   aHash[16];      // File digest, binary, field="hash"
  U64  Version;        // File version, field="ver"
  char aModified[32];  // Timestamp of last modification, field="client_modified" [note: Dropbox provides this only for files]
  int  Valid;          // Nonzero when members contain valid data
} IOT_DROPBOX_METADATA;

/*********************************************************************
*
*       IOT_DROPBOX_METADATA_ENUM_FUNC
*
*  Description
*    Callback invoked during metadata enumeration.
*
*  Parameters
*    pMeta - Pointer to parsed metadata.
*
*  See also
*    IOT_DROPBOX_GetMetadata()
*/
typedef void (IOT_DROPBOX_METADATA_ENUM_FUNC)(const IOT_DROPBOX_METADATA *pMeta);


/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       Dropbox API setup functions
*/
void         IOT_DROPBOX_Init             (IOT_DROPBOX_CONTEXT *pSelf, char *pJSONBuf, unsigned JSONBufLen);
void         IOT_DROPBOX_SetIO            (IOT_DROPBOX_CONTEXT *pSelf, const IOT_IO_API *pAPI, void *pContext);
void         IOT_DROPBOX_Exit             (IOT_DROPBOX_CONTEXT *pSelf);
void         IOT_DROPBOX_SetAPIKey        (IOT_DROPBOX_CONTEXT *pSelf, const char *pAPIKey);
void         IOT_DROPBOX_SetFlag          (IOT_DROPBOX_CONTEXT *pSelf, unsigned Flag);
void         IOT_DROPBOX_ClrFlag          (IOT_DROPBOX_CONTEXT *pSelf, unsigned Flag);

/*********************************************************************
*
*       Files and metadata functions
*/
int          IOT_DROPBOX_GetMetadata      (IOT_DROPBOX_CONTEXT *pSelf, const char *sPath, IOT_DROPBOX_METADATA_ENUM_FUNC *pfEnum);
int          IOT_DROPBOX_Remove           (IOT_DROPBOX_CONTEXT *pSelf, const char *sPath);
int          IOT_DROPBOX_Move             (IOT_DROPBOX_CONTEXT *pSelf, const char *sFromPath, const char *sToPath);
int          IOT_DROPBOX_Copy             (IOT_DROPBOX_CONTEXT *pSelf, const char *sFromPath, const char *sToPath);
int          IOT_DROPBOX_CreateFolder     (IOT_DROPBOX_CONTEXT *pSelf, const char *sPath);

/*********************************************************************
*
*       Upload and download
*/
int          IOT_DROPBOX_PutBegin         (IOT_DROPBOX_CONTEXT *pSelf, const char *sPath, unsigned ContentLen);
int          IOT_DROPBOX_PutContent       (IOT_DROPBOX_CONTEXT *pSelf, const void *pData, unsigned DataLen);
int          IOT_DROPBOX_PutEnd           (IOT_DROPBOX_CONTEXT *pSelf);
int          IOT_DROPBOX_GetBegin         (IOT_DROPBOX_CONTEXT *pSelf, const char *sPath);
int          IOT_DROPBOX_GetContent       (IOT_DROPBOX_CONTEXT *pSelf,       void *pData, unsigned DataLen);
void         IOT_DROPBOX_GetEnd           (IOT_DROPBOX_CONTEXT *pSelf);

/*********************************************************************
*
*       Version and copyright information
*/
const char * IOT_DROPBOX_GetVersionText   (void);
const char * IOT_DROPBOX_GetCopyrightText (void);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
