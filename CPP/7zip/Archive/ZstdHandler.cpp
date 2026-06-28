// ZstdHandler.cpp

#include "StdAfx.h"

// #define Z7_USE_ZSTD_ORIG_DECODER

#include "../../Common/ComTry.h"
#include "../../Common/MyBuffer.h"

#include "../Common/MethodProps.h"
#include "../Common/FileStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"
#include "../Compress/ZstdDecoder.h"
#ifdef Z7_USE_ZSTD_ORIG_DECODER
#include "../Compress/Zstd2Decoder.h"
#endif

#include "Common/DummyOutStream.h"
#include "Tar/TarHandler.h"

#include "../../../C/CpuArch.h"
#include "../../../C/zstd/lib/zstd.h"

#include "../../Windows/FileDir.h"

using namespace NWindows;

namespace NArchive {
namespace NZstd {

static const int kZstdDefaultLevel = 5;

class CDecodedTempInStream;

#define DESCRIPTOR_Get_DictionaryId_Flag(d)   ((d) & 3)
#define DESCRIPTOR_FLAG_CHECKSUM              (1 << 2)
#define DESCRIPTOR_FLAG_RESERVED              (1 << 3)
#define DESCRIPTOR_FLAG_UNUSED                (1 << 4)
#define DESCRIPTOR_FLAG_SINGLE                (1 << 5)
#define DESCRIPTOR_Get_ContentSize_Flag3(d)   ((d) >> 5)
#define DESCRIPTOR_Is_ContentSize_Defined(d)  (((d) & 0xe0) != 0)

struct CFrameHeader
{
  Byte Descriptor;
  Byte WindowDescriptor;
  UInt32 DictionaryId;
  UInt64 ContentSize;

  /* by zstd specification:
      the decoder must check that (Is_Reserved() == false)
      the decoder must ignore Unused_bit */
  bool Is_Reserved() const                { return (Descriptor & DESCRIPTOR_FLAG_RESERVED) != 0; }
  bool Is_Checksum() const                { return (Descriptor & DESCRIPTOR_FLAG_CHECKSUM) != 0; }
  bool Is_SingleSegment() const           { return (Descriptor & DESCRIPTOR_FLAG_SINGLE) != 0; }
  bool Is_ContentSize_Defined() const     { return DESCRIPTOR_Is_ContentSize_Defined(Descriptor); }
  unsigned Get_DictionaryId_Flag() const  { return DESCRIPTOR_Get_DictionaryId_Flag(Descriptor); }
  unsigned Get_ContentSize_Flag3() const  { return DESCRIPTOR_Get_ContentSize_Flag3(Descriptor); }
  
  const Byte *Parse(const Byte *p, int size)
  {
    if ((unsigned)size < 2)
      return NULL;
    Descriptor = *p++;
    size--;
    {
      Byte w = 0;
      if (!Is_SingleSegment())
      {
        w = *p++;
        size--;
      }
      WindowDescriptor = w;
    }
    {
      unsigned n = Get_DictionaryId_Flag();
      UInt32 d = 0;
      if (n)
      {
        n = (unsigned)1 << (n - 1);
        if ((size -= (int)n) < 0)
          return NULL;
        d = GetUi32(p) & ((UInt32)(Int32)-1 >> (32 - 8u * n));
        p += n;
      }
      DictionaryId = d;
    }
    {
      unsigned n = Get_ContentSize_Flag3();
      UInt64 v = 0;
      if (n)
      {
        n >>= 1;
        if (n == 1)
          v = 256;
        n = (unsigned)1 << n;
        if ((size -= (int)n) < 0)
          return NULL;
        v += GetUi64(p) & ((UInt64)(Int64)-1 >> (64 - 8u * n));
        p += n;
      }
      ContentSize = v;
    }
    return p;
  }
};



class CHandler Z7_final:
  public IInArchive,
  public IArchiveOpenSeq,
  public IInArchiveGetStream,
  public ISetProperties,
#ifndef Z7_EXTRACT_ONLY
  public IOutArchive,
#endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(IInArchive)
  Z7_COM_QI_ENTRY(IArchiveOpenSeq)
  Z7_COM_QI_ENTRY(IInArchiveGetStream)
  Z7_COM_QI_ENTRY(ISetProperties)
#ifndef Z7_EXTRACT_ONLY
  Z7_COM_QI_ENTRY(IOutArchive)
#endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE
  
  Z7_IFACE_COM7_IMP(IInArchive)
  Z7_IFACE_COM7_IMP(IArchiveOpenSeq)
  Z7_IFACE_COM7_IMP(IInArchiveGetStream)
  Z7_IFACE_COM7_IMP(ISetProperties)
#ifndef Z7_EXTRACT_ONLY
  Z7_IFACE_COM7_IMP(IOutArchive)
#endif

  bool _isArc;
  bool _needSeekToStart;
  // bool _dataAfterEnd;
  // bool _needMoreInput;
  bool _unsupportedBlock;

