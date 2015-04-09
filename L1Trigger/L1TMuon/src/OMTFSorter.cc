#include <cassert>
#include <iostream>
#include <strstream>
#include <algorithm>

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/L1GlobalMuonTrigger/interface/L1MuRegionalCand.h"

#include "L1Trigger/L1TMuon/interface/OMTFConfiguration.h"
#include "L1Trigger/L1TMuon/interface/OMTFSorter.h"

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
std::tuple<unsigned int,unsigned int, int, int, unsigned int> OMTFSorter::sortSingleResult(const OMTFResult & aResult){

  OMTFResult::vector1D pdfValsVec = aResult.getSummaryVals();
  OMTFResult::vector1D nHitsVec = aResult.getSummaryHits();
  OMTFResult::vector1D refPhiVec = aResult.getRefPhis();
  OMTFResult::vector1D hitsVec = aResult.getHitsWord();

  assert(pdfValsVec.size()==nHitsVec.size());

  unsigned int nHitsMax = 0;
  unsigned int pdfValMax = 0;
  unsigned int hitsWord = 0;
  int refPhi = 1024;
  int refLayer = -1;

  std::tuple<unsigned int,unsigned int, int, int, unsigned int>  sortedResult;
  std::get<0>(sortedResult) = nHitsMax;
  std::get<1>(sortedResult) = pdfValMax;
  std::get<2>(sortedResult) = refPhi;
  std::get<3>(sortedResult) = refLayer;
  std::get<4>(sortedResult) = hitsWord;

  ///Find a result with biggest number of hits
  for(auto itHits: nHitsVec){
    if(itHits>nHitsMax) nHitsMax = itHits;
  }

  if(!nHitsMax) return sortedResult;

  for(unsigned int ipdfVal=0;ipdfVal<pdfValsVec.size();++ipdfVal){
    if(nHitsVec[ipdfVal] == nHitsMax){
      if(pdfValsVec[ipdfVal]>pdfValMax){
	pdfValMax = pdfValsVec[ipdfVal]; 
	refPhi = refPhiVec[ipdfVal]; 
	refLayer = ipdfVal;
	hitsWord = hitsVec[ipdfVal]; 
      }
    }
  }

  std::get<0>(sortedResult) = nHitsMax;
  std::get<1>(sortedResult) = pdfValMax;
  std::get<2>(sortedResult) = refPhi;
  std::get<3>(sortedResult) = refLayer;
  std::get<4>(sortedResult) = hitsWord;
  return sortedResult;
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
InternalObj OMTFSorter::sortRefHitResults(const OMTFProcessor::resultsMap & aResultsMap,
					  int charge){

  unsigned int pdfValMax = 0;
  unsigned int nHitsMax = 0;  
  unsigned int hitsWord = 0;
  int refPhi = 9999;
  int refLayer = -1;
  Key bestKey;
  for(auto itKey: aResultsMap){   
    if(charge!=0 && itKey.first.theCharge!=charge) continue; //charge==0 means ignore charge
    std::tuple<unsigned int,unsigned int, int, int, unsigned int > val = sortSingleResult(itKey.second);
    ///Accept only candidates with >2 hits
    if(std::get<0>(val)<3) continue;
    ///
    if( std::get<0>(val)>nHitsMax){
      nHitsMax = std::get<0>(val);
      pdfValMax = std::get<1>(val);
      refPhi = std::get<2>(val);
      refLayer = std::get<3>(val);
      hitsWord = std::get<4>(val);
      bestKey = itKey.first;
    }
    else if(std::get<0>(val)==nHitsMax && std::get<1>(val)>pdfValMax){
      pdfValMax = std::get<1>(val);
      refPhi = std::get<2>(val);
      refLayer = std::get<3>(val);
      hitsWord = std::get<4>(val);
      bestKey = itKey.first;
    }
    else if(std::get<0>(val)==nHitsMax && std::get<1>(val)==pdfValMax &&
	    itKey.first.thePtCode<bestKey.thePtCode){
      pdfValMax = std::get<1>(val);
      refPhi = std::get<2>(val);
      refLayer = std::get<3>(val);
      hitsWord = std::get<4>(val);
      bestKey = itKey.first;
    }
  }  

  InternalObj candidate(bestKey.thePtCode, bestKey.theEtaCode, refPhi,
			pdfValMax, 0, nHitsMax,
			bestKey.theCharge, refLayer);

  candidate.hits   = hitsWord;

  /////TEST AVERAGE PT///////
  /*
  pdfValMax = 0;
  for(auto itKey: aResultsMap){
    if(itKey.first.thePtCode>candidate.pt) continue;
    std::tuple<unsigned int,unsigned int, int, int > val = sortSingleResult(itKey.second);
    if(std::get<0>(val)==nHitsMax && std::get<1>(val)!=candidate.disc &&  std::get<1>(val)>pdfValMax){
      pdfValMax = std::get<1>(val);
      refPhi = std::get<2>(val);
      refLayer = std::get<3>(val);
      bestKey = itKey.first;
    }
  }
  InternalObj candidatePrevious;
  candidatePrevious.pt =  bestKey.thePtCode;
  candidatePrevious.eta = bestKey.theEtaCode; 
  candidatePrevious.phi = refPhi;
  candidatePrevious.charge = bestKey.theCharge;
  candidatePrevious.q   = nHitsMax;
  candidatePrevious.disc = pdfValMax;
  candidatePrevious.refLayer = refLayer;

  pdfValMax = 0;
  for(auto itKey: aResultsMap){   
    if(itKey.first.thePtCode<candidate.pt) continue;
    std::tuple<unsigned int,unsigned int, int, int > val = sortSingleResult(itKey.second);
    if(std::get<0>(val)==nHitsMax && std::get<1>(val)!=candidate.disc && std::get<1>(val)>pdfValMax){
      pdfValMax = std::get<1>(val);
      refPhi = std::get<2>(val);
      refLayer = std::get<3>(val);
      bestKey = itKey.first;
    }
  }
  InternalObj candidateNext;
  candidateNext.pt =  bestKey.thePtCode;
  candidateNext.eta = bestKey.theEtaCode; 
  candidateNext.phi = refPhi;
  candidateNext.charge = bestKey.theCharge;
  candidateNext.q   = nHitsMax;
  candidateNext.disc = pdfValMax;
  candidateNext.refLayer = refLayer;

  if(candidate.pt){
    float weightedPtCode = candidatePrevious.pt*candidatePrevious.disc + 
                           candidate.pt*candidate.disc + 
                           candidateNext.pt*candidateNext.disc;
    weightedPtCode/= candidatePrevious.disc + candidate.disc + candidateNext.disc;
    candidate.pt = (int)weightedPtCode;
  } 
*/
  ////////////////////////////// 

  return candidate;

}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
InternalObj OMTFSorter::sortProcessorResults(const std::vector<OMTFProcessor::resultsMap> & procResults,
					     int charge){ //method kept for backward compatibility

  std::vector<InternalObj> sortedCandidates;
  sortProcessorResults(procResults, sortedCandidates, charge);

  InternalObj candidate = sortedCandidates.size()>0 ? sortedCandidates[0] : InternalObj(0,99,9999,0,0,0,0,-1);

  std::ostringstream myStr;
  myStr<<"Selected Candidate with charge: "<<charge<<" "<<candidate<<std::endl;
  edm::LogInfo("OMTF Sorter")<<myStr.str();

  return candidate;

}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
void OMTFSorter::sortProcessorResults(const std::vector<OMTFProcessor::resultsMap> & procResults,
				      std::vector<InternalObj> & refHitCleanCands,
				      int charge){

  refHitCleanCands.clear();
  std::vector<InternalObj> refHitCands;

  for(auto itRefHit: procResults) refHitCands.push_back(sortRefHitResults(itRefHit,charge));

  // Sort candidates with decreased goodness,
  // where goodness definied in < operator of InternalObj
  std::sort( refHitCands.begin(), refHitCands.end() );

  // Clean candidate list by removing dupicates bazing on Phi distance. 
  // Assumed that the list is ordered
  for(std::vector<InternalObj>::iterator it1 = refHitCands.begin();
      it1 != refHitCands.end(); ++it1){
    bool isGhost=false;
    for(std::vector<InternalObj>::iterator it2 = refHitCleanCands.begin();
	it2 != refHitCleanCands.end(); ++it2){
      //do not accept candidates with similar phi and same charge 
      if(std::abs(it1->phi - it2->phi)<5/360.0*OMTFConfiguration::nPhiBins //veto window 5deg(=half of logic cone)=5/360*4096=57"logic strips"
	 && it1->charge==it2->charge){
	isGhost=true;
	break;
      }
    }
    if(it1->q>0 && !isGhost) refHitCleanCands.push_back(*it1);
  }
  //return 3 candidates (adding empty ones if needed)
  refHitCleanCands.resize( 3, InternalObj(0,99,9999,0,0,0,0,-1) );

  std::ostringstream myStr;
  bool hasCandidates = false;
  for(unsigned int iRefHit=0;iRefHit<refHitCands.size();++iRefHit){
    if(refHitCands[iRefHit].q){
      hasCandidates=true;
      break;
    }  
  }    
  for(unsigned int iRefHit=0;iRefHit<refHitCands.size();++iRefHit){
    if(refHitCands[iRefHit].q) myStr<<"Ref hit: "<<iRefHit<<" "<<refHitCands[iRefHit]<<std::endl;
  }
  myStr<<"Selected Candidates with charge: "<<charge<<std::endl;
  for(unsigned int iCand=0; iCand<refHitCleanCands.size(); ++iCand){
    myStr<<"Cand: "<<iCand<<" "<<refHitCleanCands[iCand]<<std::endl;
  }

  if(hasCandidates) edm::LogInfo("OMTF Sorter")<<myStr.str();


  return;
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
L1MuRegionalCand OMTFSorter::sortProcessor(const std::vector<OMTFProcessor::resultsMap> & procResults,
					   int charge){ //method kept for backward compatibility

  InternalObj myCand = sortProcessorResults(procResults, charge);

  L1MuRegionalCand candidate;
  candidate.setPhiValue(myCand.phi);
  candidate.setPtPacked(myCand.pt);
  //candidate.setQualityPacked(3);//FIX ME
  //candidate.setBx(1000*myCand.disc+100*myCand.refLayer+myCand.q);//FIX ME

  candidate.setBx(1E6*myCand.disc + myCand.hits);//FIX ME
  //candidate.setEtaPacked(10*myCand.refLayer+myCand.q);
  candidate.setEtaPacked(myCand.q);

  candidate.setChargeValue(myCand.charge);

  return candidate;
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
void OMTFSorter::sortProcessor(const std::vector<OMTFProcessor::resultsMap> & procResults,
			       std::vector<L1MuRegionalCand> & sortedCands,
			       int charge){

  sortedCands.clear();
  std::vector<InternalObj> mySortedCands;
  sortProcessorResults(procResults, mySortedCands, charge);

  for(auto myCand: mySortedCands){
    L1MuRegionalCand candidate;
    candidate.setPhiValue(myCand.phi);
    candidate.setPtPacked(myCand.pt);
    //candidate.setQualityPacked(3);//FIX ME
    //candidate.setBx(1000*myCand.disc+100*myCand.refLayer+myCand.q);//FIX ME

    candidate.setBx(100*myCand.hits+myCand.q);//FIX ME

    candidate.setChargeValue(myCand.charge);
    sortedCands.push_back(candidate);
  }

  return;
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

