// system includes
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>

// user includes
#include "CalibTracker/Records/interface/SiPixelTemplateDBObjectESProducerRcd.h"
#include "CondFormats/DataRecord/interface/SiPixelTemplateDBObjectRcd.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelTemplateDBObject.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/ESWatcher.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/FileInPath.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "MagneticField/Engine/interface/MagneticField.h"

class SiPixelTemplateDBObjectReader : public edm::one::EDAnalyzer<> {
public:
  explicit SiPixelTemplateDBObjectReader(const edm::ParameterSet&);
  ~SiPixelTemplateDBObjectReader() override = default;

private:
  void analyze(const edm::Event&, const edm::EventSetup&) override;

  edm::ESWatcher<SiPixelTemplateDBObjectESProducerRcd> SiPixTemplDBObjectWatcher_;
  edm::ESWatcher<SiPixelTemplateDBObjectRcd> SiPixTemplDBObjWatcher_;

  bool hasTriggeredWatcher;

  const std::string theTemplateCalibrationLocation;
  const bool theDetailedTemplateDBErrorOutput;
  const bool theFullTemplateDBOutput;
  const bool testGlobalTag;
  const edm::ESGetToken<MagneticField, IdealMagneticFieldRecord> magneticFieldToken_;
  const edm::ESGetToken<SiPixelTemplateDBObject, SiPixelTemplateDBObjectESProducerRcd> the1DTemplateESProdToken_;
  const edm::ESGetToken<SiPixelTemplateDBObject, SiPixelTemplateDBObjectRcd> the1DTemplateToken_;
};

SiPixelTemplateDBObjectReader::SiPixelTemplateDBObjectReader(const edm::ParameterSet& iConfig)
    : hasTriggeredWatcher(false),
      theTemplateCalibrationLocation(iConfig.getParameter<std::string>("siPixelTemplateCalibrationLocation")),
      theDetailedTemplateDBErrorOutput(iConfig.getParameter<bool>("wantDetailedTemplateDBErrorOutput")),
      theFullTemplateDBOutput(iConfig.getParameter<bool>("wantFullTemplateDBOutput")),
      testGlobalTag(iConfig.getParameter<bool>("TestGlobalTag")),
      magneticFieldToken_(esConsumes()),
      the1DTemplateESProdToken_(esConsumes()),
      the1DTemplateToken_(esConsumes()) {}

