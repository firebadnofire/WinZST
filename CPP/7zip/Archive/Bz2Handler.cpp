// Bz2Handler.cpp

#include "StdAfx.h"

#include "../../Common/ComTry.h"

#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Common/FileStreams.h"

#include "../Compress/BZip2Decoder.h"
#include "../Compress/BZip2Encoder.h"
#include "../Compress/CopyCoder.h"

#include "Common/DummyOutStream.h"
#include "Common/HandlerOut.h"
#include "Common/TarWrapUpdate.h"

#include "../../Windows/ErrorMsg.h"

using namespace NWindows;

namespace NArchive {
namespace NBz2 {

static const char * const kTarBz2Suffixes[] =
{
    ".tar.bz2"
  , ".tbz2"
  , ".tbz"
};

Z7_CLASS_IMP_CHandler_IInArchive_4(
  IArchiveOpenSeq,
  IInArchiveGetStream,
  IOutArchive,
  ISetProperties
)
  CMyComPtr<IInStream> _stream;
  CMyComPtr<ISequentialInStream> _seqStream;
  
  bool _isArc;
  bool _needSeekToStart;
  bool _dataAfterEnd;
  bool _needMoreInput;

  bool _packSize_Defined;
  bool _unpackSize_Defined;
  bool _numStreams_Defined;
  bool _numBlocks_Defined;

  UInt64 _packSize;
  UInt64 _unpackSize;
  UInt64 _numStreams;
  UInt64 _numBlocks;

  bool _tarMode;
  UString _subFileName;

  CSingleMethodProps _props;

  HRESULT DecodeToOutput(ISequentialOutStream *realOutStream,
      ICompressProgressInfo *progress, Int32 &opRes);
  friend class CDecodedTempInStream;

public:
  CHandler(): _tarMode(false) {}
};

static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize
};

static const Byte kArcProps[] =
{
  kpidNumStreams,
  kpidNumBlocks
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPhySize: if (_packSize_Defined) prop = _packSize; break;
    case kpidUnpackSize: if (_unpackSize_Defined) prop = _unpackSize; break;
    case kpidNumStreams: if (_numStreams_Defined) prop = _numStreams; break;
    case kpidNumBlocks: if (_numBlocks_Defined) prop = _numBlocks; break;
    case kpidMainSubfile:
      if (_tarMode)
        prop = (UInt32)0;
      break;
    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc) v |= kpv_ErrorFlags_IsNotArc;
      if (_needMoreInput) v |= kpv_ErrorFlags_UnexpectedEnd;
      if (_dataAfterEnd) v |= kpv_ErrorFlags_DataAfterEnd;
      prop = v;
      break;
    }
    default: break;
  }
  prop.Detach(value);
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems))
{
  *numItems = 1;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPath:
      if (_tarMode && !_subFileName.IsEmpty())
        prop = _subFileName;
      break;
    case kpidPackSize: if (_packSize_Defined) prop = _packSize; break;
    case kpidSize: if (_unpackSize_Defined) prop = _unpackSize; break;
    default: break;
  }
  prop.Detach(value);
  return S_OK;
}

static const unsigned kSignatureCheckSize = 10;

API_FUNC_static_IsArc IsArc_BZip2(const Byte *p, size_t size)
{
  if (size < kSignatureCheckSize)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != 'B' || p[1] != 'Z' || p[2] != 'h' || p[3] < '1' || p[3] > '9')
    return k_IsArc_Res_NO;
  p += 4;
  if (NCompress::NBZip2::IsBlockSig(p))
    return k_IsArc_Res_YES;
  if (NCompress::NBZip2::IsEndSig(p))
    return k_IsArc_Res_YES;
  return k_IsArc_Res_NO;
}
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback))
{
  COM_TRY_BEGIN
  Close();
  if (callback)
  {
    CMyComPtr<IArchiveOpenVolumeCallback> volumeCallback;
    callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&volumeCallback);
    if (volumeCallback)
    {
      NCOM::CPropVariant prop;
      if (volumeCallback->GetProperty(kpidName, &prop) == S_OK
          && prop.vt == VT_BSTR
          && prop.bstrVal != NULL)
        NTarWrap::GetTarSubFileName(prop.bstrVal, kTarBz2Suffixes, Z7_ARRAY_SIZE(kTarBz2Suffixes), _subFileName);
    }
  }
  {
    Byte buf[kSignatureCheckSize];
    RINOK(ReadStream_FALSE(stream, buf, kSignatureCheckSize))
    if (IsArc_BZip2(buf, kSignatureCheckSize) == k_IsArc_Res_NO)
      return S_FALSE;
    _isArc = true;
    _stream = stream;
    _seqStream = stream;
    _needSeekToStart = true;
    _tarMode = !_subFileName.IsEmpty();
  }
  return S_OK;
  COM_TRY_END
}


Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream))
{
  Close();
  _isArc = true;
  _seqStream = stream;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::Close())
{
  _isArc = false;
  _needSeekToStart = false;
  _dataAfterEnd = false;
  _needMoreInput = false;

  _packSize_Defined = false;
  _unpackSize_Defined = false;
  _numStreams_Defined = false;
  _numBlocks_Defined = false;

  _packSize = 0;
  _tarMode = false;
  _subFileName.Empty();

  _seqStream.Release();
  _stream.Release();
  return S_OK;
}

