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
#ifndef MDCMIMAGECODEC_H
#define MDCMIMAGECODEC_H

#include "mdcmCodec.h"
#include "mdcmPhotometricInterpretation.h"
#include "mdcmLookupTable.h"
#include "mdcmSmartPointer.h"
#include "mdcmPixelFormat.h"

namespace mdcm
{

/**
 * ImageCodec
 * Main codec, this is a central place for all implementation
 */
class MDCM_EXPORT ImageCodec : public Codec
{
  friend class ImageChangePhotometricInterpretation;
public:
  ImageCodec();
  ~ImageCodec();
  bool CanCode(TransferSyntax const &) const { return false; }
  bool CanDecode(TransferSyntax const &) const { return false; }
  bool Decode(DataElement const &is_, DataElement &os);
  bool IsLossy() const;
  void SetLossyFlag(bool l);
  bool GetLossyFlag() const;
  virtual bool GetHeaderInfo(std::istream &is_, TransferSyntax &ts);
  virtual ImageCodec * Clone() const = 0;
protected:

  bool DecodeByStreams(std::istream & is_, std::ostream & os);
  virtual bool IsValid(PhotometricInterpretation const & pi);
public:

  unsigned int GetPlanarConfiguration() const
  {
    return PlanarConfiguration;
  }

  void SetPlanarConfiguration(unsigned int pc)
  {
    assert(pc == 0 || pc == 1);
    PlanarConfiguration = pc;
  }

  PixelFormat &GetPixelFormat()
  {
    return PF;
  }

  const PixelFormat &GetPixelFormat() const
  {
    return PF;
  }

  virtual void SetPixelFormat(PixelFormat const &pf)
  {
    PF = pf;
  }

  const PhotometricInterpretation &GetPhotometricInterpretation() const;
  void SetPhotometricInterpretation(PhotometricInterpretation const &pi);

  bool GetNeedByteSwap() const
  {
    return NeedByteSwap;
  }

  void SetNeedByteSwap(bool b)
  {
    NeedByteSwap = b;
  }

  void SetNeedOverlayCleanup(bool b)
  {
    NeedOverlayCleanup = b;
  }

  void SetLUT(LookupTable const &lut)
  {
    LUT = SmartPointer<LookupTable>(const_cast<LookupTable*>(&lut));
  }

  const LookupTable &GetLUT() const
  {
    return *LUT;
  }

  void SetDimensions(const unsigned int d[3]);
  void SetDimensions(const std::vector<unsigned int> & d);
  const unsigned int *GetDimensions() const { return Dimensions; }
  void SetNumberOfDimensions(unsigned int dim);
  unsigned int GetNumberOfDimensions() const;
  bool CleanupUnusedBits(char * data, size_t datalen);

protected:
  // Streaming (write) API:
  // This is a high level API to encode in a streaming fashion. Each plugin
  // will handle differently the caching mechanism so that a limited memory is
  // used when compressing dataset.
  // Codec will fall into two categories:
  // - Full row encoder: only a single scanline (row) of data is needed to be loaded at a time;
  // - Full frame encoder (default): a complete frame (row x col) is needed to be loaded at a time
  friend class FileChangeTransferSyntax;
  virtual bool StartEncode(std::ostream & os);
  virtual bool IsRowEncoder();
  virtual bool IsFrameEncoder();
  virtual bool AppendRowEncode(std::ostream & out, const char * data, size_t datalen);
  virtual bool AppendFrameEncode(std::ostream & out, const char * data, size_t datalen);
  virtual bool StopEncode(std::ostream & os);

protected:
  bool RequestPlanarConfiguration;
  bool RequestPaddedCompositePixelCode;
  unsigned int PlanarConfiguration;
  PhotometricInterpretation PI;
  PixelFormat PF;
  bool NeedByteSwap;
  bool NeedOverlayCleanup;
  typedef SmartPointer<LookupTable> LUTPtr;
  LUTPtr LUT;
  unsigned int Dimensions[3];
  unsigned int NumberOfDimensions;
  bool LossyFlag;
  bool DoOverlayCleanup(std::istream &, std::ostream &);
  bool DoByteSwap(std::istream &, std::ostream &);
  bool DoYBRFull422(std::istream &, std::ostream &);
  bool DoPlanarConfiguration(std::istream &, std::ostream &);
  bool DoSimpleCopy(std::istream &is_, std::ostream & os);
  bool DoPaddedCompositePixelCode(std::istream &, std::ostream &);
  bool DoInvertMonochrome(std::istream &, std::ostream &);
};

} // end namespace mdcm

#endif //MDCMIMAGECODEC_H
