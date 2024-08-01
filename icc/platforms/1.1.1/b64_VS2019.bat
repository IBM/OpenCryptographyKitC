REM     NO. DONT DO THIS :
rem Windows 64 environment setup
rem set SPL=C:\Strawberry
rem set the path for all the components needed to build OpenSSL
rem set PATH=%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\Wbem;%SPL%\c\bin;%SPL%\perl\site\bin;%SPL%\perl\bin;%PFX%\nasm\;%VC%;%VSB%\Common7\IDE;%SDKW%\bin
rem setup so Windows programs can find headers/libs

REM  The environment path for MSVC, nmake, perl, nasm etc must all be set up on the build machine.
REM  Please dont add it in here otherwise the build wont work on development machines!
REM  Refer to GSKit windows build machine.

mkdir tmp
set OSSLVER=%1
if x%OSSLVER% == x set goto no_OSSLVER
cd ..\%OSSLVER%
perl Configure VC-WIN64A no-engine no-sctp no-idea no-rc5 no-whirlpool no-zlib enable-mdc2 enable-camellia enable-md2 no-seed 
set MAKE=
set MAKEFLAGS=
nmake /f makefile
mkdir tmp32dll
copy /Y crypto\*.obj tmp32dll\
copy /Y ms\*.obj tmp32dll\
copy /Y apps\*.obj tmp32dll\
copy /Y ssl\*.obj tmp32dll\
copy /Y crypto\async\arch\*.obj tmp32dll\
copy /Y crypto\ec\curve448\*.obj tmp32dll\
copy /Y crypto\ec\curve448\arch_32\*.obj tmp32dll\
FOR /D %%G in ( crypto/* ) DO copy /Y crypto\%%G\*.obj tmp32dll\
FOR /D %%G in ( ssl/* ) DO copy /Y ssl\%%G\*.obj tmp32dll\

goto done

:no_OSSLVER
echo no OSSLVER set as argument
goto done

:done
rem exit