  bool _wasParsed;
  bool _phySize_Decoded_Defined;
  bool _unpackSize_Defined; // decoded
  bool _decoded_Info_Defined;
  
  bool _parseMode;
  bool _disableHash;
  bool _tarMode;
  bool _tarPosixMode;
  // bool _smallMode;

  int _level;
  UInt64 _phySize;
  UInt64 _phySize_Decoded;
  UInt64 _unpackSize;

  CZstdDecInfo _parsed_Info;
  CZstdDecInfo _decoded_Info;

  CMyComPtr<IInStream> _stream;
  CMyComPtr<ISequentialInStream> _seqStream;
  UString _subFileName;

  HRESULT DecodeToOutput(ISequentialOutStream *outStream, ICompressProgressInfo *progress,
      UInt64 &inSize, UInt64 &outSize, SRes &decodeSRes, bool &extraSize);
  friend class CDecodedTempInStream;

public:
  CHandler():
    _parseMode(false),
    _disableHash(false),
    _tarMode(false),
    _tarPosixMode(false),
    _level(kZstdDefaultLevel)
    // _smallMode(false)
    {}
};


static const Byte kProps[] =
{
  kpidSize,
  kpidPackSize
};

static const Byte kArcProps[] =
{
  kpidNumStreams,
  kpidNumBlocks,
  kpidMethod,
  // kpidChecksum
  kpidCRC
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps


// static const unsigned kBlockType_Raw = 0;
static const unsigned kBlockType_RLE = 1;
// static const unsigned kBlockType_Compressed = 2;
static const unsigned kBlockType_Reserved = 3;
/*
static const char * const kNames[] =
{
    "RAW"
  , "RLE"
  , "Compressed"
  , "Reserved"
};
*/

static bool StringEndsWith_Ascii_NoCase(const UString &s, const char *suffix)
{
  unsigned len = 0;
  while (suffix[len] != 0)
    len++;
  if (s.Len() < len)
    return false;
  return StringsAreEqualNoCase_Ascii(s.RightPtr(len), suffix);
}

static bool RemoveSuffix_Ascii_NoCase(UString &s, const char *suffix)
{
  unsigned len = 0;
  while (suffix[len] != 0)
    len++;
  if (s.Len() <= len)
    return false;
  if (!StringsAreEqualNoCase_Ascii(s.RightPtr(len), suffix))
    return false;
  s.DeleteFrom(s.Len() - len);
  return true;
}

static bool IsTarZstdName(const UString &name)
{
  return
      StringEndsWith_Ascii_NoCase(name, ".tzs")
   || StringEndsWith_Ascii_NoCase(name, ".tzst")
   || StringEndsWith_Ascii_NoCase(name, ".tzstd")
   || StringEndsWith_Ascii_NoCase(name, ".tar.zst");
}

static bool GetTarZstdSubFileName(const UString &name, UString &subFileName)
{
  subFileName = name;
  if (RemoveSuffix_Ascii_NoCase(subFileName, ".tar.zst"))
  {
    subFileName += ".tar";
    return true;
  }
  if (RemoveSuffix_Ascii_NoCase(subFileName, ".tzs")
      || RemoveSuffix_Ascii_NoCase(subFileName, ".tzst")
      || RemoveSuffix_Ascii_NoCase(subFileName, ".tzstd"))
  {
    subFileName += ".tar";
    return true;
  }
  subFileName.Empty();
  return false;
}

static void Add_UInt64(AString &s, const char *name, UInt64 v)
{
  s.Add_OptSpaced(name);
  s.Add_Colon();
  s.Add_UInt64(v);
}


static void PrintSize(AString &s, UInt64 w)
{
  char c = 0;
       if ((w & ((1 << 30) - 1)) == 0)  { c = 'G'; w >>= 30; }
  else if ((w & ((1 << 20) - 1)) == 0)  { c = 'M'; w >>= 20; }
  else if ((w & ((1 << 10) - 1)) == 0)  { c = 'K'; w >>= 10; }
  s.Add_UInt64(w);
  if (c)
  {
    s.Add_Char(c);
    s += "iB";
  }
}


Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value))
{
  NCOM::CPropVariant prop;

  CZstdDecInfo *p = NULL;
  if (_wasParsed || !_decoded_Info_Defined)
    p = &_parsed_Info;
  else if (_decoded_Info_Defined)
    p = &_decoded_Info;

  switch (propID)
  {
    case kpidPhySize:
      if (_wasParsed)
        prop = _phySize;
      else if (_phySize_Decoded_Defined)
        prop = _phySize_Decoded;
      break;

    case kpidUnpackSize:
      if (_unpackSize_Defined)
        prop = _unpackSize;
      break;
    
    case kpidNumStreams:
      if (p)
      if (_wasParsed || _decoded_Info_Defined)
        prop = p->num_DataFrames;
      break;
     
    case kpidNumBlocks:
      if (p)
      if (_wasParsed || _decoded_Info_Defined)
        prop = p->num_Blocks;
      break;
    
    // case kpidChecksum:
    case kpidCRC:
      if (p)
        if (p->checksum_Defined && p->num_DataFrames == 1)
          prop = p->checksum; // it's checksum from last frame
      break;

    case kpidMethod:
    {
      AString s;
      s.Add_OptSpaced(p == &_decoded_Info ?
          "decoded:" : _wasParsed ?
          "parsed:" :
          "header-open-only:");
      
      if (p->dictionaryId != 0)
      {
        if (p->are_DictionaryId_Different)
          s.Add_OptSpaced("different-dictionary-IDs");
        s.Add_OptSpaced("dictionary-ID:");
        s.Add_UInt32(p->dictionaryId);
      }
      /*
      if (ContentSize_Defined)
      {
        s.Add_OptSpaced("ContentSize=");
        s.Add_UInt64(ContentSize_Total);
      }
      */
      // if (p->are_Checksums)
      if (p->descriptor_OR & DESCRIPTOR_FLAG_CHECKSUM)
        s.Add_OptSpaced("XXH64");
      if (p->descriptor_NOT_OR & DESCRIPTOR_FLAG_CHECKSUM)
        s.Add_OptSpaced("NO-XXH64");

      if (p->descriptor_OR & DESCRIPTOR_FLAG_UNUSED)
        s.Add_OptSpaced("unused_bit");

      if (p->descriptor_OR & DESCRIPTOR_FLAG_SINGLE)
        s.Add_OptSpaced("single-segments");

      if (p->descriptor_NOT_OR & DESCRIPTOR_FLAG_SINGLE)
      {
        // Add_UInt64(s, "wnd-descriptors", p->num_WindowDescriptors);
        s.Add_OptSpaced("wnd-desc-log-MAX:");
        // WindowDescriptor_MAX = 16 << 3; // for debug
        const unsigned e = p->windowDescriptor_MAX >> 3;
        s.Add_UInt32(e + 10);
        const unsigned m = p->windowDescriptor_MAX & 7;
        if (m != 0)
        {
          s.Add_Dot();
          s.Add_UInt32(m);
        }
      }
      
      if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor_OR) ||
          (p->descriptor_NOT_OR & DESCRIPTOR_FLAG_SINGLE))
      /*
      if (p->are_ContentSize_Known ||
          p->are_WindowDescriptors)
      */
      {
        s.Add_OptSpaced("wnd-MAX:");
        PrintSize(s, p->windowSize_MAX);
        if (p->windowSize_MAX != p->windowSize_Allocate_MAX)
        {
          s.Add_OptSpaced("wnd-use-MAX:");
          PrintSize(s, p->windowSize_Allocate_MAX);
        }
      }

      if (p->num_DataFrames != 1)
        Add_UInt64(s, "data-frames", p->num_DataFrames);
      if (p->num_SkipFrames != 0)
      {
        Add_UInt64(s, "skip-frames", p->num_SkipFrames);
        Add_UInt64(s, "skip-frames-size-total", p->skipFrames_Size);
      }

      if (p->are_ContentSize_Unknown)
        s.Add_OptSpaced("unknown-content-size");

      if (DESCRIPTOR_Is_ContentSize_Defined(p->descriptor_OR))
      {
        Add_UInt64(s, "content-size-frame-max", p->contentSize_MAX);
        Add_UInt64(s, "content-size-total", p->contentSize_Total);
      }
     
      /*
      for (unsigned i = 0; i < 4; i++)
      {
        const UInt64 n = p->num_Blocks_forType[i];
        if (n)
        {
          s.Add_OptSpaced(kNames[i]);
          s += "-blocks:";
          s.Add_UInt64(n);

          s.Add_OptSpaced(kNames[i]);
          s += "-block-bytes:";
          s.Add_UInt64(p->num_BlockBytes_forType[i]);
        }
      }
      */
      prop = s;
      break;
    }

    case kpidErrorFlags:
    {
      UInt32 v = 0;
      if (!_isArc)            v |= kpv_ErrorFlags_IsNotArc;
      // if (_needMoreInput)     v |= kpv_ErrorFlags_UnexpectedEnd;
      // if (_dataAfterEnd)      v |= kpv_ErrorFlags_DataAfterEnd;
      if (_unsupportedBlock)  v |= kpv_ErrorFlags_UnsupportedMethod;
      /*
      if (_parsed_Info.numBlocks_forType[kBlockType_Reserved])
        v |= kpv_ErrorFlags_UnsupportedMethod;
      */
      prop = v;
      break;
    }
    case kpidMainSubfile:
      if (_tarMode)
        prop = (UInt32)0;
      break;

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

    case kpidPackSize:
      if (_wasParsed)
        prop = _phySize;
      else if (_phySize_Decoded_Defined)
        prop = _phySize_Decoded;
      break;

    case kpidSize:
      if (_wasParsed && !_parsed_Info.are_ContentSize_Unknown)
        prop = _parsed_Info.contentSize_Total;
      else if (_unpackSize_Defined)
        prop = _unpackSize;
      break;

    default: break;
  }
  prop.Detach(value);
  return S_OK;
}

