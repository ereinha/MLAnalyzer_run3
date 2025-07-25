#include "MLAnalyzer_run3/RecHitAnalyzer/interface/RecHitAnalyzer.h"
#include <algorithm>   // std::sort
#include <tuple>

static constexpr unsigned int kMaxTripletHits = 8192;   // keep at most 5k
static constexpr float        kPadValue       = -999.f; // pad sentinel

template<std::size_t NL, std::size_t NP> static void zeroContainer(std::vector<float> (&arr)[NL][NP])
{
  for (std::size_t l = 0; l < NL; ++l)
    for (std::size_t p = 0; p < NP; ++p)
      arr[l][p].assign(3 * kMaxTripletHits, kPadValue);
}


extern std::vector<float> vBPIX_TRKTriplets_[nBPIX][Nhitproj];
extern std::vector<float> vFPIX_TRKTriplets_[nFPIX][Nhitproj];
extern std::vector<float> vTIB_TRKTriplets_[ nTIB ][Nhitproj];
extern std::vector<float> vTID_TRKTriplets_[ nTID ][Nhitproj];
extern std::vector<float> vTOB_TRKTriplets_[ nTOB ][Nhitproj];
extern std::vector<float> vTEC_TRKTriplets_[ nTEC ][Nhitproj];


#define DEFINE_TRIPLET_CONTAINER(name, NLAYER)         \
  std::vector<float> name[NLAYER][Nhitproj];

DEFINE_TRIPLET_CONTAINER(vBPIX_TRKTriplets_, nBPIX)
DEFINE_TRIPLET_CONTAINER(vFPIX_TRKTriplets_, nFPIX)
DEFINE_TRIPLET_CONTAINER(vTIB_TRKTriplets_,  nTIB )
DEFINE_TRIPLET_CONTAINER(vTID_TRKTriplets_,  nTID )
DEFINE_TRIPLET_CONTAINER(vTOB_TRKTriplets_,  nTOB )
DEFINE_TRIPLET_CONTAINER(vTEC_TRKTriplets_,  nTEC )
#undef DEFINE_TRIPLET_CONTAINER

namespace {
  struct HitTriple {
    float value;
    float eta;
    float phi;
    bool operator<(const HitTriple& other) const { return value > other.value; }
  };
}

void
RecHitAnalyzer::branchesTRKTriplets(TTree *tree,
                                          edm::Service<TFileService>&)
{
  char hname[64];
  for (unsigned int proj = 0; proj < Nhitproj; ++proj) {

#   define BOOK(detStr, ARR, NLAYER)                                   \
      for (int iL = 0; iL < NLAYER; ++iL) {                            \
        std::snprintf(hname, sizeof(hname),                            \
                      detStr "_layer%d_triplets%s", iL+1,              \
                      hit_projections[proj].c_str());                  \
        tree->Branch(hname, &ARR[iL][proj]);                           \
      }

    BOOK("BPIX", vBPIX_TRKTriplets_, nBPIX)
    BOOK("FPIX", vFPIX_TRKTriplets_, nFPIX)
    BOOK("TIB",  vTIB_TRKTriplets_,  nTIB )
    BOOK("TID",  vTID_TRKTriplets_,  nTID )
    BOOK("TOB",  vTOB_TRKTriplets_,  nTOB )
    BOOK("TEC",  vTEC_TRKTriplets_,  nTEC )
#   undef BOOK
  }
}

