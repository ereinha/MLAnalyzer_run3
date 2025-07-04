#include "MLAnalyzer_run3/RecHitAnalyzer/interface/RecHitAnalyzer.h"
#include <algorithm>

using std::vector;
using namespace trigger;

std::vector<unsigned int> vAs_ditau;
std::vector<unsigned int> vAs_diphoton;
std::vector<unsigned int> vPhotons;

std::vector<unsigned int> vReco_First_Photons_Idxs;
std::vector<unsigned int> vReco_Second_Photons_Idxs;
std::vector<unsigned int> vGen_As_diphoton_Idxs;
std::vector<unsigned int> vGen_As_ditau_Idxs;

std::vector<float> vA_diphoton_gen_m0_;
std::vector<float> vA_diphoton_gen_dR_;
std::vector<float> vA_diphoton_gen_E_;
std::vector<float> vA_diphoton_gen_pT_;
std::vector<float> vA_diphoton_gen_eta_;
std::vector<float> vA_diphoton_gen_phi_;
std::vector<float> vA_diphoton_reco_M_;
std::vector<float> vA_diphoton_reco_dR_;
std::vector<float> vA_diphoton_reco_E_;
std::vector<float> vA_diphoton_reco_pT_;
std::vector<float> vA_diphoton_reco_eta_;
std::vector<float> vA_diphoton_reco_phi_;

std::vector<float> vA_ditau_gen_m0_;
std::vector<float> vA_ditau_gen_dR_;
std::vector<float> vA_ditau_gen_E_;
std::vector<float> vA_ditau_gen_pT_;
std::vector<float> vA_ditau_gen_eta_;
std::vector<float> vA_ditau_gen_phi_;

struct gen_obj {
  unsigned int idx;
  double pt;
};

// Initialize branches _____________________________________________________//
void RecHitAnalyzer::branchesEvtSel_jet_h2aa2ditau_dipho ( TTree* tree, edm::Service<TFileService> &fs )
{
  tree->Branch("A_diphoton_gen_m0",         &vA_diphoton_gen_m0_);
  tree->Branch("A_diphoton_gen_dR",         &vA_diphoton_gen_dR_);
  tree->Branch("A_diphoton_gen_E",           &vA_diphoton_gen_E_);
  tree->Branch("A_diphoton_gen_pT",         &vA_diphoton_gen_pT_);
  tree->Branch("A_diphoton_gen_eta",       &vA_diphoton_gen_eta_);
  tree->Branch("A_diphoton_gen_phi",       &vA_diphoton_gen_phi_);
  tree->Branch("A_diphoton_reco_M",         &vA_diphoton_reco_M_);
  tree->Branch("A_diphoton_reco_dR",       &vA_diphoton_reco_dR_);
  tree->Branch("A_diphoton_reco_E",         &vA_diphoton_reco_E_);
  tree->Branch("A_diphoton_reco_pT",       &vA_diphoton_reco_pT_);
  tree->Branch("A_diphoton_reco_eta",     &vA_diphoton_reco_eta_);
  tree->Branch("A_diphoton_reco_phi",     &vA_diphoton_reco_phi_);

  tree->Branch("A_ditau_gen_m0",        &vA_ditau_gen_m0_);
  tree->Branch("A_ditau_gen_dR",        &vA_ditau_gen_dR_);
  tree->Branch("A_ditau_gen_E",          &vA_ditau_gen_E_);
  tree->Branch("A_ditau_gen_pT",        &vA_ditau_gen_pT_);
  tree->Branch("A_ditau_gen_eta",      &vA_ditau_gen_eta_);
  tree->Branch("A_ditau_gen_phi",      &vA_ditau_gen_phi_);
}

