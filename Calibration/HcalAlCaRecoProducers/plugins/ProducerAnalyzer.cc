// system include files
#include <memory>
#include <string>
#include <iostream>

// user include files

#include "DataFormats/Provenance/interface/StableProvenance.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/HcalCalibObjects/interface/HOCalibVariables.h"
#include "DataFormats/TrackReco/interface/TrackExtra.h"
#include "DataFormats/TrackReco/interface/TrackExtraFwd.h"
// collections
#include "DataFormats/HcalRecHit/interface/HcalRecHitCollections.h"
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "DataFormats/JetReco/interface/CaloJetCollection.h"
#include "DataFormats/HcalCalibObjects/interface/HOCalibVariableCollection.h"
#include "DataFormats/EgammaReco/interface/SuperCluster.h"
#include "DataFormats/EgammaReco/interface/SuperClusterFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/HcalTowerAlgo/interface/HcalGeometry.h"

#include "RecoTracker/TrackProducer/interface/TrackProducerBase.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"

namespace cms {

  //
  // class declaration
  //

  class ProducerAnalyzer : public edm::one::EDAnalyzer<> {
  public:
    explicit ProducerAnalyzer(const edm::ParameterSet&);
    ~ProducerAnalyzer() override;

    void analyze(const edm::Event&, const edm::EventSetup&) override;
    void beginJob() override {}
    void endJob() override {}

  private:
    // ----------member data ---------------------------
    std::string nameProd_;
    std::string jetCalo_;
    std::string gammaClus_;
    std::string ecalInput_;
    std::string hbheInput_;
    std::string hoInput_;
    std::string hfInput_;
    std::string tracks_;

    edm::EDGetTokenT<HOCalibVariableCollection> tok_hovar_;
    edm::EDGetTokenT<HORecHitCollection> tok_horeco_;
    edm::EDGetTokenT<HORecHitCollection> tok_ho_;
    edm::EDGetTokenT<HORecHitCollection> tok_hoProd_;

    edm::EDGetTokenT<HFRecHitCollection> tok_hf_;

    edm::EDGetTokenT<reco::CaloJetCollection> tok_jets_;
    edm::EDGetTokenT<reco::SuperClusterCollection> tok_gamma_;
    edm::EDGetTokenT<reco::MuonCollection> tok_muons_;
    edm::EDGetTokenT<EcalRecHitCollection> tok_ecal_;
    edm::EDGetTokenT<reco::TrackCollection> tok_tracks_;

    edm::EDGetTokenT<HBHERecHitCollection> tok_hbhe_;
    edm::EDGetTokenT<HBHERecHitCollection> tok_hbheProd_;

    edm::ESGetToken<CaloGeometry, CaloGeometryRecord> tok_geom_;
  };
}  // end namespace cms

using namespace reco;

namespace cms {

  //
  // constructors and destructor
  //
  ProducerAnalyzer::ProducerAnalyzer(const edm::ParameterSet& iConfig) {
    // get name of output file with histogramms

    nameProd_ = iConfig.getUntrackedParameter<std::string>("nameProd");
    jetCalo_ = iConfig.getUntrackedParameter<std::string>("jetCalo", "GammaJetJetBackToBackCollection");
    gammaClus_ = iConfig.getUntrackedParameter<std::string>("gammaClus", "GammaJetGammaBackToBackCollection");
    ecalInput_ = iConfig.getUntrackedParameter<std::string>("ecalInput", "GammaJetEcalRecHitCollection");
    hbheInput_ = iConfig.getUntrackedParameter<std::string>("hbheInput");
    hoInput_ = iConfig.getUntrackedParameter<std::string>("hoInput");
    hfInput_ = iConfig.getUntrackedParameter<std::string>("hfInput");
    tracks_ = iConfig.getUntrackedParameter<std::string>("Tracks", "GammaJetTracksCollection");

    tok_hovar_ = consumes<HOCalibVariableCollection>(edm::InputTag(nameProd_, hoInput_));
    tok_horeco_ = consumes<HORecHitCollection>(edm::InputTag("horeco"));
    tok_ho_ = consumes<HORecHitCollection>(edm::InputTag(hoInput_));
    tok_hoProd_ = consumes<HORecHitCollection>(edm::InputTag(nameProd_, hoInput_));

    tok_hf_ = consumes<HFRecHitCollection>(edm::InputTag(hfInput_));

    tok_jets_ = consumes<reco::CaloJetCollection>(edm::InputTag(nameProd_, jetCalo_));
    tok_gamma_ = consumes<reco::SuperClusterCollection>(edm::InputTag(nameProd_, gammaClus_));
    tok_muons_ = consumes<reco::MuonCollection>(edm::InputTag(nameProd_, "SelectedMuons"));
    tok_ecal_ = consumes<EcalRecHitCollection>(edm::InputTag(nameProd_, ecalInput_));
    tok_tracks_ = consumes<reco::TrackCollection>(edm::InputTag(nameProd_, tracks_));

    tok_hbheProd_ = consumes<HBHERecHitCollection>(edm::InputTag(nameProd_, hbheInput_));
    tok_hbhe_ = consumes<HBHERecHitCollection>(edm::InputTag(hbheInput_));

    tok_geom_ = esConsumes<CaloGeometry, CaloGeometryRecord>();
  }

