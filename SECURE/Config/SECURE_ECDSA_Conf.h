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

File        : SECURE_ECDSA_Conf.h
Purpose     : Configurable emSecure-ECDSA preprocessor configuration

*/

#ifndef SECURE_ECDSA_CONF_H
#define SECURE_ECDSA_CONF_H

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       SECURE_ECDSA_MAX_KEY_LENGTH
*
*  Description
*    Maximum key length.
*
*    Define the maximum used key length to optimize ROM and Stack usage.
*    The key length must be no smaller than the prime size of the
*    largest curve that you use.  For instance, if you use P-256 as your
*    curve, set the to 256.  If you use P-521, set this to 521.
*/
#define SECURE_ECDSA_MAX_KEY_LENGTH            521


#endif

/*************************** End of file ****************************/