HRESULT CHandler::DecodeToOutput(ISequentialOutStream *realOutStream,
    ICompressProgressInfo *progress, Int32 &opRes)
{
  if (_needSeekToStart)
  {
    if (!_stream)
      return E_FAIL;
    RINOK(InStream_SeekToBegin(_stream))
  }
  else
    _needSeekToStart = true;

  CMyComPtr2_Create<ICompressCoder, NCompress::NBZip2::CDecoder> decoder;

  #ifndef Z7_ST
  RINOK(decoder->SetNumberOfThreads(_props._numThreads))
  #endif

  CMyComPtr2_Create<ISequentialOutStream, CDummyOutStream> outStream;
  outStream->SetStream(realOutStream);
  outStream->Init();

  decoder->FinishMode = true;
  decoder->Base.DecodeAllStreams = true;

  _dataAfterEnd = false;
  _needMoreInput = false;

  HRESULT result = decoder.Interface()->Code(_seqStream, outStream, NULL, NULL, progress);

  if (result != S_FALSE && result != S_OK)
    return result;

  if (decoder->Base.NumStreams == 0)
  {
    _isArc = false;
    result = S_FALSE;
  }
  else
  {
    const UInt64 inProcessedSize = decoder->GetInputProcessedSize();
    UInt64 packSize = inProcessedSize;

    if (decoder->Base.NeedMoreInput)
      _needMoreInput = true;

    if (!decoder->Base.IsBz)
    {
      packSize = decoder->Base.FinishedPackSize;
      if (packSize != inProcessedSize)
        _dataAfterEnd = true;
    }

    _packSize = packSize;
    _unpackSize = decoder->GetOutProcessedSize();
    _numStreams = decoder->Base.NumStreams;
    _numBlocks = decoder->GetNumBlocks();

    _packSize_Defined = true;
    _unpackSize_Defined = true;
    _numStreams_Defined = true;
    _numBlocks_Defined = true;

    if (progress)
      progress->SetRatioInfo(&packSize, &_unpackSize);
  }

  if (!_isArc)
    opRes = NExtract::NOperationResult::kIsNotArc;
  else if (_needMoreInput)
    opRes = NExtract::NOperationResult::kUnexpectedEnd;
  else if (decoder->GetCrcError())
    opRes = NExtract::NOperationResult::kCRCError;
  else if (_dataAfterEnd)
    opRes = NExtract::NOperationResult::kDataAfterEnd;
  else if (result == S_FALSE)
    opRes = NExtract::NOperationResult::kDataError;
  else if (decoder->Base.MinorError)
    opRes = NExtract::NOperationResult::kDataError;
  else if (result == S_OK)
    opRes = NExtract::NOperationResult::kOK;
  else
    return result;

  return S_OK;
}


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  if (_packSize_Defined)
  {
    RINOK(extractCallback->SetTotal(_packSize))
  }

  Int32 opRes;
  CMyComPtr<ISequentialOutStream> realOutStream;
  const Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode))
  if (!testMode && !realOutStream)
    return S_OK;

  RINOK(extractCallback->PrepareOperation(askMode))

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(extractCallback, true);
  RINOK(DecodeToOutput(realOutStream, lps, opRes))
  return extractCallback->SetOperationResult(opRes);

  // } catch(...)  { return E_FAIL; }

  COM_TRY_END
}


Z7_CLASS_IMP_COM_2(
  CDecodedTempInStream,
  IInStream,
  IStreamGetSize
)
  Z7_IFACE_COM7_IMP(ISequentialInStream)

  NFile::NDir::CTempFile _tempFile;
  CMyComPtr2<IInStream, CInFileStream> _inStream;

