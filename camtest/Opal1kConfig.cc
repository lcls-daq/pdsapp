#include "Opal1kConfig.hh"
#include "CameraPixelCoord.hh"

using namespace Pds;

const CameraPixelCoord& Opal1kConfig::DefectPixelCoord(unsigned index) const
{
  return reinterpret_cast<const CameraPixelCoord*>(this+1)[index];
}

unsigned Opal1kConfig::size() const
{
  return sizeof(*this)+nDefectPixels*sizeof(CameraPixelCoord);
}