static const unsigned kSignatureSize = 4;
static const Byte k_Signature[kSignatureSize] = { 0x28, 0xb5, 0x2f, 0xfd } ;

static const UInt32 kDataFrameSignature32    = 0xfd2fb528;
static const UInt32 kSkipFrameSignature      = 0x184d2a50;
static const UInt32 kSkipFrameSignature_Mask = 0xfffffff0;

/*
API_FUNC_static_IsArc IsArc_Zstd(const Byte *p, size_t size)
{
  if (size < kSignatureSize)
    return k_IsArc_Res_NEED_MORE;
  if (memcmp(p, k_Signature, kSignatureSize) != 0)
  {
    const UInt32 v = GetUi32(p);
    if ((v & kSkipFrameSignature_Mask) != kSkipFrameSignature)
      return k_IsArc_Res_NO;
    return k_IsArc_Res_YES;
  }
  p += 4;
  // return k_IsArc_Res_YES;
}
}
*/

// kBufSize must be >= (ZSTD_FRAMEHEADERSIZE_MAX = 18)
// we use big buffer for fast parsing of worst case small blocks.
static const unsigned kBufSize =
     1 << 9;
     // 1 << 14; // fastest in real file

struct CStreamBuffer
{
  unsigned pos;
  unsigned lim;
  IInStream *Stream;
  UInt64 StreamOffset;
  Byte buf[kBufSize];

