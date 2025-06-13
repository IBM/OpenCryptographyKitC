/*
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*/

int icc_failure = 0;

int main(int argc, char *argv[])
{
   ICC_STATUS icc_stat;
   memset(&icc_stat,0,sizeof(icc_stat));

   STUB##_Test(&icc_stat);

}