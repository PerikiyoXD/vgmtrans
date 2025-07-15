/*
 * VGMTrans (c) 2002-2024
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */

#include "SonyPS2Seq.h"
#include "SonyPS2InstrSet.h"
#include "ScannerManager.h"
#include "VGMColl.h"
#include "PSXSPU.h"

namespace vgmtrans::scanners {
ScannerRegistration<SonyPS2Scanner> s_sonyps2("SonyPS2", {"sq", "hd", "bd", "vag"});
}

void SonyPS2Scanner::scan(RawFile* file, void* info) {
  auto sequences = searchForSeq(file);
  auto instrSets = searchForInstrSet(file);
  auto sampColls = PSXSampColl::searchForPSXADPCMs(file, "SonyPS2");

  // Create collections that associate related components
  createCollections(sequences, instrSets, sampColls);
}

std::vector<SonyPS2Seq*> SonyPS2Scanner::searchForSeq(RawFile* file) {
  std::vector<SonyPS2Seq*> loadedFiles;

  size_t nFileLength = file->size();
  for (uint32_t i = 0; i + 0x40 < nFileLength; i++) {
    uint32_t sig1 = file->readWord(i);
    uint32_t sig2 = file->readWord(i + 4);
    if (sig1 != 0x53434549 || sig2 != 0x56657273)  // "SCEIVers" in ASCII
      continue;

    sig1 = file->readWord(i + 0x10);
    sig2 = file->readWord(i + 0x14);
    if (sig1 != 0x53434549 || sig2 != 0x53657175)  // "SCEISequ" in ASCII
      continue;

    sig1 = file->readWord(i + 0x30);
    sig2 = file->readWord(i + 0x34);
    if (sig1 != 0x53434549 || sig2 != 0x4D696469)  // "SCEIMidi" in ASCII
      continue;

    SonyPS2Seq* newSeq = new SonyPS2Seq(file, i, "Sony PS2 Seq");
    if (newSeq->loadVGMFile()) {
      loadedFiles.push_back(newSeq);
    } else {
      delete newSeq;
    }
  }

  return loadedFiles;
}

std::vector<SonyPS2InstrSet*> SonyPS2Scanner::searchForInstrSet(RawFile* file) {
  std::vector<SonyPS2InstrSet*> loadedFiles;

  size_t nFileLength = file->size();
  for (uint32_t i = 0; i + 0x40 < nFileLength; i++) {
    uint32_t sig1 = file->readWord(i);
    uint32_t sig2 = file->readWord(i + 4);
    if (sig1 != 0x53434549 || sig2 != 0x56657273)  // "SCEIVers" in ASCII
      continue;

    sig1 = file->readWord(i + 0x10);
    sig2 = file->readWord(i + 0x14);
    if (sig1 != 0x53434549 || sig2 != 0x48656164)  // "SCEIHead" in ASCII
      continue;

    sig1 = file->readWord(i + 0x50);
    sig2 = file->readWord(i + 0x54);
    if (sig1 != 0x53434549 || sig2 != 0x56616769)  // "SCEIVagi" in ASCII
      continue;

    SonyPS2InstrSet* newInstrSet = new SonyPS2InstrSet(file, i);
    if (newInstrSet->loadVGMFile()) {
      loadedFiles.push_back(newInstrSet);
    } else {
      delete newInstrSet;
    }
  }

  return loadedFiles;
}

void SonyPS2Scanner::createCollections(const std::vector<SonyPS2Seq*>& sequences,
                                       const std::vector<SonyPS2InstrSet*>& instrSets,
                                       const std::vector<PSXSampColl*>& sampColls) {
  // For PS2 VAGs, we create a collection for each sequence, associating it with
  // ALL instrument sets and sample collections from the same file
  for (SonyPS2Seq* seq : sequences) {
    if (!seq)
      continue;

    // Create collection with sequence name
    VGMColl* coll = new VGMColl(seq->name());
    coll->useSeq(seq);

    // Add ALL instrument sets from the same file
    bool foundInstrSets = false;
    for (SonyPS2InstrSet* iset : instrSets) {
      if (iset && iset->rawFile() == seq->rawFile()) {
        coll->addInstrSet(iset);
        foundInstrSets = true;
      }
    }

    // If no instrument sets from same file, add all available ones
    if (!foundInstrSets && !instrSets.empty()) {
      for (SonyPS2InstrSet* iset : instrSets) {
        if (iset) {
          coll->addInstrSet(iset);
        }
      }
    }

    // Add ALL sample collections from the same file
    bool foundSampColls = false;
    for (PSXSampColl* samp : sampColls) {
      if (samp && samp->rawFile() == seq->rawFile()) {
        coll->addSampColl(samp);
        foundSampColls = true;
      }
    }

    // If no sample collections from same file, add all available ones
    if (!foundSampColls && !sampColls.empty()) {
      for (PSXSampColl* samp : sampColls) {
        if (samp) {
          coll->addSampColl(samp);
        }
      }
    }

    // Load the collection
    if (!coll->load()) {
      delete coll;
    }
  }

  // Create collections for orphaned instrument sets (no associated sequence)
  for (SonyPS2InstrSet* instrSet : instrSets) {
    if (!instrSet)
      continue;

    // Check if this instrument set is already used in a collection with a sequence
    bool isUsed = false;
    for (SonyPS2Seq* seq : sequences) {
      if (seq && seq->rawFile() == instrSet->rawFile()) {
        isUsed = true;
        break;
      }
    }

    if (!isUsed) {
      VGMColl* coll = new VGMColl(instrSet->name());
      coll->addInstrSet(instrSet);

      // Add ALL other instrument sets from the same file
      for (SonyPS2InstrSet* otherIset : instrSets) {
        if (otherIset && otherIset != instrSet && otherIset->rawFile() == instrSet->rawFile()) {
          coll->addInstrSet(otherIset);
        }
      }

      // Add ALL sample collections from the same file
      for (PSXSampColl* samp : sampColls) {
        if (samp && samp->rawFile() == instrSet->rawFile()) {
          coll->addSampColl(samp);
        }
      }

      if (!coll->load()) {
        delete coll;
      }
    }
  }
}