  CStreamBuffer():
      pos(0),
      lim(0),
      StreamOffset(0)
      {}
  unsigned Avail() const { return lim - pos; }
  const Byte *GetPtr() const { return &buf[pos]; }
  UInt64 GetCurOffset() const { return StreamOffset - Avail(); }
  void SkipInBuf(UInt32 size) { pos += size; }
  HRESULT Skip(UInt32 size);
  HRESULT Read(unsigned num);
};

HRESULT CStreamBuffer::Skip(UInt32 size)
{
  unsigned rem = lim - pos;
  if (rem != 0)
  {
    if (rem > size)
      rem = size;
    pos += rem;
    size -= rem;
    if (pos != lim)
      return S_OK;
  }
  if (size == 0)
    return S_OK;
  return Stream->Seek(size, STREAM_SEEK_CUR, &StreamOffset);
}

HRESULT CStreamBuffer::Read(unsigned num)
{
  if (lim - pos >= num)
    return S_OK;
  if (pos != 0)
  {
    lim -= pos;
    memmove(buf, buf + pos, lim);
    pos = 0;
  }
  size_t processed = kBufSize - ((unsigned)StreamOffset & (kBufSize - 1));
  const unsigned avail = kBufSize - lim;
  num -= lim;
  if (avail < processed || processed < num)
    processed = avail;
  const HRESULT res = ReadStream(Stream, buf + lim, &processed);
  StreamOffset += processed;
  lim += (unsigned)processed;
  return res;
}


