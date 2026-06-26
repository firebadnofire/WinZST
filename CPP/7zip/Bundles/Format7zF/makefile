PROG = 7z.dll
# USE_C_LZFINDOPT = 1
DEF_FILE = ../../Archive/Archive2.def
CFLAGS = $(CFLAGS) \
  -DZ7_EXTERNAL_CODECS \

!IFNDEF UNDER_CE
!ENDIF

!include "Arc.mak"

COMMON_OBJS = $(COMMON_OBJS) \
  $O\FileStreams.obj \

COMPRESS_OBJS = $(COMPRESS_OBJS) \
  $O\CodecExports.obj \

AR_OBJS = $(AR_OBJS) \
  $O\ArchiveExports.obj \
  $O\DllExports2.obj \


!include "../../7zip.mak"
