@ECHO OFF

REM /*********************************************************************
REM *                   (c) SEGGER Microcontroller GmbH                  *
REM *                        The Embedded Experts                        *
REM *                           www.segger.com                           *
REM *********************************************************************/
REM 
REM File    : MakeKeys.bat
REM Purpose : Generate all sample SSH keys.
REM

SSH_KeyGen -rsa -nf -l 1024        -x -k SSH_ServerKeys_RSA_Temp_1024b_  >SSH_ServerKeys_RSA.c
SSH_KeyGen -rsa -nf -l 2048        -x -k SSH_ServerKeys_RSA_Temp_2048b_ >>SSH_ServerKeys_RSA.c
SSH_KeyGen -rsa -nf -l 2048        -x -k SSH_ServerKeys_RSA_Host_2048b_ >>SSH_ServerKeys_RSA.c

SSH_KeyGen -dsa -nf -l 1024 -n 160 -x -k SSH_ServerKeys_DSA_1024b_160b_  >SSH_ServerKeys_DSA.c
SSH_KeyGen -dsa -nf -l 2048 -n 160 -x -k SSH_ServerKeys_DSA_2048b_160b_ >>SSH_ServerKeys_DSA.c
SSH_KeyGen -dsa -nf -l 2048 -n 256 -x -k SSH_ServerKeys_DSA_2048b_256b_ >>SSH_ServerKeys_DSA.c
SSH_KeyGen -dsa -nf -l 3072 -n 256 -x -k SSH_ServerKeys_DSA_3072b_256b_ >>SSH_ServerKeys_DSA.c

SSH_KeyGen -ecdsa -p256            -x -k SSH_ServerKeys_ECDSA_P256_      >SSH_ServerKeys_ECDSA.c
SSH_KeyGen -ecdsa -p384            -x -k SSH_ServerKeys_ECDSA_P384_     >>SSH_ServerKeys_ECDSA.c
SSH_KeyGen -ecdsa -p521            -x -k SSH_ServerKeys_ECDSA_P521_     >>SSH_ServerKeys_ECDSA.c

SSH_KeyGen -ed25519                -x -k SSH_ServerKeys_Ed25519_         >SSH_ServerKeys_EdDSA.c
SSH_KeyGen -ed448                  -x -k SSH_ServerKeys_Ed448_          >>SSH_ServerKeys_EdDSA.c

ECHO Keys generated OK!

REM /*************************** End of file ****************************/