void
RecHitAnalyzer::fillTRKTriplets(const edm::Event&  iEvent,
                                      const edm::EventSetup& iSetup,
                                      unsigned int      proj)
{
  zeroContainer(vBPIX_TRKTriplets_);
  zeroContainer(vFPIX_TRKTriplets_);
  zeroContainer(vTIB_TRKTriplets_ );
  zeroContainer(vTID_TRKTriplets_ );
  zeroContainer(vTOB_TRKTriplets_ );
  zeroContainer(vTEC_TRKTriplets_ );

  auto pushHit = [&](std::vector<HitTriple> layerHits[],
                     int layerIdx,
                     float value, float eta, float phi)
  {
    layerHits[layerIdx].push_back({value, eta, phi});
  };

  edm::Handle<SiPixelRecHitCollection> recHitColl;
  iEvent.getByToken(siPixelRecHitCollectionT_, recHitColl);

  edm::Handle<SiStripRecHit2DCollection>  stripRPhiRecHitColl;
  iEvent.getByToken(siStripRPhiRecHitCollectionT_, stripRPhiRecHitColl);

  edm::EDGetTokenT<reco::TrackCollection> tracksToken_;

  tracksToken_ = consumes<reco::TrackCollection>
                  (edm::InputTag("generalTracks"));

  edm::Handle<reco::TrackCollection> tracks;
  iEvent.getByToken(tracksToken_, tracks);

  // one scratch vector per BPIX/FPIX layer
  std::vector<HitTriple> scratchBPIX[nBPIX];
  std::vector<HitTriple> scratchFPIX[nFPIX];
  std::vector<HitTriple> scratchTIB[nTIB];
  std::vector<HitTriple> scratchTOB[nTOB];
  std::vector<HitTriple> scratchTID[nTID];
  std::vector<HitTriple> scratchTEC[nTEC];

  for (auto const& trk : *tracks) {
    float pt  = trk.pt();
    for (auto const& hit : trk.recHits()) {
      if (!hit || !hit->isValid()) continue;

      DetId detId = hit->geographicalId();
      unsigned layer = getLayer(detId, &tTopo) - 1;

      const auto  *detUnit = tkGeom->idToDetUnit(detId);
      GlobalPoint gp = detUnit->surface().toGlobal(hit->localPosition());
      float eta = gp.eta();
      float phi = gp.phi();

      // push hit with raw pT for now
      switch (detId.subdetId()) {
        case PixelSubdetector::PixelBarrel: pushHit(scratchBPIX, layer, pt, eta, phi); break;
        case PixelSubdetector::PixelEndcap: pushHit(scratchFPIX, layer, pt, eta, phi); break;
        case SiStripDetId::TIB: pushHit(scratchTIB, layer, pt, eta, phi); break;
        case SiStripDetId::TOB: pushHit(scratchTOB, layer, pt, eta, phi); break;
        case SiStripDetId::TID: pushHit(scratchTID, layer, pt, eta, phi); break;
        case SiStripDetId::TEC: pushHit(scratchTEC, layer, pt, eta, phi); break;
      }
    }
  }

  auto flushLayer = [](const std::vector<HitTriple> &src,
                       std::vector<float>           &dest)
  {
    std::vector<HitTriple> ordered(src);
    std::sort(ordered.begin(), ordered.end());     // descending by value

    const unsigned nKeep = std::min<unsigned>(ordered.size(), kMaxTripletHits);
    for (unsigned i = 0; i < nKeep; ++i) {
      dest[3*i]   = ordered[i].value;
      dest[3*i+1] = ordered[i].eta;
      dest[3*i+2] = ordered[i].phi;
    }
    // remaining slots already set to kPadValue
  };

  for (int l=0; l<nBPIX; ++l)
    flushLayer(scratchBPIX[l], vBPIX_TRKTriplets_[l][proj]);

  for (int l=0; l<nFPIX; ++l)
    flushLayer(scratchFPIX[l], vFPIX_TRKTriplets_[l][proj]);
  
  for (int l=0; l<nTIB; ++l)
    flushLayer(scratchTIB[l], vTIB_TRKTriplets_[l][proj]);

  for (int l=0; l<nTOB; ++l)
    flushLayer(scratchTOB[l], vTOB_TRKTriplets_[l][proj]);
  
  for (int l=0; l<nTID; ++l)
    flushLayer(scratchTID[l], vTID_TRKTriplets_[l][proj]);

  for (int l=0; l<nTEC; ++l)
    flushLayer(scratchTEC[l], vTEC_TRKTriplets_[l][proj]);
}