static const unsigned k_ZSTD_FRAMEHEADERSIZE_MAX = 4 + 14;

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
      {
        _tarMode = GetTarZstdSubFileName(prop.bstrVal, _subFileName);
      }
    }
  }

  CZstdDecInfo *p = &_parsed_Info;
  // p->are_ContentSize_Unknown = False;
  CStreamBuffer sb;
  sb.Stream = stream;
  
  for (;;)
  {
    RINOK(sb.Read(k_ZSTD_FRAMEHEADERSIZE_MAX))
    if (sb.Avail() < kSignatureSize)
      break;

    if (callback && (ZstdDecInfo_GET_NUM_FRAMES(p) & 0xFFF) == 2)
    {
      const UInt64 numBytes = sb.GetCurOffset();
      RINOK(callback->SetCompleted(NULL, &numBytes))
    }

    const UInt32 v = GetUi32(sb.GetPtr());
    if (v != kDataFrameSignature32)
    {
      if ((v & kSkipFrameSignature_Mask) != kSkipFrameSignature)
        break;
      _phySize = sb.GetCurOffset() + 8;
      p->num_SkipFrames++;
      sb.SkipInBuf(4);
      if (sb.Avail() < 4)
        break;
      const UInt32 size = GetUi32(sb.GetPtr());
      p->skipFrames_Size += size;
      sb.SkipInBuf(4);
      _phySize = sb.GetCurOffset() + size;
      RINOK(sb.Skip(size))
      continue;
    }

    p->num_DataFrames++;
    // _numStreams_Defined = true;
    sb.SkipInBuf(4);
    CFrameHeader fh;
    {
      const Byte *data = fh.Parse(sb.GetPtr(), (int)sb.Avail());
      if (!data)
      {
        // _needMoreInput = true;
        // we set size for one byte more to show that stream was truncated
        _phySize = sb.StreamOffset + 1;
        break;
      }
      if (fh.Is_Reserved())
      {
        // we don't want false detection
        if (ZstdDecInfo_GET_NUM_FRAMES(p) == 1)
          return S_FALSE;
        // _phySize = sb.GetCurOffset();
        break;
      }
      sb.SkipInBuf((unsigned)(data - sb.GetPtr()));
    }

    p->descriptor_OR     = (Byte)(p->descriptor_OR     |  fh.Descriptor);
    p->descriptor_NOT_OR = (Byte)(p->descriptor_NOT_OR | ~fh.Descriptor);

    // _numBlocks_Defined = true;
    // if (fh.Get_DictionaryId_Flag())
    // p->dictionaryId_Cur = fh.DictionaryId;
    if (fh.DictionaryId != 0)
    {
      if (p->dictionaryId == 0)
        p->dictionaryId = fh.DictionaryId;
      else if (p->dictionaryId != fh.DictionaryId)
        p->are_DictionaryId_Different = True;
    }

    UInt32 blockSizeAllowedMax = (UInt32)1 << 17;
    {
      UInt64 winSize = fh.ContentSize;
      UInt64 winSize_forAllocate = fh.ContentSize;
      if (!fh.Is_SingleSegment())
      {
        if (p->windowDescriptor_MAX < fh.WindowDescriptor)
            p->windowDescriptor_MAX = fh.WindowDescriptor;
        const unsigned e = (fh.WindowDescriptor >> 3);
        const unsigned m = (fh.WindowDescriptor & 7);
        winSize = (UInt64)(8 + m) << (e + 10 - 3);
        if (!fh.Is_ContentSize_Defined()
            || fh.DictionaryId != 0
            || winSize_forAllocate > winSize)
          winSize_forAllocate = winSize;
        // p->are_WindowDescriptors = true;
      }
      else
      {
        // p->are_SingleSegments = True;
      }
      if (blockSizeAllowedMax > winSize)
          blockSizeAllowedMax = (UInt32)winSize;
      if (p->windowSize_MAX < winSize)
          p->windowSize_MAX = winSize;
      if (p->windowSize_Allocate_MAX < winSize_forAllocate)
          p->windowSize_Allocate_MAX = winSize_forAllocate;
    }

    if (fh.Is_ContentSize_Defined())
    {
      // p->are_ContentSize_Known = True;
      p->contentSize_Total += fh.ContentSize;
      if (p->contentSize_MAX < fh.ContentSize)
          p->contentSize_MAX = fh.ContentSize;
    }
    else
    {
      p->are_ContentSize_Unknown = True;
    }

    p->checksum_Defined = false;

    // p->numBlocks_forType[3] += 99; // for debug

    if (!_parseMode)
    {
      if (ZstdDecInfo_GET_NUM_FRAMES(p) == 1)
        break;
    }

    _wasParsed = true;
     
    bool blocksWereParsed = false;

    for (;;)
    {
      if (callback && (p->num_Blocks & 0xFFF) == 2)
      {
        // Sleep(10);
        const UInt64 numBytes = sb.GetCurOffset();
        RINOK(callback->SetCompleted(NULL, &numBytes))
      }
      _phySize = sb.GetCurOffset() + 3;
      RINOK(sb.Read(3))
      if (sb.Avail() < 3)
      {
        // _needMoreInput = true;
        // return S_FALSE;
        break; // change it
      }
      const unsigned pos = sb.pos;
      sb.pos = pos + 3;
      UInt32 b = 0;
      b += (UInt32)sb.buf[pos];
      b += (UInt32)sb.buf[pos + 1] << (8 * 1);
      b += (UInt32)sb.buf[pos + 2] << (8 * 2);
      p->num_Blocks++;
      const unsigned blockType = (b >> 1) & 3;
      UInt32 size = b >> 3;
      // p->num_Blocks_forType[blockType]++;
      // p->num_BlockBytes_forType[blockType] += size;
      if (size > blockSizeAllowedMax
          || blockType == kBlockType_Reserved)
      {
        _unsupportedBlock = true;
        if (ZstdDecInfo_GET_NUM_FRAMES(p) == 1 && p->num_Blocks == 1)
          return S_FALSE;
        break;
      }
      if (blockType == kBlockType_RLE)
        size = 1;
      _phySize = sb.GetCurOffset() + size;
      RINOK(sb.Skip(size))
      if (b & 1)
      {
        // it's last block
        blocksWereParsed = true;
        break;
      }
    }

    if (!blocksWereParsed)
      break;

    if (fh.Is_Checksum())
    {
      _phySize = sb.GetCurOffset() + 4;
      RINOK(sb.Read(4))
      if (sb.Avail() < 4)
        break;
      p->checksum_Defined = true;
      // if (p->num_DataFrames == 1)
      p->checksum = GetUi32(sb.GetPtr());
      sb.SkipInBuf(4);
    }
  }
  
  if (ZstdDecInfo_GET_NUM_FRAMES(p) == 0)
    return S_FALSE;

  _needSeekToStart = true;
  // } // _parseMode
  _isArc = true;
  _stream = stream;
  _seqStream = stream;

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
  // _dataAfterEnd = false;
  // _needMoreInput = false;
  _unsupportedBlock = false;

  _wasParsed = false;
  _phySize_Decoded_Defined = false;
  _unpackSize_Defined = false;
  _decoded_Info_Defined = false;
  _tarMode = false;
  _subFileName.Empty();

  ZstdDecInfo_CLEAR(&_parsed_Info)
  ZstdDecInfo_CLEAR(&_decoded_Info)

  _phySize = 0;
  _phySize_Decoded = 0;
  _unpackSize = 0;

  _seqStream.Release();
  _stream.Release();
  return S_OK;
}