void SiPixelTemplateDBObjectReader::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  //To test with the ESProducer
  SiPixelTemplateDBObject dbobject;
  if (testGlobalTag) {
    // Get magnetic field
    GlobalPoint center(0.0, 0.0, 0.0);
    edm::ESHandle<MagneticField> magfield = iSetup.getHandle(magneticFieldToken_);
    float theMagField = magfield.product()->inTesla(center).mag();

    edm::LogPrint("SiPixelTemplateDBObjectReader") << "\nTesting global tag at magfield = " << theMagField;
    if (SiPixTemplDBObjWatcher_.check(iSetup)) {
      edm::LogPrint("SiPixelTemplateDBObjectReader") << "With record SiPixelTemplateDBObjectESProducerRcd";
      dbobject = *&iSetup.getData(the1DTemplateESProdToken_);
      hasTriggeredWatcher = true;
    }
  } else {
    edm::LogPrint("SiPixelTemplateDBObjectReader") << "\nLoading from file " << std::endl;
    if (SiPixTemplDBObjWatcher_.check(iSetup)) {
      edm::LogPrint("SiPixelTemplateDBObjectReader") << "With record SiPixelTemplateDBObjectRcd";
      dbobject = *&iSetup.getData(the1DTemplateToken_);
      hasTriggeredWatcher = true;
    }
  }

  if (hasTriggeredWatcher) {
    std::vector<short> tempMapId;

    if (theFullTemplateDBOutput)
      edm::LogPrint("SiPixelTemplateDBObjectReader") << "Map info" << std::endl;
    std::map<unsigned int, short> templMap = dbobject.getTemplateIDs();
    for (std::map<unsigned int, short>::const_iterator it = templMap.begin(); it != templMap.end(); ++it) {
      if (tempMapId.empty())
        tempMapId.push_back(it->second);
      for (unsigned int i = 0; i < tempMapId.size(); ++i) {
        if (tempMapId[i] == it->second)
          continue;
        else if (i == tempMapId.size() - 1) {
          tempMapId.push_back(it->second);
          break;
        }
      }
      if (theFullTemplateDBOutput)
        edm::LogPrint("SiPixelTemplateDBObjectReader")
            << "DetId: " << it->first << " TemplateID: " << it->second << "\n";
    }

    edm::LogPrint("SiPixelTemplateDBObjectReader") << "\nMap stores template Id(s): ";
    for (unsigned int vindex = 0; vindex < tempMapId.size(); ++vindex)
      edm::LogPrint("SiPixelTemplateDBObjectReader") << tempMapId[vindex] << " ";
    edm::LogPrint("SiPixelTemplateDBObjectReader") << std::endl;

    //local variables
    int numOfTempl = dbobject.numOfTempl();
    int index = 0;
    float tempnum = 0, diff = 0;
    float tol = 1.0E-23;
    bool error = false, givenErrorMsg = false;

    edm::LogPrint("SiPixelTemplateDBObjectReader")
        << "\nChecking Template DB object version " << dbobject.version() << " containing " << numOfTempl
        << " calibration(s) at " << dbobject.sVector()[index + 22] << "T\n";

    /*
    for(unsigned int kk=0;kk < dbobject.sVector().size(); kk++){
      edm::LogPrint("SiPixelTemplateDBObjectReader") << "dbobject.sVector()[" << kk <<"] = " << dbobject.sVector()[kk] << "\n";
    }
    */

    for (int i = 0; i < numOfTempl; ++i) {
      //Removes header in db object from diff
      index += 20;

      //Tell the person viewing the output what the template ID and version are -- note that version is only valid for >=13
      edm::LogPrint("SiPixelTemplateDBObjectReader")
          << "Calibration " << i + 1 << " of " << numOfTempl << ", with Template ID " << dbobject.sVector()[index]
          << "\tand Version " << dbobject.sVector()[index + 1] << "\t--------  ";

      //Opening the text-based template calibration
      std::ostringstream tout;
      tout << theTemplateCalibrationLocation.c_str() << "/data/template_summary_zp" << std::setw(4) << std::setfill('0')
           << std::right << dbobject.sVector()[index] << ".out" << std::ends;

      if (testGlobalTag)
        continue;

      edm::FileInPath file(tout.str());
      std::ifstream in_file(file.fullPath(), std::ios::in);

      if (in_file.is_open()) {
        //Removes header in textfile from diff
        //First read in from the text file -- this will be compared with index = 20
        in_file >> tempnum;

        //Read until the end of the current text file
        while (!in_file.eof()) {
          //Calculate the difference between the text file and the db object
          diff = std::abs(tempnum - dbobject.sVector()[index]);

          //Is there a difference?
          if (diff > tol) {
            //We have the if statement to output the message only once
            if (!givenErrorMsg)
              edm::LogPrint("SiPixelTemplateDBObjectReader") << "does NOT match\n";
            //If there is an error we want to display a message upon completion
            error = true;
            givenErrorMsg = true;
            //Do we want more detailed output?
            if (theDetailedTemplateDBErrorOutput) {
              edm::LogPrint("SiPixelTemplateDBObjectReader")
                  << "from file = " << tempnum << "\t from dbobject = " << dbobject.sVector()[index]
                  << "\tdiff = " << diff << "\t db index = " << index << std::endl;
            }
          }
          //Go to the next entries
          in_file >> tempnum;
          ++index;
        }
        //There were no errors, the two files match.
        if (!givenErrorMsg)
          edm::LogPrint("SiPixelTemplateDBObjectReader") << "MATCHES\n";
      }  //end current file
      in_file.close();
      givenErrorMsg = false;
    }  //end loop over all files

    if (error && !theDetailedTemplateDBErrorOutput)
      edm::LogPrint("SiPixelTemplateDBObjectReader")
          << "\nThe were differences found between the files and the database.\n"
          << "If you would like more detailed information please set\n"
          << "wantDetailedOutput = True in the cfg file. If you would like a\n"
          << "full output of the contents of the database file please set\n"
          << "wantFullOutput = True. Make sure that you pipe the output to a\n"
          << "log file. This could take a few minutes.\n\n";

    if (theFullTemplateDBOutput)
      edm::LogPrint("SiPixelTemplateDBObjectReader") << dbobject << std::endl;
  }
}