public:
  HRESULT Create(CHandler *handler);
};

Z7_COM7F_IMF(CDecodedTempInStream::Read(void *data, UInt32 size, UInt32 *processedSize))
{
  return _inStream.Interface()->Read(data, size, processedSize);
}

Z7_COM7F_IMF(CDecodedTempInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  return _inStream.Interface()->Seek(offset, seekOrigin, newPosition);
}

Z7_COM7F_IMF(CDecodedTempInStream::GetSize(UInt64 *size))
{
  return _inStream->GetSize(size);
}

HRESULT CDecodedTempInStream::Create(CHandler *handler)
{
  CMyComPtr2_Create<IOutStream, COutFileStream> outStream;
  if (!_tempFile.CreateRandomInTempFolder(FTEXT("WinZST"), &outStream->File))
    return GetLastError_noZero_HRESULT();

  Int32 opRes;
  const HRESULT hres = handler->DecodeToOutput(outStream.Interface(), NULL, opRes);
  const HRESULT closeRes = outStream->Close();
  if (hres != S_OK)
    return hres;
  RINOK(closeRes)
  if (opRes != NExtract::NOperationResult::kOK)
    return S_FALSE;

  _inStream.Create_if_Empty();
  if (!_inStream->Open(_tempFile.GetPath()))
    return GetLastError_noZero_HRESULT();
  return S_OK;
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream))
{
  COM_TRY_BEGIN

  *stream = NULL;
  if (index != 0)
    return E_INVALIDARG;
  if (!_tarMode || !_stream)
    return S_FALSE;

  CMyComPtr2<ISequentialInStream, CDecodedTempInStream> tempStream;
  tempStream.Create_if_Empty();
  RINOK(tempStream->Create(this))

  *stream = tempStream.Detach();
  return S_OK;

  COM_TRY_END
}


/*
static HRESULT ReportItemProp(IArchiveUpdateCallbackArcProp *reportArcProp, PROPID propID, const PROPVARIANT *value)
{
  return reportArcProp->ReportProp(NEventIndexType::kOutArcIndex, 0, propID, value);
}

static HRESULT ReportArcProp(IArchiveUpdateCallbackArcProp *reportArcProp, PROPID propID, const PROPVARIANT *value)
{
  return reportArcProp->ReportProp(NEventIndexType::kArcProp, 0, propID, value);
}

static HRESULT ReportArcProps(IArchiveUpdateCallbackArcProp *reportArcProp,
    const UInt64 *unpackSize,
    const UInt64 *numBlocks)
{
  NCOM::CPropVariant sizeProp;
  if (unpackSize)
  {
    sizeProp = *unpackSize;
    RINOK(ReportItemProp(reportArcProp, kpidSize, &sizeProp));
    RINOK(reportArcProp->ReportFinished(NEventIndexType::kOutArcIndex, 0, NArchive::NUpdate::NOperationResult::kOK));
  }
 
  if (unpackSize)
  {
    RINOK(ReportArcProp(reportArcProp, kpidSize, &sizeProp));
  }
  if (numBlocks)
  {
    NCOM::CPropVariant prop;
    prop = *numBlocks;
    RINOK(ReportArcProp(reportArcProp, kpidNumBlocks, &prop));
  }
  return S_OK;
}
*/

static HRESULT CompressStream(
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    const CProps &props,
    ISequentialInStream *fileInStream,
    IArchiveUpdateCallback *updateCallback,
    bool setOperationResult
    // , ArchiveUpdateCallbackArcProp *reportArcProp
    )
{
  {
    if (!fileInStream)
      return S_FALSE;
    {
      Z7_DECL_CMyComPtr_QI_FROM(
          IStreamGetSize,
          streamGetSize, fileInStream)
      if (streamGetSize)
      {
        UInt64 size;
        if (streamGetSize->GetSize(&size) == S_OK)
          unpackSize = size;
      }
    }
    RINOK(updateCallback->SetTotal(unpackSize))

    CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
    lps->Init(updateCallback, true);
    {
      CMyComPtr2_Create<ICompressCoder, NCompress::NBZip2::CEncoder> encoder;
      RINOK(props.SetCoderProps(encoder.ClsPtr(), NULL))
      RINOK(encoder.Interface()->Code(fileInStream, outStream, NULL, NULL, lps))
      /*
      if (reportArcProp)
      {
        unpackSize = encoderSpec->GetInProcessedSize();
        RINOK(ReportArcProps(reportArcProp, &unpackSize, &encoderSpec->NumBlocks));
      }
      */
    }
  }
  if (setOperationResult)
    return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
  return S_OK;
}

