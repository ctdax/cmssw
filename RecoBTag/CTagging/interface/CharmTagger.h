#ifndef RecoBTag_CTagging_CharmTagger_h
#define RecoBTag_CTagging_CharmTagger_h

#include "CommonTools/MVAUtils/interface/TMVAEvaluator.h"
#include "DataFormats/BTauReco/interface/TaggingVariable.h"
#include "FWCore/Framework/interface/ESConsumesCollector.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "RecoBTag/SecondaryVertex/interface/CombinedSVSoftLeptonComputer.h"
#include "RecoBTau/JetTagComputer/interface/JetTagComputer.h"
#include "RecoBTau/JetTagComputer/interface/JetTagComputerRecord.h"

#include <memory>

/** \class CharmTagger
 *  \author M. Verzetti, U. Rochester, N.Y.
 *  copied from ElectronTagger.h
 */

class CharmTagger : public JetTagComputer {
public:
  struct Tokens {
    Tokens(const edm::ParameterSet& configuration, edm::ESConsumesCollector&& cc);
    edm::ESGetToken<GBRForest, GBRWrapperRcd> gbrForest_;
  };

  /// explicit ctor
  CharmTagger(const edm::ParameterSet&, Tokens);
  ~CharmTagger() override;  //{}
  float discriminator(const TagInfoHelper& tagInfo) const override;
  void initialize(const JetTagComputerRecord& record) override;

  static void fillPSetDescription(edm::ParameterSetDescription& desc);

  typedef std::vector<edm::ParameterSet> vpset;

  struct MVAVar {
    std::string name;
    reco::btau::TaggingVariableName id;
    size_t index;
    bool has_index;
    float default_value;
  };

private:
  std::unique_ptr<TMVAEvaluator> mvaID_;
  CombinedSVSoftLeptonComputer sl_computer_;
  std::vector<MVAVar> variables_;

  std::string mva_name_;
  edm::FileInPath weight_file_;
  bool use_GBRForest_;
  bool use_adaBoost_;
  bool defaultValueNoTracks_;
  Tokens tokens_;
};

#endif