  ProducerAnalyzer::~ProducerAnalyzer() {
    // do anything here that needs to be done at desctruction time
    // (e.g. close files, deallocate resources etc.)
  }

  //
  // member functions
  //

  // ------------ method called to produce the data  ------------
  void ProducerAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    using namespace edm;

    const CaloGeometry* geo = &(iSetup.getData(tok_geom_));

    std::vector<StableProvenance const*> theProvenance;
    iEvent.getAllStableProvenance(theProvenance);
    for (auto const& provenance : theProvenance) {
      edm::LogVerbatim("HcalAlCa") << " Print all module/label names " << provenance->moduleName() << " "
                                   << provenance->moduleLabel() << " " << provenance->productInstanceName();
    }

    if (nameProd_ == "hoCalibProducer") {
      edm::Handle<HOCalibVariableCollection> ho;
      iEvent.getByToken(tok_hovar_, ho);
      const HOCalibVariableCollection Hitho = *(ho.product());
      edm::LogVerbatim("HcalAlCa") << " Size of HO " << (Hitho).size();
    }

    if (nameProd_ == "ALCARECOMuAlZMuMu") {
      edm::Handle<HORecHitCollection> ho;
      iEvent.getByToken(tok_horeco_, ho);
      const HORecHitCollection Hitho = *(ho.product());
      edm::LogVerbatim("HcalAlCa") << " Size of HO " << (Hitho).size();
      edm::Handle<MuonCollection> mucand;
      iEvent.getByToken(tok_muons_, mucand);
      edm::LogVerbatim("HcalAlCa") << " Size of muon collection " << mucand->size();
      for (const auto& it : *(mucand.product())) {
        TrackRef mu = it.combinedMuon();
        edm::LogVerbatim("HcalAlCa") << " Pt muon " << mu->innerMomentum();
      }
    }