static HRESULT UpdateArchive(
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    const CProps &props,
    IArchiveUpdateCallback *updateCallback
    // , ArchiveUpdateCallbackArcProp *reportArcProp
    )
{
  CMyComPtr<ISequentialInStream> fileInStream;
  RINOK(updateCallback->GetStream(0, &fileInStream))
  return CompressStream(unpackSize, outStream, props, fileInStream, updateCallback, true);
}

Z7_COM7F_IMF(CHandler::GetFileTimeType(UInt32 *timeType))
{
  *timeType = GET_FileTimeType_NotDefined_for_GetFileTimeType;
  // *timeType = NFileTimeType::kUnix;
  return S_OK;
}

Z7_COM7F_IMF(CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback))
{
  COM_TRY_BEGIN

  if (!updateCallback)
    return E_FAIL;

  if (NTarWrap::IsTarUpdate(updateCallback, kTarBz2Suffixes, Z7_ARRAY_SIZE(kTarBz2Suffixes)))
  {
    {
      Z7_DECL_CMyComPtr_QI_FROM(
          IStreamSetRestriction,
          setRestriction, outStream)
      if (setRestriction)
        RINOK(setRestriction->SetRestriction(0, 0))
    }

    NTarWrap::CTempTarInStream tarStream;
    RINOK(tarStream.Create(numItems, updateCallback, false))

    CMethodProps props2 = _props;
#ifndef Z7_ST
    props2.AddProp_NumThreads(_props._numThreads);
#ifdef _WIN32
    if (_props._numThreadGroups > 1)
      props2.AddProp32(NCoderPropID::kNumThreadGroups, _props._numThreadGroups);
#endif
#endif
    return CompressStream(tarStream.GetSize(), outStream, props2,
        tarStream.GetStream(), updateCallback, false);
  }

  if (numItems != 1)
    return E_INVALIDARG;

  {
    Z7_DECL_CMyComPtr_QI_FROM(
        IStreamSetRestriction,
        setRestriction, outStream)
    if (setRestriction)
      RINOK(setRestriction->SetRestriction(0, 0))
  }

  Int32 newData, newProps;
  UInt32 indexInArchive;
  RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProps, &indexInArchive))
 
  // Z7_DECL_CMyComPtr_QI_FROM(IArchiveUpdateCallbackArcProp, reportArcProp, updateCallback)

  if (IntToBool(newProps))
  {
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidIsDir, &prop))
      if (prop.vt != VT_EMPTY)
        if (prop.vt != VT_BOOL || prop.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
    }
  }
  
  if (IntToBool(newData))
  {
    UInt64 size;
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidSize, &prop))
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      size = prop.uhVal.QuadPart;
    }

    CMethodProps props2 = _props;
#ifndef Z7_ST
    props2.AddProp_NumThreads(_props._numThreads);
#ifdef _WIN32
    if (_props._numThreadGroups > 1)
      props2.AddProp32(NCoderPropID::kNumThreadGroups, _props._numThreadGroups);
#endif
#endif

    return UpdateArchive(size, outStream, props2, updateCallback);
  }

  if (indexInArchive != 0)
    return E_INVALIDARG;

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(updateCallback, true);

  Z7_DECL_CMyComPtr_QI_FROM(
      IArchiveUpdateCallbackFile,
      opCallback, updateCallback)
  if (opCallback)
  {
    RINOK(opCallback->ReportOperation(
        NEventIndexType::kInArcIndex, 0,
        NUpdateNotifyOp::kReplicate))
  }

  if (_stream)
    RINOK(InStream_SeekToBegin(_stream))

  return NCompress::CopyStream(_stream, outStream, lps);

  // return ReportArcProps(reportArcProp, NULL, NULL);

  COM_TRY_END
}

Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  return _props.SetProperties(names, values, numProps);
}

static const Byte k_Signature[] = { 'B', 'Z', 'h' };

REGISTER_ARC_IO(
  "bzip2", "bz2 bzip2 tbz2 tbz", "* * .tar .tar", 2,
  k_Signature,
  0,
  NArcInfoFlags::kKeepName
  , 0
  , IsArc_BZip2)

}}