HRESULT CHandler::DecodeToOutput(ISequentialOutStream *outStream, ICompressProgressInfo *progress,
    UInt64 &inSize, UInt64 &outSize, SRes &decodeSRes, bool &extraSize)
{
  if (_needSeekToStart)
  {
    if (!_stream)
      return E_FAIL;
    RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL))
  }
  else
    _needSeekToStart = true;

#ifdef Z7_USE_ZSTD_ORIG_DECODER
  CMyComPtr2_Create<ICompressCoder, NCompress::NZstd2::CDecoder> decoder;
#else
  CMyComPtr2_Create<ICompressCoder, NCompress::NZstd::CDecoder> decoder;
#endif

  CMyComPtr2_Create<ISequentialOutStream, CDummyOutStream> outStreamSpec;
  outStreamSpec->SetStream(outStream);
  outStreamSpec->Init();

  decoder->FinishMode = true;
#ifndef Z7_USE_ZSTD_ORIG_DECODER
  decoder->DisableHash = _disableHash;
#endif

  const HRESULT hres = decoder.Interface()->Code(_seqStream, outStreamSpec, NULL, NULL, progress);
  outSize = outStreamSpec->GetSize();

  decodeSRes = SZ_OK;
  extraSize = false;
  inSize = 0;

  if (hres == S_OK || hres == S_FALSE)
  {
#ifndef Z7_USE_ZSTD_ORIG_DECODER
    _decoded_Info_Defined = true;
    _decoded_Info = decoder->_state.info;
    inSize = decoder->_inProcessed;
    decodeSRes = decoder->ResInfo.decode_SRes;
    extraSize = (decoder->ResInfo.extraSize != 0);
#else
    inSize = decoder->GetInputProcessedSize();
#endif
    _phySize_Decoded = inSize;
    _phySize_Decoded_Defined = true;

    _unpackSize_Defined = true;
    _unpackSize = outSize;

    if (progress)
      progress->SetRatioInfo(&inSize, &outSize);
  }

  return hres;
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

  UInt64 inSize, outSize;
  SRes decodeSRes;
  bool extraSize;
  const HRESULT hres = handler->DecodeToOutput(outStream.Interface(), NULL,
      inSize, outSize, decodeSRes, extraSize);
  const HRESULT closeRes = outStream->Close();
  if (hres != S_OK)
    return hres;
  RINOK(closeRes)
  if (decodeSRes != SZ_OK || extraSize)
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


Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems,
  Int32 testMode, IArchiveExtractCallback *extractCallback))
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;
  if (_wasParsed)
  {
    RINOK(extractCallback->SetTotal(_phySize))
  }

  Int32 opRes;
  {
    CMyComPtr<ISequentialOutStream> realOutStream;
    const Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    RINOK(extractCallback->GetStream(0, &realOutStream, askMode))
    if (!testMode && !realOutStream)
      return S_OK;
      
    extractCallback->PrepareOperation(askMode);
    
    CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
    lps->Init(extractCallback, true);
    
    // _dataAfterEnd = false;
    // _needMoreInput = false;
    UInt64 inSize, outSize;
    SRes decodeSRes;
    bool extraSize;
    const HRESULT hres = DecodeToOutput(realOutStream, lps, inSize, outSize, decodeSRes, extraSize);

    // if (hres == E_ABORT) return hres;
    opRes = NExtract::NOperationResult::kDataError;
    
    if (hres == E_OUTOFMEMORY)
    {
      return hres;
      // opRes = NExtract::NOperationResult::kMemError;
    }
    else if (hres == S_OK || hres == S_FALSE)
    {
#ifdef Z7_USE_ZSTD_ORIG_DECODER
      if (hres == S_OK)
        opRes = NExtract::NOperationResult::kOK;
#else
      if (decodeSRes == SZ_ERROR_CRC)
      {
        opRes = NExtract::NOperationResult::kCRCError;
      }
      else if (decodeSRes == SZ_ERROR_NO_ARCHIVE)
      {
        _isArc = false;
        opRes = NExtract::NOperationResult::kIsNotArc;
      }
      else if (decodeSRes == SZ_ERROR_INPUT_EOF)
        opRes = NExtract::NOperationResult::kUnexpectedEnd;
      else
      {
        if (hres == S_OK && decodeSRes == SZ_OK)
          opRes = NExtract::NOperationResult::kOK;
        if (extraSize)
        {
          // if (inSize == 0) _isArc = false;
          opRes = NExtract::NOperationResult::kDataAfterEnd;
        }
      }
#endif
    }
    else if (hres == E_NOTIMPL)
    {
      opRes = NExtract::NOperationResult::kUnsupportedMethod;
    }
    else
      return hres;
  }

  return extractCallback->SetOperationResult(opRes);

  COM_TRY_END
}



Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps))
{
  // _smallMode = false;
  _disableHash = false;
  _parseMode = false;
  _tarPosixMode = false;
  _level = kZstdDefaultLevel;
  // _parseMode = true; // for debug

  for (UInt32 i = 0; i < numProps; i++)
  {
    UString name = names[i];
    const PROPVARIANT &value = values[i];
    
    if (name.IsEqualTo("parse"))
    {
      bool parseMode = true;
      RINOK(PROPVARIANT_to_bool(value, parseMode))
        _parseMode = parseMode;
      continue;
    }
    if (name.IsPrefixedBy_Ascii_NoCase("crc"))
    {
      name.Delete(0, 3);
      UInt32 crcSize = 4;
      RINOK(ParsePropToUInt32(name, value, crcSize))
      if (crcSize == 0)
        _disableHash = true;
      else if (crcSize == 4)
        _disableHash = false;
      else
        return E_INVALIDARG;
      continue;
    }
    if (name.IsPrefixedBy_Ascii_NoCase("zx")
        || name.IsPrefixedBy_Ascii_NoCase("x"))
    {
      name.Delete(0, name[0] == 'z' || name[0] == 'Z' ? 2 : 1);
      UInt32 level = (UInt32)kZstdDefaultLevel;
      RINOK(ParsePropToUInt32(name, value, level))
      if (level > (UInt32)ZSTD_maxCLevel())
        return E_INVALIDARG;
      _level = (int)level;
      continue;
    }
    if (name.IsPrefixedBy_Ascii_NoCase("zf")
        || name.IsPrefixedBy_Ascii_NoCase("f"))
    {
      name.Delete(0, name[0] == 'z' || name[0] == 'Z' ? 2 : 1);
      UInt32 level = 1;
      RINOK(ParsePropToUInt32(name, value, level))
      const int minLevel = ZSTD_minCLevel();
      const UInt32 maxFastLevel = minLevel < 0 ? (UInt32)(-minLevel) : 0;
      if (level == 0 || level > maxFastLevel)
        return E_INVALIDARG;
      _level = -(int)level;
      continue;
    }
    if (name.IsEqualTo_Ascii_NoCase("m"))
    {
      if (value.vt != VT_BSTR || value.bstrVal == NULL)
        return E_INVALIDARG;
      const UString method = value.bstrVal;
      if (method.IsEqualTo_Ascii_NoCase("pax") ||
          method.IsEqualTo_Ascii_NoCase("posix"))
        _tarPosixMode = true;
      else if (method.IsEqualTo_Ascii_NoCase("gnu") ||
          method.IsEqualTo_Ascii_NoCase("zstd") ||
          method.IsEqualTo_Ascii_NoCase("zstandard"))
        _tarPosixMode = false;
      else
        return E_INVALIDARG;
      continue;
    }
  }
  return S_OK;
}




