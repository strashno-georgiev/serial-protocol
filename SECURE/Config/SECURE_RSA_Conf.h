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

File        : SECURE_RSA_Conf.h
Purpose     : Configurable emSecure-RSA preprocessor configuration

*/

#ifndef SECURE_RSA_CONF_H
#define SECURE_RSA_CONF_H

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       SECURE_RSA_MAX_KEY_LENGTH
*
*  Description
*    Maximum key length. Default: 2048
*
*    Define the maximum used key length to optimize ROM and Stack usage.
*    Longer keys require more memory.
*/
#define SECURE_RSA_MAX_KEY_LENGTH                     (2048)

#endif

/*************************** End of file ****************************/
