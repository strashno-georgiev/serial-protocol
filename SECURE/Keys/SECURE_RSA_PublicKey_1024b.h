static const CRYPTO_MPI_LIMB _SECURE_RSA_PublicKey_1024b_N_aLimbs[] = {
  CRYPTO_MPI_LIMB_DATA4(0x8F, 0x53, 0xA6, 0x06),
  CRYPTO_MPI_LIMB_DATA4(0x20, 0x05, 0xA9, 0x7A),
  CRYPTO_MPI_LIMB_DATA4(0x5D, 0x5B, 0x1E, 0x9C),
  CRYPTO_MPI_LIMB_DATA4(0xD7, 0xC4, 0xA0, 0x09),
  CRYPTO_MPI_LIMB_DATA4(0x3C, 0x82, 0x97, 0xB8),
  CRYPTO_MPI_LIMB_DATA4(0xCD, 0x8B, 0xAF, 0x2A),
  CRYPTO_MPI_LIMB_DATA4(0xE4, 0x5F, 0x19, 0x8A),
  CRYPTO_MPI_LIMB_DATA4(0x18, 0x23, 0xCF, 0x3E),
  CRYPTO_MPI_LIMB_DATA4(0xFE, 0x13, 0x36, 0x24),
  CRYPTO_MPI_LIMB_DATA4(0xDF, 0xAF, 0x93, 0xD3),
  CRYPTO_MPI_LIMB_DATA4(0x66, 0x65, 0x79, 0x6A),
  CRYPTO_MPI_LIMB_DATA4(0xFE, 0x7E, 0xEF, 0x21),
  CRYPTO_MPI_LIMB_DATA4(0x42, 0x98, 0x3C, 0x96),
  CRYPTO_MPI_LIMB_DATA4(0x5B, 0x3A, 0x4D, 0xDC),
  CRYPTO_MPI_LIMB_DATA4(0x7D, 0x8E, 0xF4, 0xC4),
  CRYPTO_MPI_LIMB_DATA4(0x61, 0xDC, 0x37, 0xDF),
  CRYPTO_MPI_LIMB_DATA4(0x61, 0x2A, 0xB1, 0x80),
  CRYPTO_MPI_LIMB_DATA4(0x70, 0x99, 0x19, 0x56),
  CRYPTO_MPI_LIMB_DATA4(0x33, 0x6B, 0x60, 0xD8),
  CRYPTO_MPI_LIMB_DATA4(0x35, 0x86, 0x53, 0xCF),
  CRYPTO_MPI_LIMB_DATA4(0x01, 0x2D, 0x1D, 0xFC),
  CRYPTO_MPI_LIMB_DATA4(0x1E, 0xAC, 0x9D, 0xE6),
  CRYPTO_MPI_LIMB_DATA4(0x82, 0xF0, 0x29, 0x52),
  CRYPTO_MPI_LIMB_DATA4(0x9F, 0x3A, 0x75, 0x09),
  CRYPTO_MPI_LIMB_DATA4(0x2E, 0x4C, 0xF5, 0x30),
  CRYPTO_MPI_LIMB_DATA4(0xE4, 0xD9, 0x89, 0x9B),
  CRYPTO_MPI_LIMB_DATA4(0xE6, 0xB8, 0x5A, 0x6D),
  CRYPTO_MPI_LIMB_DATA4(0x21, 0x1E, 0x1E, 0x2D),
  CRYPTO_MPI_LIMB_DATA4(0x3B, 0xFD, 0xDB, 0x62),
  CRYPTO_MPI_LIMB_DATA4(0xDE, 0xD2, 0xA3, 0xBE),
  CRYPTO_MPI_LIMB_DATA4(0x91, 0xDB, 0xFB, 0x99),
  CRYPTO_MPI_LIMB_DATA4(0x65, 0x0D, 0xA2, 0xE4)
};

static const CRYPTO_MPI_LIMB _SECURE_RSA_PublicKey_1024b_E_aLimbs[] = {
  CRYPTO_MPI_LIMB_DATA3(0x01, 0x00, 0x01)
};

static const CRYPTO_RSA_PUBLIC_KEY _SECURE_RSA_PublicKey_1024b = {
  { CRYPTO_MPI_INIT_RO(_SECURE_RSA_PublicKey_1024b_N_aLimbs) },
  { CRYPTO_MPI_INIT_RO(_SECURE_RSA_PublicKey_1024b_E_aLimbs) },
};