// Run event selection ___________________________________________________________________//
bool RecHitAnalyzer::runEvtSel_jet_h2aa2ditau_dipho ( const edm::Event& iEvent, const edm::EventSetup& iSetup ) {
  edm::Handle<reco::GenParticleCollection> genParticles;
  iEvent.getByToken(genParticleCollectionT_, genParticles);
  edm::Handle<reco::PhotonCollection> photons;
  iEvent.getByToken( photonCollectionT_, photons );

  vAs_diphoton.clear();
  vAs_ditau.clear();
  vPhotons.clear();
  vGen_As_ditau_Idxs.clear();
  vGen_As_diphoton_Idxs.clear();
  vReco_First_Photons_Idxs .clear();
  vReco_Second_Photons_Idxs.clear();
 
  ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > vH;

  for ( unsigned int iG = 0; iG < genParticles->size(); iG++ ) {

    reco::GenParticleRef iGen( genParticles, iG );
    if ( std::abs(iGen->pdgId()) != 35 && std::abs(iGen->pdgId()) != 36 ) continue;
    if ( iGen->numberOfDaughters() != 2 ) continue;
    if ( abs(iGen->daughter(0)->pdgId()) == 22 || abs(iGen->daughter(1)->pdgId()) == 22 ) {
      if ( debug ) std::cout<<"*****************************************************"<< std::endl;
      if ( debug ) std::cout<< "iG:" << iG << " ID:" << iGen->pdgId() << " A->diphoton mass:" << iGen->mass() << std::endl;
      vAs_diphoton.push_back( iG );
      vH += iGen->p4();

    } else if ( abs(iGen->daughter(0)->pdgId()) == 15 || abs(iGen->daughter(1)->pdgId()) == 15 ) {
      if ( debug ) std::cout<<"*****************************************************"<< std::endl;
      if ( debug ) std::cout<< "iG:" << iG << " ID:" << iGen->pdgId() << " A->ditau mass:" << iGen->mass() << std::endl;
      vAs_ditau.push_back( iG );
      vH += iGen->p4();
    } else continue;
  }

  if ( vAs_diphoton.size() != 1 || vAs_ditau.size() != 1) return false;

  reco::GenParticleRef iGenA1( genParticles, vAs_ditau[0] );
  if ( debug ) std::cout << " >> pT:" << iGenA1->pt() << " eta:" << iGenA1->eta() << " phi: " << iGenA1->phi() << " E:" << iGenA1->energy() << std::endl;

  vPhotons.clear();
  reco::GenParticleRef iGenA2( genParticles, vAs_diphoton[0] );

  std::vector<int> matchedPhotons; // indices in reco::PhotonCollection
  std::set<int>    usedReco; // to keep duplicates away

  for (unsigned int d = 0; d < iGenA2->numberOfDaughters(); ++d) {

    const reco::Candidate * gPho = iGenA2->daughter(d);
    if (std::abs(gPho->pdgId()) != 22) continue; // skip non-photon daughters

    double bestDR  = 0.10; // tighter than the parent dR
    int    bestIdx = -1;

    for (unsigned int iP = 0; iP < photons->size(); ++iP) {
      if (usedReco.count(iP)) continue; // already matched
      const auto &recoPho = photons->at(iP);
      if (recoPho.pt() < 5.) continue; // pT cut

      double dR = reco::deltaR(recoPho.eta(), recoPho.phi(),
                              gPho->eta(),    gPho->phi());
      if (dR < bestDR) { bestDR = dR; bestIdx = iP; }
    }

    if (bestIdx >= 0) {
      matchedPhotons.push_back(bestIdx);
      usedReco.insert(bestIdx);
    }
  }

  if (matchedPhotons.size() != 2) return false; // need exactly two

  // order by pT so the first index is always the hardest photon
  std::sort(matchedPhotons.begin(), matchedPhotons.end(),
            [&](int i, int j){ return photons->at(i).pt() > photons->at(j).pt(); });

  vReco_First_Photons_Idxs.push_back( matchedPhotons[0] );
  vReco_Second_Photons_Idxs.push_back( matchedPhotons[1] );


  vGen_As_ditau_Idxs.push_back( vAs_ditau[0] );
  if ( debug ) std::cout << " >> pT:" << iGenA2->pt() << " eta:" << iGenA2->eta() << " phi: " << iGenA2->phi() << " E:" << iGenA2->energy() << std::endl;
  vGen_As_diphoton_Idxs.push_back( vAs_diphoton[0] );

  return true;
}

