/*
 * VGMTrans (c) 2002-2024
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */
#pragma once
#include "Scanner.h"
#include <vector>

// Forward declarations
class SonyPS2Seq;
class SonyPS2InstrSet;
class PSXSampColl;

class SonyPS2Scanner : public VGMScanner {
public:
  explicit SonyPS2Scanner(Format* format) : VGMScanner(format) {}

  virtual void scan(RawFile* file, void* info = 0);
  std::vector<SonyPS2Seq*> searchForSeq(RawFile* file);
  std::vector<SonyPS2InstrSet*> searchForInstrSet(RawFile* file);

private:
  // Helper method to create collections
  void createCollections(const std::vector<SonyPS2Seq*>& sequences,
                         const std::vector<SonyPS2InstrSet*>& instrSets,
                         const std::vector<PSXSampColl*>& sampColls);
};
