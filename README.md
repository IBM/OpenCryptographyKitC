# OpenCryptographyKitC

The Open Cryptography Kit for C to which this document refers is based on the C language and is usable by products that are based on C or C++.

## Build instructions

For a debug build on any platform specify CONFIG=debug as an argument to make.

### Linux 64-bit

```bash
make -k -C icc OPSYS=AMD64_LINUX create_all
make -k -C icc OPSYS=AMD64_LINUX all
```

### Linux 32-bit

```bash
make -k -C icc OPSYS=LINUX create_all
make -k -C icc OPSYS=LINUX all
```

### Windows 64-bit

```bash
make -k -C icc OPSYS=WIN64_VS2022 create_all
make -k -C icc OPSYS=WIN64_VS2022 all
```

This build is not constrained to MS VS 2022 but is tested on that platform.  
#### Requirements
Visual studio build tools are required, including mfc.  
The supported make is windows gnumake 3.81. Also required is NASM, common unix utilities for windows (ls, sh, mv, rm, patch, etc), strawberry perl, and openjdk.  
  
Tested unix utilities are provided by msys2, or those packaged with the windows git installer.  
The tested openjdk is the Microsoft Build of OpenJDK 17.  
NASM should be added to the windows PATH environment variable.  
Run the appropriate vcvars.bat file in a cmd.exe or otherwise use a development console before running make.  
  
Note: Windows utilities which use a Unix style PATH such as the gnumake available in msys2 are not supported.  
Note: Cygwin is not supported.  