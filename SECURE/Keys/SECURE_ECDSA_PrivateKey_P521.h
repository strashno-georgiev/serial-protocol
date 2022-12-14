static const CRYPTO_MPI_LIMB _SECURE_ECDSA_PrivateKey_P521_X_aLimbs[] = {
  CRYPTO_MPI_LIMB_DATA4(0xD4, 0xE1, 0x08, 0xCD),
  CRYPTO_MPI_LIMB_DATA4(0x1D, 0x4C, 0xF1, 0xCC),
  CRYPTO_MPI_LIMB_DATA4(0x5B, 0xFE, 0x5A, 0xF9),
  CRYPTO_MPI_LIMB_DATA4(0x3D, 0xFA, 0xD1, 0x01),
  CRYPTO_MPI_LIMB_DATA4(0x3C, 0xDE, 0xF6, 0x35),
  CRYPTO_MPI_LIMB_DATA4(0x3C, 0x20, 0x5B, 0xF8),
  CRYPTO_MPI_LIMB_DATA4(0x22, 0xA4, 0x96, 0xF3),
  CRYPTO_MPI_LIMB_DATA4(0xA3, 0xD7, 0x24, 0xF3),
  CRYPTO_MPI_LIMB_DATA4(0x85, 0x6A, 0x9B, 0xEE),
  CRYPTO_MPI_LIMB_DATA4(0x78, 0xE7, 0xEB, 0xD5),
  CRYPTO_MPI_LIMB_DATA4(0xBF, 0xB2, 0xED, 0xCE),
  CRYPTO_MPI_LIMB_DATA4(0x68, 0x07, 0x2E, 0xCC),
  CRYPTO_MPI_LIMB_DATA4(0xA5, 0x61, 0x91, 0xE2),
  CRYPTO_MPI_LIMB_DATA4(0x67, 0xED, 0xDF, 0x8F),
  CRYPTO_MPI_LIMB_DATA4(0x2A, 0xB0, 0x99, 0x58),
  CRYPTO_MPI_LIMB_DATA4(0xB0, 0x15, 0xDB, 0x43),
  CRYPTO_MPI_LIMB_DATA2(0x32, 0x01)
};

static const CRYPTO_ECDSA_PRIVATE_KEY _SECURE_ECDSA_PrivateKey_P521 = {
  { CRYPTO_MPI_INIT_RO(_SECURE_ECDSA_PrivateKey_P521_X_aLimbs) },
  &CRYPTO_EC_CURVE_secp521r1
};