#ifndef Z7_EXTRACT_ONLY

Z7_CLASS_IMP_COM_1(CZstdEncodeOutStream, ISequentialOutStream)
  CMyComPtr<ISequentialOutStream> _stream;
  ZSTD_CCtx *_ctx;
  CByteBuffer _outBuf;
  bool _finished;

public:
  CZstdEncodeOutStream():
      _ctx(NULL),
      _finished(false)
      {}
  ~CZstdEncodeOutStream()
  {
    if (_ctx)
      ZSTD_freeCCtx(_ctx);
  }

  HRESULT Init(ISequentialOutStream *stream, int level);
  HRESULT Finish();
};

HRESULT CZstdEncodeOutStream::Init(ISequentialOutStream *stream, int level)
{
  _stream = stream;
  _finished = false;
  if (!stream)
    return E_FAIL;
  if (!_ctx)
  {
    _ctx = ZSTD_createCCtx();
    if (!_ctx)
      return E_OUTOFMEMORY;
  }
  size_t res = ZSTD_CCtx_reset(_ctx, ZSTD_reset_session_and_parameters);
  if (ZSTD_isError(res))
    return E_FAIL;
  res = ZSTD_CCtx_setParameter(_ctx, ZSTD_c_compressionLevel, (int)level);
  if (ZSTD_isError(res))
    return E_INVALIDARG;
  _outBuf.Alloc(ZSTD_CStreamOutSize());
  return S_OK;
}