// Fill branches ___________________________________________________________________//
void RecHitAnalyzer::fillEvtSel_jet_h2aa2ditau_dipho ( const edm::Event& iEvent, const edm::EventSetup& iSetup )
{

  edm::Handle<reco::GenParticleCollection> genParticles;
  iEvent.getByToken(genParticleCollectionT_, genParticles);

  edm::Handle<reco::PhotonCollection> photons;
  iEvent.getByToken(photonCollectionT_, photons);

  //edm::Handle<PhotonCollection> photons;
  //iEvent.getByToken(photonCollectionT_, photons);

  vA_diphoton_gen_m0_.clear();
  vA_diphoton_gen_dR_.clear();
  vA_diphoton_gen_E_.clear();
  vA_diphoton_gen_pT_.clear();
  vA_diphoton_gen_eta_.clear();
  vA_diphoton_gen_phi_.clear();
  vA_diphoton_reco_M_.clear();
  vA_diphoton_reco_dR_.clear();
  vA_diphoton_reco_E_.clear();
  vA_diphoton_reco_pT_.clear();
  vA_diphoton_reco_eta_.clear();
  vA_diphoton_reco_phi_.clear();

  vA_ditau_gen_E_.clear();
  vA_ditau_gen_pT_.clear();
  vA_ditau_gen_eta_.clear();
  vA_ditau_gen_phi_.clear();
  vA_ditau_gen_m0_.clear();
  vA_ditau_gen_dR_.clear();

  vAs_diphoton.clear();
  vAs_ditau.clear();

  for ( unsigned int iG : vGen_As_ditau_Idxs ) {

    reco::GenParticleRef iGen( genParticles, iG );

    vA_ditau_gen_E_.push_back( std::abs(iGen->energy()) );
    vA_ditau_gen_pT_.push_back( std::abs(iGen->pt()) );
    vA_ditau_gen_eta_.push_back( iGen->eta() );
    vA_ditau_gen_phi_.push_back( iGen->phi() );
    vA_ditau_gen_m0_.push_back( iGen->mass() );
    vA_ditau_gen_dR_.push_back( reco::deltaR(iGen->daughter(0)->eta(),iGen->daughter(0)->phi(), iGen->daughter(1)->eta(),iGen->daughter(1)->phi()) );

  }

  for ( unsigned int iG : vGen_As_diphoton_Idxs ) {

    reco::GenParticleRef iGen( genParticles, iG );

    vA_diphoton_gen_E_.push_back( std::abs(iGen->energy()) );
    vA_diphoton_gen_pT_.push_back( std::abs(iGen->pt()) );
    vA_diphoton_gen_eta_.push_back( iGen->eta() );
    vA_diphoton_gen_phi_.push_back( iGen->phi() );
    vA_diphoton_gen_m0_.push_back( iGen->mass() );
    vA_diphoton_gen_dR_.push_back( reco::deltaR(iGen->daughter(0)->eta(),iGen->daughter(0)->phi(), iGen->daughter(1)->eta(),iGen->daughter(1)->phi()) );
  }

  for ( unsigned int iP = 0; iP < vReco_First_Photons_Idxs.size(); ++iP ) {
    unsigned int idx1 = vReco_First_Photons_Idxs[iP];
    unsigned int idx2 = vReco_Second_Photons_Idxs[iP];
  
    reco::PhotonRef photon1( photons, idx1 );
    reco::PhotonRef photon2( photons, idx2 );
  
    auto diphoton_A = photon1->p4() + photon2->p4();
  
    vA_diphoton_reco_E_ .push_back( std::abs(diphoton_A.energy()) );
    vA_diphoton_reco_pT_.push_back( std::abs(diphoton_A.pt()) );
    vA_diphoton_reco_eta_.push_back( diphoton_A.eta() );
    vA_diphoton_reco_phi_.push_back( diphoton_A.phi() );
    vA_diphoton_reco_M_ .push_back( diphoton_A.mass() );
    vA_diphoton_reco_dR_.push_back(
      reco::deltaR(photon1->eta(), photon1->phi(),
                    photon2->eta(), photon2->phi())
    );
  }

}
