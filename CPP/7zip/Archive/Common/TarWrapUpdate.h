// TarWrapUpdate.h

#ifndef ZIP7_INC_ARCHIVE_COMMON_TAR_WRAP_UPDATE_H
#define ZIP7_INC_ARCHIVE_COMMON_TAR_WRAP_UPDATE_H

#include "../../../Common/MyCom.h"
#include "../../../Common/MyString.h"

#include "../../../Windows/FileDir.h"

#include "../../IStream.h"
#include "../IArchive.h"
#include "../../Common/FileStreams.h"

namespace NArchive {
namespace NTarWrap {

bool IsNameWithSuffix(const UString &name, const char *suffix);
bool IsNameWithSuffixes(const UString &name, const char * const *suffixes, unsigned numSuffixes);
bool GetTarSubFileName(const UString &name, const char * const *suffixes, unsigned numSuffixes, UString &subFileName);
bool IsTarUpdate(IArchiveUpdateCallback *updateCallback, const char * const *suffixes, unsigned numSuffixes);

class CTempTarInStream
{
  NWindows::NFile::NDir::CTempFile _tempFile;
  CMyComPtr2<IInStream, CInFileStream> _inStream;
  UInt64 _size;

public:
  CTempTarInStream(): _size(0) {}

  HRESULT Create(UInt32 numItems, IArchiveUpdateCallback *updateCallback, bool posixMode);

  ISequentialInStream *GetStream() const { return _inStream.Interface(); }
  UInt64 GetSize() const { return _size; }
};

}}

#endif