Z7_COM7F_IMF(CZstdEncodeOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  COM_TRY_BEGIN

  if (processedSize)
    *processedSize = 0;
  if (_finished || !_ctx || !_stream)
    return E_FAIL;

  ZSTD_inBuffer input = { data, size, 0 };
  while (input.pos < input.size)
  {
    ZSTD_outBuffer output = { _outBuf, _outBuf.Size(), 0 };
    const size_t res = ZSTD_compressStream2(_ctx, &output, &input, ZSTD_e_continue);
    if (ZSTD_isError(res))
      return E_FAIL;
    if (output.pos != 0)
      RINOK(WriteStream(_stream, _outBuf, output.pos))
  }

  if (processedSize)
    *processedSize = size;
  return S_OK;

  COM_TRY_END
}

HRESULT CZstdEncodeOutStream::Finish()
{
  if (_finished)
    return S_OK;
  if (!_ctx || !_stream)
    return E_FAIL;

  ZSTD_inBuffer input = { NULL, 0, 0 };
  for (;;)
  {
    ZSTD_outBuffer output = { _outBuf, _outBuf.Size(), 0 };
    const size_t res = ZSTD_compressStream2(_ctx, &output, &input, ZSTD_e_end);
    if (ZSTD_isError(res))
      return E_FAIL;
    if (output.pos != 0)
      RINOK(WriteStream(_stream, _outBuf, output.pos))
    if (res == 0)
      break;
  }
  _finished = true;
  return S_OK;
}

static bool IsTarZstdUpdate(IArchiveUpdateCallback *updateCallback)
{
  CMyComPtr<IArchiveGetRootProps> rootProps;
  updateCallback->QueryInterface(IID_IArchiveGetRootProps, (void **)&rootProps);
  if (!rootProps)
    return false;

  NCOM::CPropVariant prop;
  if (rootProps->GetRootProp(kpidArcFileName, &prop) != S_OK)
    return false;
  if (prop.vt != VT_BSTR || prop.bstrVal == NULL)
    return false;
  return IsTarZstdName(prop.bstrVal);
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

  if (IsTarZstdUpdate(updateCallback))
  {
    CMyComPtr2_Create<ISequentialOutStream, CZstdEncodeOutStream> zstdStream;
    RINOK(zstdStream->Init(outStream, _level))

    CMyComPtr2_Create<IOutArchive, NTar::CHandler> tarHandler;
    const wchar_t *name = L"m";
    NCOM::CPropVariant value(_tarPosixMode ? L"posix" : L"gnu");
    CMyComPtr<ISetProperties> setProperties;
    tarHandler.Interface()->QueryInterface(IID_ISetProperties, (void **)&setProperties);
    if (!setProperties)
      return E_FAIL;
    RINOK(setProperties->SetProperties(&name, &value, 1))
    RINOK(tarHandler.Interface()->UpdateItems(zstdStream.Interface(), numItems, updateCallback))
    return zstdStream->Finish();
  }

  if (numItems != 1)
    return E_INVALIDARG;

  {
    CMyComPtr<IStreamSetRestriction> setRestriction;
    outStream->QueryInterface(IID_IStreamSetRestriction, (void **)&setRestriction);
    if (setRestriction)
      RINOK(setRestriction->SetRestriction(0, 0))
  }
  Int32 newData, newProps;
  UInt32 indexInArchive;
  RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProps, &indexInArchive))
 
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

    {
      CMyComPtr<ISequentialInStream> fileInStream;
      RINOK(updateCallback->GetStream(0, &fileInStream))
      if (!fileInStream)
        return S_FALSE;
      {
        CMyComPtr<IStreamGetSize> streamGetSize;
        fileInStream.QueryInterface(IID_IStreamGetSize, &streamGetSize);
        if (streamGetSize)
        {
          UInt64 size2;
          if (streamGetSize->GetSize(&size2) == S_OK)
            size = size2;
        }
      }
      RINOK(updateCallback->SetTotal(size))

      CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
      lps->Init(updateCallback, true);
      {
        CMyComPtr2_Create<ISequentialOutStream, CZstdEncodeOutStream> zstdStream;
        RINOK(zstdStream->Init(outStream, _level))
        RINOK(NCompress::CopyStream(fileInStream, zstdStream.Interface(), lps))
        RINOK(zstdStream->Finish())
      }
    }
    return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
  }

  if (indexInArchive != 0)
    return E_INVALIDARG;

  CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
  lps->Init(updateCallback, true);

  CMyComPtr<IArchiveUpdateCallbackFile> opCallback;
  updateCallback->QueryInterface(IID_IArchiveUpdateCallbackFile, (void **)&opCallback);
  if (opCallback)
  {
    RINOK(opCallback->ReportOperation(
        NEventIndexType::kInArcIndex, 0,
        NUpdateNotifyOp::kReplicate))
  }

  if (_stream)
    RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL))

  return NCompress::CopyStream(_stream, outStream, lps);

  COM_TRY_END
}
#endif



REGISTER_ARC_IO(
  "zstd", "tzs zst tzst tzstd", ".tar * .tar .tar", 0xe,
  k_Signature, 0
  , NArcInfoFlags::kKeepName
  , 0
  , NULL)

}}
