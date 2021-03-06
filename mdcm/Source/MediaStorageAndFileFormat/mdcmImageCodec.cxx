/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmImageCodec.h"
#include "mdcmJPEGCodec.h"
#include "mdcmByteSwap.txx"
#include "mdcmTrace.h"
#include <iostream>
#include <iomanip>
#include <iterator>
#include <cstring>
#include <limits.h>

namespace mdcm
{

class ImageInternals
{
public:
};

ImageCodec::ImageCodec()
{
  PlanarConfiguration = 0;
  RequestPlanarConfiguration = false;
  RequestPaddedCompositePixelCode = false;
  PI = PhotometricInterpretation::UNKNOWN;
  LUT = new LookupTable;
  NeedByteSwap = false;
  NeedOverlayCleanup = false;
  Dimensions[0] = Dimensions[1] = Dimensions[2] = 0;
  NumberOfDimensions = 0;
  LossyFlag = false;
}

ImageCodec::~ImageCodec()
{
}

bool ImageCodec::GetHeaderInfo(std::istream &, TransferSyntax &)
{
  assert(0);
  return false;
}

void ImageCodec::SetLossyFlag(bool l)
{
  LossyFlag = l;
}

bool ImageCodec::GetLossyFlag() const
{
  return LossyFlag;
}

bool ImageCodec::IsLossy() const
{
  return LossyFlag;
}

void ImageCodec::SetNumberOfDimensions(unsigned int dim)
{
  NumberOfDimensions = dim;
}

unsigned int ImageCodec::GetNumberOfDimensions() const
{
  return NumberOfDimensions;
}


const PhotometricInterpretation &ImageCodec::GetPhotometricInterpretation() const
{
  return PI;
}

void ImageCodec::SetPhotometricInterpretation(PhotometricInterpretation const & pi)
{
  PI = pi;
}

bool ImageCodec::DoByteSwap(std::istream & is, std::ostream & os)
{
  std::streampos start = is.tellg();
  assert(0 - start == 0);
  is.seekg(0, std::ios::end);
  size_t buf_size = (size_t)is.tellg();
  char * dummy_buffer = new char[(unsigned int)buf_size];
  is.seekg(start, std::ios::beg);
  is.read(dummy_buffer, buf_size);
  is.seekg(start, std::ios::beg);
  assert(!(buf_size % 2));
#ifdef MDCM_WORDS_BIGENDIAN
  if(PF.GetBitsAllocated() == 16)
  {
    ByteSwap<uint16_t>::SwapRangeFromSwapCodeIntoSystem((uint16_t*)
      dummy_buffer, SwapCode::LittleEndian, buf_size/2);
  }
#else
  // GE_DLX-8-MONO2-PrivateSyntax.dcm is 8bits
  if (PF.GetBitsAllocated() == 16)
  {
    ByteSwap<uint16_t>::SwapRangeFromSwapCodeIntoSystem((uint16_t*)
      dummy_buffer, SwapCode::BigEndian, buf_size/2);
  }
#endif
  os.write(dummy_buffer, buf_size);
  delete[] dummy_buffer;
  return true;
}

bool ImageCodec::DoYBRFull422(std::istream & is, std::ostream & os)
{
  std::streampos start = is.tellg();
  assert(0 - start == 0);
  is.seekg(0, std::ios::end);
  const size_t buf_size = (size_t)is.tellg();
  if (buf_size % 2 != 0) return false;
  const size_t rgb_buf_size = buf_size * 3 / 2;
  if (rgb_buf_size % 3 != 0) return false;
  unsigned char * buffer;
  try { buffer = new unsigned char[buf_size]; }
  catch (std::bad_alloc&) { return false; }
  is.seekg(start, std::ios::beg);
  is.read((char*)buffer, buf_size);
  is.seekg(start, std::ios::beg);
  unsigned char * copy;
  try { copy = new unsigned char[rgb_buf_size]; }
  catch (std::bad_alloc&) { delete [] buffer; return false; }
  const size_t size = buf_size/4;
  for (size_t j = 0; j < size; ++j)
  {
    const unsigned char ybr422[] =
    {
      buffer[4*j+0],
      buffer[4*j+1],
      buffer[4*j+2],
      buffer[4*j+3]
    };
    const unsigned char ybr[] =
    {
      ybr422[0],
      ybr422[2],
      ybr422[3],
      ybr422[1],
      ybr422[2],
      ybr422[3]
    };
    memcpy(copy + 6 * j, ybr, 6);
  }
  os.write((char*)copy, rgb_buf_size);
  delete [] copy;
  delete [] buffer;
  return true;
}

bool ImageCodec::DoPlanarConfiguration(std::istream & is, std::ostream & os)
{
  std::streampos start = is.tellg();
  assert(0 - start == 0);
  is.seekg(0, std::ios::end);
  size_t buf_size = (size_t)is.tellg();
  char * dummy_buffer = new char[(unsigned int)buf_size];
  is.seekg(start, std::ios::beg);
  is.read(dummy_buffer, buf_size);
  is.seekg(start, std::ios::beg);
  // US-RGB-8-epicard.dcm
  assert(buf_size % 3 == 0);
  unsigned long size = (unsigned long)buf_size/3;
  char * copy = new char[(unsigned int)buf_size];
  const char * r = dummy_buffer;
  const char * g = dummy_buffer + size;
  const char * b = dummy_buffer + size + size;
  char * p = copy;
  for (unsigned long j = 0; j < size; ++j)
  {
    *(p++) = *(r++);
    *(p++) = *(g++);
    *(p++) = *(b++);
  }
  delete[] dummy_buffer;
  os.write(copy, buf_size);
  delete[] copy;
  return true;
}

bool ImageCodec::DoSimpleCopy(std::istream & is, std::ostream & os)
{
#if 1
  std::streampos start = is.tellg();
  assert(0 - start == 0);
  is.seekg(0, std::ios::end);
  size_t buf_size = (size_t)is.tellg();
  char * dummy_buffer = new char[(unsigned int)buf_size];
  is.seekg(start, std::ios::beg);
  is.read(dummy_buffer, buf_size);
  is.seekg(start, std::ios::beg);
  os.write(dummy_buffer, buf_size);
  delete[] dummy_buffer ;
#else
  // This code is ideal but is failing on an RLE image, need to figure out
  // what is wrong to reactivate this code.
  os.rdbuf(is.rdbuf());
#endif
  return true;
}

bool ImageCodec::DoPaddedCompositePixelCode(std::istream & is, std::ostream & os)
{
  std::streampos start = is.tellg();
  assert(0 - start == 0);
  is.seekg(0, std::ios::end);
  size_t buf_size = (size_t)is.tellg();
  char * dummy_buffer = new char[(unsigned int)buf_size];
  is.seekg(start, std::ios::beg);
  is.read(dummy_buffer, buf_size);
  is.seekg(start, std::ios::beg);
  assert(!(buf_size % 2));
  bool ret = true;
  if(GetPixelFormat().GetBitsAllocated() == 16)
  {
    for(size_t i = 0; i < buf_size/2; ++i)
    {
#ifdef MDCM_WORDS_BIGENDIAN
      os.write(dummy_buffer+i, 1);
      os.write(dummy_buffer+i+buf_size/2, 1);
#else
      os.write(dummy_buffer+i+buf_size/2, 1);
      os.write(dummy_buffer+i, 1);
#endif
    }
  }
  else if(GetPixelFormat().GetBitsAllocated() == 32)
  {
    assert(!(buf_size % 4));
    for(size_t i = 0; i < buf_size/4; ++i)
    {
#ifdef MDCM_WORDS_BIGENDIAN
      os.write(dummy_buffer+i, 1);
      os.write(dummy_buffer+i+1*buf_size/4, 1);
      os.write(dummy_buffer+i+2*buf_size/4, 1);
      os.write(dummy_buffer+i+3*buf_size/4, 1);
#else
      os.write(dummy_buffer+i+3*buf_size/4, 1);
      os.write(dummy_buffer+i+2*buf_size/4, 1);
      os.write(dummy_buffer+i+1*buf_size/4, 1);
      os.write(dummy_buffer+i, 1);
#endif
    }
  }
  else
  {
    ret = false;
  }
  delete[] dummy_buffer;
  return ret;
}

bool ImageCodec::DoInvertMonochrome(std::istream & is, std::ostream & os)
{
  if (PF.GetPixelRepresentation())
  {
    if (PF.GetBitsAllocated() == 8)
    {
      uint8_t c;
      while(is.read((char*)&c,1))
      {
        c = (uint8_t)(255 - c);
        os.write((char*)&c, 1);
      }
    }
    else if (PF.GetBitsAllocated() == 16)
    {
      assert(PF.GetBitsStored() != 12);
      uint16_t smask16 = 65535;
      uint16_t c;
      while(is.read((char*)&c,2))
      {
        c = (uint16_t)(smask16 - c);
        os.write((char*)&c, 2);
      }
    }
  }
  else
  {
    if (PF.GetBitsAllocated() == 8)
    {
      uint8_t c;
      while(is.read((char*)&c,1))
      {
        c = (uint8_t)(255 - c);
        os.write((char*)&c, 1);
      }
    }
    else if (PF.GetBitsAllocated() == 16)
    {
      uint16_t mask = 1;
      for (int j=0; j<PF.GetBitsStored()-1; ++j)
      {
        mask = (uint16_t)((mask << 1) + 1); // will be 0x0fff when BitsStored = 12
      }
      uint16_t c;
      while(is.read((char*)&c,2))
      {
        if(c > mask)
        {
          // IMAGES/JPLY/RG3_JPLY aka D_CLUNIE_RG3_JPLY.dcm
          // stores a 12bits JPEG stream with scalar value [0,1024], however
          // the DICOM header says the data are stored on 10bits [0,1023], thus this HACK:
          mdcmWarningMacro("Bogus max value: "<< c << " max should be at most: " << mask
            << " results will be truncated. Use at own risk");
          c = mask;
        }
        assert(c <= mask);
        c = (uint16_t)(mask - c);
        assert(c <= mask);
        os.write((char*)&c, 2);
      }
    }
  }
  return true;
}

bool ImageCodec::CleanupUnusedBits(char * data, size_t datalen)
{
  if(!NeedOverlayCleanup) return true;
  if(PF.GetBitsAllocated() == 16)
  {
    const uint16_t d = PF.GetBitsAllocated() - PF.GetBitsStored();
    const uint16_t t = PF.GetBitsStored() - PF.GetHighBit() - 1;
    // mask unused bits
    const uint16_t pmask = (uint16_t)(0xffff >> d);
    if(PF.GetPixelRepresentation() == 1)
    {
      // check sign
      const uint16_t smask = (uint16_t)(0x0001 << (16 - (d + 1)));
      // propagate sign bit on negative values
      const int16_t nmask = (int16_t)(0x8000 >> (d - 1));
      uint16_t * start = (uint16_t*)data;
      for(uint16_t * p = start; p != start + datalen / 2; ++p)
      {
        uint16_t c = *p;
        c = c >> t;
        if (c & smask)
        {
          c = (uint16_t)(c | nmask);
        }
        else
        {
          c = c & pmask;
        }
        *p = c;
      }
    }
    else // unsigned
    {
      uint16_t * start = (uint16_t*)data;
      for(uint16_t * p = start; p != start + datalen / 2; ++p)
      {
        uint16_t c = *p;
        c = (uint16_t)((c >> t) & pmask);
        *p = c;
      }
    }
  }
  else
  {
    assert(0); // TODO
    return false;
  }
  return true;
}

bool ImageCodec::DoOverlayCleanup(std::istream &is, std::ostream &os)
{
  if(PF.GetBitsAllocated() == 16)
  {
    const uint16_t d = PF.GetBitsAllocated() - PF.GetBitsStored();
    const uint16_t t = PF.GetBitsStored() - PF.GetHighBit() - 1;
    // mask unused bits
    const uint16_t pmask = (uint16_t)(0xffff >> d);
    const size_t s16 = sizeof(uint16_t);
    const size_t bsize = 1000;
    if(PF.GetPixelRepresentation() == 1)
    {
      // check sign
      const uint16_t smask = (uint16_t)(0x0001 << (16 - (d + 1)));
      // propagate sign bit on negative values
      const int16_t nmask = (int16_t)(0x8000 >> (d - 1));
      std::vector<uint16_t> b(bsize);
      while(is)
      {
        is.read((char*)&b[0], bsize * s16);
        std::streamsize read = is.gcount();
        std::vector<uint16_t>::iterator bend = b.begin() + read / s16;
        for (std::vector<uint16_t>::iterator it = b.begin(); it != bend; ++it)
        {
          uint16_t c = (*it) >> t;
          if (c & smask)
          {
            c = (uint16_t)(c | nmask);
          }
          else
          {
            c = c & pmask;
          }
          *it = c;
        }
        os.write((char*)&b[0], read);
      }
    }
    else // unsigned
    {
      std::vector<uint16_t> b(bsize);
      while(is)
      {
        is.read((char*)&b[0], bsize * s16);
        std::streamsize rs = is.gcount();
        std::vector<uint16_t>::iterator bend = b.begin() + rs / s16;
        for (std::vector<uint16_t>::iterator it = b.begin(); it != bend; ++it)
        {
          const uint16_t c = ((*it) >> t) & pmask;
          *it = c;
        }
        os.write((char*)&b[0], rs);
      }
    }
  }
  else
  {
    assert(0); // TODO
    return false;
  }
  return true;
}

bool ImageCodec::Decode(DataElement const &, DataElement &)
{
  return true;
}

bool ImageCodec::DecodeByStreams(std::istream &is, std::ostream &os)
{
  assert(PlanarConfiguration == 0 || PlanarConfiguration == 1);
  assert(PI != PhotometricInterpretation::UNKNOWN);
  std::stringstream bs_os;   // ByteSwap
  std::stringstream pcpc_os; // Padded Composite Pixel Code
  std::stringstream pi_os;   // PhotometricInterpretation
  std::stringstream pl_os;   // PlanarConf
  std::istream * cur_is = &is;
  // Byte swap
  if(NeedByteSwap)
  {
    // MR_GE_with_Private_Compressed_Icon_0009_1110.dcm
    DoByteSwap(*cur_is,bs_os);
    cur_is = &bs_os;
  }
  if (RequestPaddedCompositePixelCode)
  {
    // D_CLUNIE_CT2_RLE.dcm
    DoPaddedCompositePixelCode(*cur_is,pcpc_os);
    cur_is = &pcpc_os;
  }
  switch(PI)
  {
  case PhotometricInterpretation::MONOCHROME2:
  case PhotometricInterpretation::RGB:
  case PhotometricInterpretation::ARGB:
  case PhotometricInterpretation::YBR_ICT:
  case PhotometricInterpretation::YBR_RCT:
  case PhotometricInterpretation::MONOCHROME1:
  case PhotometricInterpretation::PALETTE_COLOR:
  case PhotometricInterpretation::YBR_FULL:
    break;
  case PhotometricInterpretation::YBR_FULL_422:
    {
      const JPEGCodec * c = dynamic_cast<const JPEGCodec*>(this);
      if(!c)
      {
        // try raw/YBR_FULL_422
        if (DoYBRFull422(*cur_is,pl_os)) cur_is = &pl_os;
        else return false;
      }
    }
    break;
  case PhotometricInterpretation::YBR_PARTIAL_422: // retired
  case PhotometricInterpretation::YBR_PARTIAL_420: // not supported
    { // try JPEG
      const JPEGCodec * c = dynamic_cast<const JPEGCodec*>(this);
      if(!c) return false;
    }
    break;
  default:
    mdcmErrorMacro("Unhandled PhotometricInterpretation: " << PI);
    return false;
  }
  if(RequestPlanarConfiguration)
  {
    DoPlanarConfiguration(*cur_is,pl_os);
    cur_is = &pl_os;
  }
  // Overlay cleanup or cleanup the unused bits
  if (PF.GetBitsAllocated() != PF.GetBitsStored() && PF.GetBitsAllocated() != 8)
  {
    if(NeedOverlayCleanup)
    {
      DoOverlayCleanup(*cur_is,os);
    }
    else
    {
      DoSimpleCopy(*cur_is,os);
    }
  }
  else
  {
    assert(PF.GetBitsAllocated() == PF.GetBitsStored());
    DoSimpleCopy(*cur_is,os);
  }
  return true;
}

bool ImageCodec::IsValid(PhotometricInterpretation const &)
{
  return false;
}

void ImageCodec::SetDimensions(const unsigned int d[3])
{
  Dimensions[0] = d[0];
  Dimensions[1] = d[1];
  Dimensions[2] = d[2];
}

void ImageCodec::SetDimensions(const std::vector<unsigned int> & d)
{
  size_t s = d.size();
  assert(s <= 3);
  for (size_t i = 0; i < 3; i++)
  {
    if (i < s) Dimensions[i] = d[i];
    else Dimensions[i] = 1;
  }
}

bool ImageCodec::StartEncode(std::ostream &)
{
  assert(0);
  return false;
}

bool ImageCodec::IsRowEncoder()
{
  return false;
}

bool ImageCodec::IsFrameEncoder()
{
  return false;
}

bool ImageCodec::AppendRowEncode(std::ostream &, const char *, size_t)
{
  assert(0);
  return false;
}

bool ImageCodec::AppendFrameEncode(std::ostream &, const char *, size_t)
{
  assert(0);
  return false;
}

bool ImageCodec::StopEncode(std::ostream &)
{
  assert(0);
  return false;
}

} // end namespace mdcm
