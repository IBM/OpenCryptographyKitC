/*
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*/
/*! 
  @brief Key unwrap function, Public API
  @param in input buffer
  @param inl length of input buffer
  @param out output buffer (length of input +16)
  @param outl place to store the output length
  @param key the AES key
  @param kl Size of the AES key (bits)
  @param flags 
  - 1 Wrap 
  - 2 Forward decrypt
  - 4 Pad
  @return 1 O.K., length of output in *outl  
  - 0 Parameter error 
  - 2 Unwrap mac mismatch
  - 3 range error in input, 
  - 4 Memory error
*/
int SP800_38F_KW(unsigned char *in, int inl, unsigned char *out, int *outl, unsigned char *key, int kl,unsigned int flags);

