// File: DataMixingHcalDigiWorkerProd.cc
// Description:  see DataMixingHcalDigiWorkerProd.h
// Author:  Mike Hildreth, University of Notre Dame
//
//--------------------------------------------

#include "DataMixingHcalDigiWorkerProd.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Utilities/interface/EDMException.h"

#include "FWCore/AbstractServices/interface/RandomNumberGenerator.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

using namespace std;
namespace edm {
  // Constructor
  DataMixingHcalDigiWorkerProd::DataMixingHcalDigiWorkerProd(const edm::ParameterSet &ps,
                                                             edm::ConsumesCollector &&iC,
                                                             const edm::ESGetToken<HcalDbService, HcalDbRecord> &tok)
      : HBHEPileInputTag_(ps.getParameter<edm::InputTag>("HBHEPileInputTag")),
        HOPileInputTag_(ps.getParameter<edm::InputTag>("HOPileInputTag")),
        HFPileInputTag_(ps.getParameter<edm::InputTag>("HFPileInputTag")),
        ZDCPileInputTag_(ps.getParameter<edm::InputTag>("ZDCPileInputTag")),
        QIE10PileInputTag_(ps.getParameter<edm::InputTag>("QIE10PileInputTag")),
        QIE11PileInputTag_(ps.getParameter<edm::InputTag>("QIE11PileInputTag")),
        tokDB_(tok),
        label_(ps.getParameter<std::string>("Label")) {
    //
    tok_hbhe_ = iC.consumes<HBHEDigitizerTraits::DigiCollection>(HBHEPileInputTag_);
    tok_ho_ = iC.consumes<HODigitizerTraits::DigiCollection>(HOPileInputTag_);
    tok_hf_ = iC.consumes<HFDigitizerTraits::DigiCollection>(HFPileInputTag_);
    tok_zdc_ = iC.consumes<ZDCDigitizerTraits::DigiCollection>(ZDCPileInputTag_);
    tok_qie10_ = iC.consumes<HcalQIE10DigitizerTraits::DigiCollection>(QIE10PileInputTag_);
    tok_qie11_ = iC.consumes<HcalQIE11DigitizerTraits::DigiCollection>(QIE11PileInputTag_);

    theHBHESignalGenerator = HBHESignalGenerator(HBHEPileInputTag_, tok_hbhe_);
    theHOSignalGenerator = HOSignalGenerator(HOPileInputTag_, tok_ho_);
    theHFSignalGenerator = HFSignalGenerator(HFPileInputTag_, tok_hf_);
    theZDCSignalGenerator = ZDCSignalGenerator(ZDCPileInputTag_, tok_zdc_);
    theQIE10SignalGenerator = QIE10SignalGenerator(QIE10PileInputTag_, tok_qie10_);
    theQIE11SignalGenerator = QIE11SignalGenerator(QIE11PileInputTag_, tok_qie11_);

    // get the subdetector names
    //    this->getSubdetectorNames();  //something like this may be useful to
    //    check what we are supposed to do...

    // declare the products to produce

    // Hcal
    // Signal inputs now handled by HcalDigitizer - gets pSimHits directly

    HBHEDigiCollectionDM_ = ps.getParameter<std::string>("HBHEDigiCollectionDM");
    HODigiCollectionDM_ = ps.getParameter<std::string>("HODigiCollectionDM");
    HFDigiCollectionDM_ = ps.getParameter<std::string>("HFDigiCollectionDM");
    ZDCDigiCollectionDM_ = ps.getParameter<std::string>("ZDCDigiCollectionDM");
    QIE10DigiCollectionDM_ = ps.getParameter<std::string>("QIE10DigiCollectionDM");
    QIE11DigiCollectionDM_ = ps.getParameter<std::string>("QIE11DigiCollectionDM");

    // initialize HcalDigitizer here...

    myHcalDigitizer_ = new HcalDigiProducer(ps, iC);

    myHcalDigitizer_->setHBHENoiseSignalGenerator(&theHBHESignalGenerator);
    myHcalDigitizer_->setHFNoiseSignalGenerator(&theHFSignalGenerator);
    myHcalDigitizer_->setHONoiseSignalGenerator(&theHOSignalGenerator);
    myHcalDigitizer_->setZDCNoiseSignalGenerator(&theZDCSignalGenerator);
    myHcalDigitizer_->setQIE10NoiseSignalGenerator(&theQIE10SignalGenerator);
    myHcalDigitizer_->setQIE11NoiseSignalGenerator(&theQIE11SignalGenerator);
  }

  // Virtual destructor needed.
  DataMixingHcalDigiWorkerProd::~DataMixingHcalDigiWorkerProd() { delete myHcalDigitizer_; }

  void DataMixingHcalDigiWorkerProd::beginRun(const edm::Run &run, const edm::EventSetup &ES) {
    myHcalDigitizer_->beginRun(run, ES);
  }

  void DataMixingHcalDigiWorkerProd::initializeEvent(const edm::Event &e, const edm::EventSetup &ES) {
    myHcalDigitizer_->initializeEvent(e, ES);
  }

  void DataMixingHcalDigiWorkerProd::addHcalSignals(const edm::Event &e, const edm::EventSetup &ES) {
    myHcalDigitizer_->accumulate(e, ES);

  }  // end of addHcalSignals

  void DataMixingHcalDigiWorkerProd::addHcalPileups(const int bcr,
                                                    const EventPrincipal *ep,
                                                    unsigned int eventNr,
                                                    const edm::EventSetup &ES,
                                                    edm::ModuleCallingContext const *mcc) {
    LogDebug("DataMixingHcalDigiWorkerProd")
        << "\n===============> adding pileups from event  " << ep->id() << " for bunchcrossing " << bcr;

    theHBHESignalGenerator.initializeEvent(ep, &ES, tokDB_);
    theHOSignalGenerator.initializeEvent(ep, &ES, tokDB_);
    theHFSignalGenerator.initializeEvent(ep, &ES, tokDB_);
    theZDCSignalGenerator.initializeEvent(ep, &ES, tokDB_);
    theQIE10SignalGenerator.initializeEvent(ep, &ES, tokDB_);
    theQIE11SignalGenerator.initializeEvent(ep, &ES, tokDB_);

    // put digis from pileup event into digitizer

    theHBHESignalGenerator.fill(mcc);
    theHOSignalGenerator.fill(mcc);
    theHFSignalGenerator.fill(mcc);
    theQIE10SignalGenerator.fill(mcc);
    theQIE11SignalGenerator.fill(mcc);
  }

  void DataMixingHcalDigiWorkerProd::putHcal(edm::Event &e, const edm::EventSetup &ES) {
    // Digitize
    // edm::Service<edm::RandomNumberGenerator> rng;
    // CLHEP::HepRandomEngine* engine = &rng->getEngine(e.streamID());

    // myHcalDigitizer_->initializeEvent( e, ES );

    //    myHcalDigitizer_->finalizeEvent( e, ES, engine );
    myHcalDigitizer_->finalizeEvent(e, ES);
  }

}  // namespace edm