std::ostream& operator<<(std::ostream& s, const SiPixelTemplateDBObject& dbobject) {
  //!-index to keep track of where we are in the object
  int index = 0;
  //!-these are modifiable parameters for the extended templates
  int txsize[4] = {7, 13, 0, 0};
  int tysize[4] = {21, 21, 0, 0};
  //!-entries takes the number of entries in By,Bx,Fy,Fx from the object
  int entries[4] = {0};
  //!-local indicies for loops
  int i, j, k, l, m, n, entry_it;
  //!-changes the size of the templates based on the version
  int sizeSetter = 0, templateVersion = 0;

  edm::LogPrint("SiPixelTemplateDBObjectReader") << "\n\nDBobject version: " << dbobject.version() << std::endl;

  for (m = 0; m < dbobject.numOfTempl(); ++m) {
    //To change the size of the output based on which template version we are using"
    templateVersion = (int)dbobject.sVector_[index + 21];
    if (templateVersion <= 10) {
      edm::LogPrint("SiPixelTemplateDBObjectReader")
          << "*****WARNING***** This code will not format this template version properly *****WARNING*****\n";
      sizeSetter = 0;
    } else if (templateVersion <= 16)
      sizeSetter = 1;
    else
      edm::LogPrint("SiPixelTemplateDBObjectReader")
          << "*****WARNING***** This code has not been tested at formatting this version *****WARNING*****\n";

    edm::LogPrint("SiPixelTemplateDBObjectReader")
        << "\n\n*********************************************************************************************"
        << std::endl;
    edm::LogPrint("SiPixelTemplateDBObjectReader")
        << "***************                  Reading Template ID " << dbobject.sVector_[index + 20] << "\t(" << m + 1
        << "/" << dbobject.numOfTempl_ << ")                 ***************" << std::endl;
    edm::LogPrint("SiPixelTemplateDBObjectReader")
        << "*********************************************************************************************\n\n"
        << std::endl;

    //Header Title
    SiPixelTemplateDBObject::char2float temp;
    for (n = 0; n < 20; ++n) {
      temp.f = dbobject.sVector_[index];
      s << temp.c[0] << temp.c[1] << temp.c[2] << temp.c[3];
      ++index;
    }

    entries[0] = (int)dbobject.sVector_[index + 3];                                   // Y
    entries[1] = (int)(dbobject.sVector_[index + 4] * dbobject.sVector_[index + 5]);  // X

    //Header
    s << dbobject.sVector_[index] << "\t" << dbobject.sVector_[index + 1] << "\t" << dbobject.sVector_[index + 2]
      << "\t" << dbobject.sVector_[index + 3] << "\t" << dbobject.sVector_[index + 4] << "\t"
      << dbobject.sVector_[index + 5] << "\t" << dbobject.sVector_[index + 6] << "\t" << dbobject.sVector_[index + 7]
      << "\t" << dbobject.sVector_[index + 8] << "\t" << dbobject.sVector_[index + 9] << "\t"
      << dbobject.sVector_[index + 10] << "\t" << dbobject.sVector_[index + 11] << "\t" << dbobject.sVector_[index + 12]
      << "\t" << dbobject.sVector_[index + 13] << "\t" << dbobject.sVector_[index + 14] << "\t"
      << dbobject.sVector_[index + 15] << "\t" << dbobject.sVector_[index + 16] << std::endl;
    index += 17;

    //Loop over By,Bx,Fy,Fx
    for (entry_it = 0; entry_it < 4; ++entry_it) {
      //Run,costrk,qavg,...,clslenx
      for (i = 0; i < entries[entry_it]; ++i) {
        s << dbobject.sVector_[index] << "\t" << dbobject.sVector_[index + 1] << "\t" << dbobject.sVector_[index + 2]
          << "\t" << dbobject.sVector_[index + 3] << "\n"
          << dbobject.sVector_[index + 4] << "\t" << dbobject.sVector_[index + 5] << "\t"
          << dbobject.sVector_[index + 6] << "\t" << dbobject.sVector_[index + 7] << "\t"
          << dbobject.sVector_[index + 8] << "\t" << dbobject.sVector_[index + 9] << "\t"
          << dbobject.sVector_[index + 10] << "\t" << dbobject.sVector_[index + 11] << "\n"
          << dbobject.sVector_[index + 12] << "\t" << dbobject.sVector_[index + 13] << "\t"
          << dbobject.sVector_[index + 14] << "\t" << dbobject.sVector_[index + 15] << "\t"
          << dbobject.sVector_[index + 16] << "\t" << dbobject.sVector_[index + 17] << "\t"
          << dbobject.sVector_[index + 18] << std::endl;
        index += 19;
        //YPar
        for (j = 0; j < 2; ++j) {
          for (k = 0; k < 5; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //YTemp
        for (j = 0; j < 9; ++j) {
          for (k = 0; k < tysize[sizeSetter]; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //XPar
        for (j = 0; j < 2; ++j) {
          for (k = 0; k < 5; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //XTemp
        for (j = 0; j < 9; ++j) {
          for (k = 0; k < txsize[sizeSetter]; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //Y average reco params
        for (j = 0; j < 4; ++j) {
          for (k = 0; k < 4; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //Yflpar
        for (j = 0; j < 4; ++j) {
          for (k = 0; k < 6; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //X average reco params
        for (j = 0; j < 4; ++j) {
          for (k = 0; k < 4; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //Xflpar
        for (j = 0; j < 4; ++j) {
          for (k = 0; k < 6; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //Chi2X,Y
        for (j = 0; j < 4; ++j) {
          for (k = 0; k < 2; ++k) {
            for (l = 0; l < 2; ++l) {
              s << dbobject.sVector_[index] << "\t";
              ++index;
            }
          }
          s << std::endl;
        }
        //Y average Chi2 params
        for (j = 0; j < 4; ++j) {
          for (k = 0; k < 4; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //X average Chi2 params
        for (j = 0; j < 4; ++j) {
          for (k = 0; k < 4; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //Y average reco params for CPE Generic
        for (j = 0; j < 4; ++j) {
          for (k = 0; k < 4; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //X average reco params for CPE Generic
        for (j = 0; j < 4; ++j) {
          for (k = 0; k < 4; ++k) {
            s << dbobject.sVector_[index] << "\t";
            ++index;
          }
          s << std::endl;
        }
        //SpareX,Y
        for (j = 0; j < 20; ++j) {
          s << dbobject.sVector_[index] << "\t";
          ++index;
          if (j == 9 || j == 19)
            s << std::endl;
        }
      }
    }
  }
  return s;
}

DEFINE_FWK_MODULE(SiPixelTemplateDBObjectReader);
