#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "Geometry/HGCalCommonData/interface/HGCalCassette.h"
#include <algorithm>
#include <sstream>

//#define EDM_ML_DEBUG

void HGCalCassette::setParameter(int cassette, const std::vector<double>& shifts, bool both) {
  cassette_ = cassette;
  typeHE_ = (cassette_ >= 12);
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "HGCalCassette::setParameter Cassette " << cassette << " Both " << both << " Size "
                                << shifts.size();
#endif
  shifts_.insert(shifts_.end(), shifts.begin(), shifts.end());
  if (both)
    shiftsScnt_.insert(shiftsScnt_.end(), shifts.begin(), shifts.end());
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "# of cassettes = " << cassette_ << " Type " << typeHE_;
  for (uint32_t j1 = 0; j1 < shifts.size(); j1 += 12) {
    std::ostringstream st1;
    if (j1 == 0) {
      if (both)
        st1 << " Shifts|ShiftsScnt:";
      else
        st1 << " Shifts:";
    } else {
      if (both)
        st1 << "                   ";
      else
        st1 << "        ";
    }
    uint32_t j2 = std::min((j1 + 12), static_cast<uint32_t>(shifts.size()));
    for (uint32_t j = j1; j < j2; ++j)
      st1 << ":" << shifts[j];
    edm::LogVerbatim("HGCalGeom") << st1.str();
  }
#endif
}

void HGCalCassette::setParameterScint(const std::vector<double>& shifts) {
  shiftsScnt_.insert(shiftsScnt_.end(), shifts.begin(), shifts.end());
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "HGCalCassette::setParameterScint with Size " << shifts.size();
  for (uint32_t j1 = 0; j1 < shifts.size(); j1 += 12) {
    std::ostringstream st1;
    if (j1 == 0) {
      st1 << " ShiftsScnt:";
    } else {
      st1 << "            ";
    }
    uint32_t j2 = std::min((j1 + 12), static_cast<uint32_t>(shifts.size()));
    for (uint32_t j = j1; j < j2; ++j)
      st1 << ":" << shifts[j];
    edm::LogVerbatim("HGCalGeom") << st1.str();
  }
#endif
}

void HGCalCassette::setParameterRetract(const std::vector<double>& shifts) {
  retractScnt_.insert(retractScnt_.end(), shifts.begin(), shifts.end());
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "HGCalCassette::setParameterRetract with Size " << shifts.size();
  for (uint32_t j1 = 0; j1 < shifts.size(); j1 += 12) {
    std::ostringstream st1;
    if (j1 == 0) {
      st1 << " RetractScnt:";
    } else {
      st1 << "             ";
    }
    uint32_t j2 = std::min((j1 + 12), static_cast<uint32_t>(shifts.size()));
    for (uint32_t j = j1; j < j2; ++j)
      st1 << ":" << shifts[j];
    edm::LogVerbatim("HGCalGeom") << st1.str();
  }
#endif
}

std::pair<double, double> HGCalCassette::getShift(int layer, int zside, int cassette, bool scnt) const {
  int locc = (zside < 0) ? (cassette - 1) : (typeHE_ ? positHE_[cassette - 1] : positEE_[cassette - 1]);
  int loc = 2 * (cassette_ * (layer - 1) + locc);
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "HGCalCassette::getShift: layer|zside|cassett|scnt " << layer << ":" << zside << ":"
                                << cassette << ":" << scnt << " loc " << locc << ":" << loc << " size "
                                << shiftsScnt_.size() << ":" << shifts_.size();
#endif
  std::pair<double, double> xy = (typeHE_ && scnt) ? (std::make_pair(shiftsScnt_[loc], shiftsScnt_[loc + 1]))
                                                   : (std::make_pair(shifts_[loc], shifts_[loc + 1]));
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "HGCalCassette::getShift: Layer " << layer << " zside " << zside << " type "
                                << typeHE_ << " cassette " << cassette << " Loc " << locc << ":" << loc << " shift "
                                << xy.first << ":" << xy.second;
#endif
  return xy;
}

std::pair<double, double> HGCalCassette::getShiftScnt(int layer, int zside, double phi) const {
  int loc = (layer - 1);
  double fac = (zside < 0) ? 1.0 : -1.0;
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "HGCalCassette::getShiftScnt: layer|zside|phi " << layer << ":" << zside << ":"
                                << phi << " loc " << loc << " size " << retractScnt_.size();
#endif
  std::pair<double, double> xy = std::make_pair(fac * retractScnt_[loc] * cos(phi), retractScnt_[loc] * sin(phi));
#ifdef EDM_ML_DEBUG
  edm::LogVerbatim("HGCalGeom") << "HGCalCassette::getShiftScnt: Layer " << layer << " zside " << zside << " Loc "
                                << loc << " fac " << fac << " shift " << xy.first << ":" << xy.second;
#endif
  return xy;
}

int HGCalCassette::cassetteIndex(int det, int layer, int side, int cassette) {
  int zs = (side > 0) ? factor_ : 0;
  return (((zs + det) * factor_ + layer) * factor_ + cassette);
}

int HGCalCassette::cassetteType(int det, int zside, int cassette) {
  int type = (zside < 0) ? cassette : ((det == 0) ? (1 + positEE_[cassette - 1]) : (1 + positHE_[cassette - 1]));
  return type;
}
