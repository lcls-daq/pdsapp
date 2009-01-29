#include "CameraFexConfig.hh"

using namespace Pds;

CameraPixelCoord& CameraFexConfig::maskedPixelCoord(unsigned index)
{
  return reinterpret_cast<CameraPixelCoord*>(this+1)[index];
}

const CameraPixelCoord& CameraFexConfig::maskedPixelCoord(unsigned index) const
{
  return reinterpret_cast<const CameraPixelCoord*>(this+1)[index];
}

unsigned CameraFexConfig::size() const
{
  return sizeof(this) + nMaskedPixels*sizeof(CameraPixelCoord);
}

static const char* _title[] = { "RawImage",
				"ROI",
				"Simple2DGss",
				"ROI2DGss",
				"ROI2DGssAndImage",
				"Sink",
				NULL };

const char* CameraFexConfig::algorithm_title(Algorithm alg)
{
  return _title[alg];
}