    if (nameProd_ != "IsoProd" && nameProd_ != "ALCARECOMuAlZMuMu" && nameProd_ != "hoCalibProducer") {
      edm::Handle<HBHERecHitCollection> hbhe;
      iEvent.getByToken(tok_hbhe_, hbhe);
      const HBHERecHitCollection Hithbhe = *(hbhe.product());
      edm::LogVerbatim("HcalAlCa") << " Size of HBHE " << (Hithbhe).size();

      edm::Handle<HORecHitCollection> ho;
      iEvent.getByToken(tok_ho_, ho);
      const HORecHitCollection Hitho = *(ho.product());
      edm::LogVerbatim("HcalAlCa") << " Size of HO " << (Hitho).size();

      edm::Handle<HFRecHitCollection> hf;
      iEvent.getByToken(tok_hf_, hf);
      const HFRecHitCollection Hithf = *(hf.product());
      edm::LogVerbatim("HcalAlCa") << " Size of HF " << (Hithf).size();
    }
    if (nameProd_ == "IsoProd") {
      edm::LogVerbatim("HcalAlCa") << " We are here ";
      edm::Handle<reco::TrackCollection> tracks;
      iEvent.getByToken(tok_tracks_, tracks);

      edm::LogVerbatim("HcalAlCa") << " Tracks size " << (*tracks).size();
      for (const auto& track : *(tracks.product())) {
        edm::LogVerbatim("HcalAlCa") << " P track " << track.p() << " eta " << track.eta() << " phi " << track.phi()
                                     << " Outer " << track.outerMomentum() << " " << track.outerPosition();
        const TrackExtraRef& myextra = track.extra();
        edm::LogVerbatim("HcalAlCa") << " Track extra " << myextra->outerMomentum() << " " << myextra->outerPosition();
      }

      edm::Handle<EcalRecHitCollection> ecal;
      iEvent.getByToken(tok_ecal_, ecal);
      const EcalRecHitCollection Hitecal = *(ecal.product());
      edm::LogVerbatim("HcalAlCa") << " Size of Ecal " << (Hitecal).size();

      double energyECAL = 0.;
      double energyHCAL = 0.;

      for (const auto& hite : *(ecal.product())) {
        const GlobalPoint& posE = geo->getPosition(hite.detid());

        edm::LogVerbatim("HcalAlCa") << " Energy ECAL " << hite.energy() << " eta " << posE.eta() << " phi "
                                     << posE.phi();

        energyECAL = energyECAL + hite.energy();
      }

      edm::Handle<HBHERecHitCollection> hbhe;
      iEvent.getByToken(tok_hbheProd_, hbhe);
      const HBHERecHitCollection Hithbhe = *(hbhe.product());
      edm::LogVerbatim("HcalAlCa") << " Size of HBHE " << (Hithbhe).size();

      for (const auto& hith : *(hbhe.product())) {
        GlobalPoint posH =
            (static_cast<const HcalGeometry*>(geo->getSubdetectorGeometry(hith.detid())))->getPosition(hith.detid());

        edm::LogVerbatim("HcalAlCa") << " Energy HCAL " << hith.energy() << " eta " << posH.eta() << " phi "
                                     << posH.phi();

        energyHCAL = energyHCAL + hith.energy();
      }

      edm::LogVerbatim("HcalAlCa") << " Energy ECAL " << energyECAL << " Energy HCAL " << energyHCAL;
    }

    if (nameProd_ == "GammaJetProd" || nameProd_ == "DiJProd") {
      edm::LogVerbatim("HcalAlCa") << " we are in GammaJetProd area ";
      edm::Handle<EcalRecHitCollection> ecal;
      iEvent.getByToken(tok_ecal_, ecal);
      edm::LogVerbatim("HcalAlCa") << " Size of ECAL " << (*ecal).size();

      edm::Handle<reco::CaloJetCollection> jets;
      iEvent.getByToken(tok_jets_, jets);
      edm::LogVerbatim("HcalAlCa") << " Jet size " << (*jets).size();

      for (const auto& jet : *(jets.product())) {
        edm::LogVerbatim("HcalAlCa") << " Et jet " << jet.et() << " eta " << jet.eta() << " phi " << jet.phi();
      }

      edm::Handle<reco::TrackCollection> tracks;
      iEvent.getByToken(tok_tracks_, tracks);
      edm::LogVerbatim("HcalAlCa") << " Tracks size " << (*tracks).size();
    }
    if (nameProd_ == "GammaJetProd") {
      edm::Handle<reco::SuperClusterCollection> eclus;
      iEvent.getByToken(tok_gamma_, eclus);
      edm::LogVerbatim("HcalAlCa") << " GammaClus size " << (*eclus).size();
      for (const auto& iclus : *(eclus.product())) {
        edm::LogVerbatim("HcalAlCa") << " Et gamma " << iclus.energy() / cosh(iclus.eta()) << " eta " << iclus.eta()
                                     << " phi " << iclus.phi();
      }
    }
  }
}  // namespace cms

#include "FWCore/PluginManager/interface/ModuleDef.h"
#include "FWCore/Framework/interface/MakerMacros.h"

using cms::ProducerAnalyzer;
DEFINE_FWK_MODULE(ProducerAnalyzer);
