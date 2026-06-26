// TarWrapUpdate.cpp

#include "StdAfx.h"

#include "../../../Common/ComTry.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/MyCom.h"

#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/PropVariant.h"

#include "../Tar/TarHandler.h"

#include "TarWrapUpdate.h"

namespace NArchive {
namespace NTarWrap {

static bool StringsEqualNoCase_Ascii(const UString &s, const char *ascii)
{
  unsigned i;
  for (i = 0; ascii[i] != 0; i++)
  {
    if (i >= s.Len())
      return false;
    const wchar_t c = s[i];
    const char a = ascii[i];
    if (c >= 0x80)
      return false;
    if (MyCharLower_Ascii((char)c) != MyCharLower_Ascii(a))
      return false;
  }
  return i == s.Len();
}

bool IsNameWithSuffix(const UString &name, const char *suffix)
{
  const unsigned suffixLen = MyStringLen(suffix);
  if (name.Len() < suffixLen)
    return false;
  return StringsEqualNoCase_Ascii(name.Ptr(name.Len() - suffixLen), suffix);
}

bool IsNameWithSuffixes(const UString &name, const char * const *suffixes, unsigned numSuffixes)
{
  for (unsigned i = 0; i < numSuffixes; i++)
    if (IsNameWithSuffix(name, suffixes[i]))
      return true;
  return false;
}

static bool RemoveSuffix_Ascii_NoCase(UString &name, const char *suffix)
{
  if (!IsNameWithSuffix(name, suffix))
    return false;
  name.DeleteFrom(name.Len() - MyStringLen(suffix));
  return true;
}

bool GetTarSubFileName(const UString &name, const char * const *suffixes, unsigned numSuffixes, UString &subFileName)
{
  for (unsigned i = 0; i < numSuffixes; i++)
  {
    UString temp = name;
    if (RemoveSuffix_Ascii_NoCase(temp, suffixes[i]))
    {
      temp += ".tar";
      subFileName = temp;
      return true;
    }
  }
  subFileName.Empty();
  return false;
}

bool IsTarUpdate(IArchiveUpdateCallback *updateCallback, const char * const *suffixes, unsigned numSuffixes)
{
  if (!updateCallback)
    return false;

  CMyComPtr<IArchiveGetRootProps> rootProps;
  updateCallback->QueryInterface(IID_IArchiveGetRootProps, (void **)&rootProps);
  if (!rootProps)
    return false;

  NWindows::NCOM::CPropVariant prop;
  if (rootProps->GetRootProp(kpidArcFileName, &prop) != S_OK)
    return false;
  if (prop.vt != VT_BSTR || prop.bstrVal == NULL)
    return false;

  return IsNameWithSuffixes(prop.bstrVal, suffixes, numSuffixes);
}

HRESULT CTempTarInStream::Create(UInt32 numItems, IArchiveUpdateCallback *updateCallback, bool posixMode)
{
  _inStream.SetFromCls(NULL);
  _size = 0;

  CMyComPtr2_Create<IOutStream, COutFileStream> outStream;
  if (!_tempFile.CreateRandomInTempFolder(FTEXT("WinZST"), &outStream->File))
    return GetLastError_noZero_HRESULT();

  CMyComPtr<IOutArchive> tarArchive = new NTar::CHandler;
  {
    CMyComPtr<ISetProperties> setProperties;
    RINOK(tarArchive.QueryInterface(IID_ISetProperties, &setProperties))

    const wchar_t *name = L"m";
    NWindows::NCOM::CPropVariant value(posixMode ? L"posix" : L"gnu");
    RINOK(setProperties->SetProperties(&name, &value, 1))
  }

  RINOK(tarArchive->UpdateItems(outStream, numItems, updateCallback))
  RINOK(outStream->Close())

  _inStream.Create_if_Empty();
  if (!_inStream->Open(_tempFile.GetPath()))
    return GetLastError_noZero_HRESULT();

  RINOK(_inStream->GetSize(&_size))
  return S_OK;
}

}}
