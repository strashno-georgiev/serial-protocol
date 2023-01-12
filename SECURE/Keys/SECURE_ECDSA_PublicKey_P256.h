static const CRYPTO_MPI_LIMB _SECURE_ECDSA_PublicKey_P256_YX_aLimbs[] = {
  CRYPTO_MPI_LIMB_DATA4(0x28, 0x59, 0x5E, 0x86),
  CRYPTO_MPI_LIMB_DATA4(0xAD, 0xED, 0x67, 0xC9),
  CRYPTO_MPI_LIMB_DATA4(0x82, 0x0A, 0x6B, 0x47),
  CRYPTO_MPI_LIMB_DATA4(0xA9, 0x44, 0x41, 0xC1),
  CRYPTO_MPI_LIMB_DATA4(0xD6, 0x46, 0xEE, 0x03),
  CRYPTO_MPI_LIMB_DATA4(0xB4, 0x69, 0x77, 0xDD),
  CRYPTO_MPI_LIMB_DATA4(0xF4, 0x82, 0x22, 0xB5),
  CRYPTO_MPI_LIMB_DATA4(0xA7, 0xE3, 0x75, 0xB9)
};

static const CRYPTO_MPI_LIMB _SECURE_ECDSA_PublicKey_P256_YY_aLimbs[] = {
  CRYPTO_MPI_LIMB_DATA4(0x28, 0xC5, 0xF6, 0xF9),
  CRYPTO_MPI_LIMB_DATA4(0x1D, 0x5F, 0xAB, 0x1C),
  CRYPTO_MPI_LIMB_DATA4(0x3C, 0x93, 0x5E, 0x34),
  CRYPTO_MPI_LIMB_DATA4(0x29, 0xB1, 0xB5, 0x94),
  CRYPTO_MPI_LIMB_DATA4(0xE1, 0xB7, 0x45, 0x2F),
  CRYPTO_MPI_LIMB_DATA4(0x2B, 0x4E, 0xD9, 0xAD),
  CRYPTO_MPI_LIMB_DATA4(0x38, 0xA1, 0xAE, 0x27),
  CRYPTO_MPI_LIMB_DATA4(0x57, 0x97, 0x7E, 0x5C)
};

static const CRYPTO_ECDSA_PUBLIC_KEY _SECURE_ECDSA_PublicKey_P256 = { {
  { CRYPTO_MPI_INIT_RO(_SECURE_ECDSA_PublicKey_P256_YX_aLimbs) },
  { CRYPTO_MPI_INIT_RO(_SECURE_ECDSA_PublicKey_P256_YY_aLimbs) },
  { CRYPTO_MPI_INIT_RO_ZERO },
  { CRYPTO_MPI_INIT_RO_ZERO },
  },
  &CRYPTO_EC_CURVE_secp256r1
};

