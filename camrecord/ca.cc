#include"pdsdata/xtc/Xtc.hh"
#include"pdsdata/xtc/TimeStamp.hh"
#include"pdsdata/xtc/DetInfo.hh"
#include"pdsdata/xtc/ProcInfo.hh"
#include"pdsdata/psddl/pulnix.ddl.h"
#include"pdsdata/psddl/opal1k.ddl.h"
#include"pdsdata/psddl/orca.ddl.h"
#include"pdsdata/psddl/camera.ddl.h"
#include"pdsdata/psddl/lusi.ddl.h"
#include"pdsdata/psddl/epics.ddl.h"
#include"pdsdata/psddl/generic1d.ddl.h"
#include"yagxtc.hh"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<vector>   //for std::vector

#include"cadef.h"
#include"alarm.h"

/*
 * Better to include this from EPICS menuFtype.h, but... it's complicated.
 * This has unfortunately changed on the way to epics7... and in a not
 * backwards compatible way, since INT64 and UINT64 were added before
 * float.  So... we cheat.  No recording of int64/uint64 for us, but we'll
 * be able to do the right thing for floats and doubles.
 */
typedef enum {
	menuFtypeSTRING,
	menuFtypeCHAR,
	menuFtypeUCHAR,
	menuFtypeSHORT,
	menuFtypeUSHORT,
	menuFtypeLONG,
	menuFtypeULONG,
	menuFtypeFLOAT,
	menuFtypeDOUBLE,
	menuFtypeFLOAT7,
	menuFtypeDOUBLE7,
}menuFtype;


using namespace std;
using namespace Pds;

using Pds::Epics::EpicsPvCtrlHeader;
using Pds::Epics::EpicsPvHeader;

typedef Pds::Generic1D::ConfigV0 G1DCfg;

// For the wave8
#define W8_NCHAN 16
static uint32_t W8_SampleType[W8_NCHAN]= {G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16,G1DCfg::UINT16, G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32,G1DCfg::UINT32};
static double W8_Period[W8_NCHAN]= {8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 8e-9, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7, 2e-7};

// For the qadc
#define QADC_NCHAN       4
#define QADC_BASERATE    1.25e9
#define QADC134_NCHAN    2
#define QADC134_BASERATE (3.2e9*13/14)

static void connection_handler(struct connection_handler_args args);

static DetInfo::Detector find_detector(const char *name, int &version)
{
    int i;
    const char *s = index(name, '-');
    unsigned int n;
    if (s != NULL) {
        n = (s - name);
        version = atoi(s + 1);
    } else {
        n = strlen(name);
    }
    for (i = 0; i != (int)DetInfo::NumDetector; i++) {
        s = DetInfo::name((DetInfo::Detector)i);
        if (strlen(s) == n && !strncmp(name, s, n))
            break;
    }
    return (DetInfo::Detector)i;
}

DetInfo::Device find_device(const char *name, int &version)
{
    int i;
    const char *s = index(name, '-');
    unsigned int n;
    if (s != NULL) {
        n = (s - name);
        version = atoi(s + 1);
    } else {
        n = strlen(name);
    }
    for (i = 0; i != (int)DetInfo::NumDevice; i++) {
        s = DetInfo::name((DetInfo::Device)i);
        if (strlen(s) == n && !strncmp(name, s, n))
            break;
    }
    return (DetInfo::Device)i;
}

static int read_ca(char *name, void *value, int dbrtype)
{
    int result;
    chid chan;

    result = ca_create_channel (name, 0, name, 50, &chan);
    if (result != ECA_NORMAL) {
        fprintf(stderr, "CA error %s while creating channel to %s!\n", ca_message(result), name);
        return 1;
    }
    result = ca_pend_io(10.0);
    if (result != ECA_NORMAL) {
        fprintf(stderr, "CA error %s while connecting to %s!\n", ca_message(result), name);
        return 1;
    }
    result = ca_array_get(dbrtype, 1, chan, value);
    if (result != ECA_NORMAL) {
        fprintf(stderr, "CA error %s while reading %s!\n", ca_message(result), name);
        return 1;
    }
    result = ca_pend_io(10.0);
    if (result != ECA_NORMAL) {
        fprintf(stderr, "CA error %s while waiting for read of %s!\n", ca_message(result), name);
        return 1;
    }
    return 0;
}

static struct generic1dinfo {
    int      ftype;
    uint32_t stype;
    int      size;
    chtype   dbf;
} g1dinfo[] = {
    { menuFtypeCHAR,    G1DCfg::UINT8,   1, DBF_CHAR },
    { menuFtypeUCHAR,   G1DCfg::UINT8,   1, DBF_CHAR },
    { menuFtypeSHORT,   G1DCfg::UINT16,  2, DBF_SHORT },
    { menuFtypeUSHORT,  G1DCfg::UINT16,  2, DBF_SHORT },
    { menuFtypeLONG,    G1DCfg::UINT32,  4, DBF_LONG },
    { menuFtypeULONG,   G1DCfg::UINT32,  4, DBF_LONG },
    { menuFtypeFLOAT,   G1DCfg::FLOAT32, 4, DBF_FLOAT },
    { menuFtypeFLOAT7,  G1DCfg::FLOAT32, 4, DBF_FLOAT },
    { menuFtypeDOUBLE,  G1DCfg::FLOAT64, 8, DBF_DOUBLE },
    { menuFtypeDOUBLE7, G1DCfg::FLOAT64, 8, DBF_DOUBLE },
    { -1, 0, 0, 0 }
};

static int get_generic1d_type(int kind, uint32_t *stype, int *size, chtype *dbf) 
{
    for (int i = 0; g1dinfo[i].ftype >= 0; i++) {
	if (kind == g1dinfo[i].ftype) {
	    *stype = g1dinfo[i].stype;
	    *size = g1dinfo[i].size;
	    *dbf = g1dinfo[i].dbf;
	    return 0;
	}
    }
    return 1;
}

/*
 * The logic for exactly how we record a PV into the XTC file is a little bit
 * squirrelly, and partially depends on the device type passed in and partially
 * depends on the PV name.
 *
 * If the device type is EpicsArch, we assume the data is small and not critical and
 * should just be put into the standard EpicsArch.  Otherwise, the data is either a
 * camera or some type of arbitrary waveform.
 * 
 * Next, we look at the PV name.  We assume we have a camera if the PV name either
 * ends ":ArrayData" (for Area Detector cameras), ".IRAW" (for a Dehong framegrabber
 * camera), ":LIVE_IMAGE_FAST" (for a unixCam camera), ":IMAGE_CMPX" (a unixCam with
 * an ROI), or ":WF" (the NFOV camera).  Many of these are historical and can probably
 * be removed.
 *
 * If we don't have a known camera, we assume we have a waveform.  Two special types
 * of waveform are understood: ":RAWDATA" denotes a QADC stream, and ":RAW" denotes wave8.
 * Otherwise, we assume we have a single waveform that does not require any special
 * unpacking.
 */
class caconn {
 public:
    caconn(string _name, string _det, string _camtype, string _pvname,
           int _flags, int _strict)
        : name(_name), detector(_det), camtype(_camtype), pvname(_pvname),
          connected(0), nelem(-1), event(0), flags(_flags), strict(_strict), force_dbf(DBF_LONG) {
        const char       *ds  = detector.c_str();
        int               detversion = 0;
        DetInfo::Detector det = find_detector(ds, detversion);
        int               devversion = 0;
        DetInfo::Device   dev = find_device(camtype.c_str(), devversion);
        char             *buf = NULL;
        char             *bf2 = NULL;
        Xtc              *cfg = NULL;
        Xtc              *frm = NULL;
        Camera::FrameV1  *f   = NULL;
        unsigned int     w = 0, h = 0, d = 0;
        double           gain = -1;
        double           exposure = -1;
        int              data_size = 0;
        uint32_t         nsamp[W8_NCHAN];
        uint32_t         stype[W8_NCHAN];
        int32_t          offset[W8_NCHAN];
        double           period[W8_NCHAN];
        char             model[40] = "Unknown";
        char             manufacturer[40] = "Unknown";

        if (det == DetInfo::NumDetector || (det != DetInfo::EpicsArch && dev == DetInfo::NumDevice)) {
            printf("Cannot find (%s, %s) values!\n", detector.c_str(), camtype.c_str());
            fprintf(stderr, "error Bad configuration: Cannot identify detector %s %s.\n",
                    detector.c_str(), camtype.c_str());
            exit(1);
        }

        num = conns.size();
        conns.push_back(this);

	is_epicsarch = 0;
        is_cam = 0;
        is_wv8 = 0;
        is_qadc = 0;
        is_qadc134 = 0;
        is_wf = 0;

        if (det == DetInfo::EpicsArch) {
            DetInfo sourceInfo(getpid(), DetInfo::EpicsArch, 0, DetInfo::NoDevice, streamno);
            
            caid = nxtcaid++;
            register_pv_alias(name, caid, sourceInfo);
            is_epicsarch = 1;
            int result = ca_create_channel(pvname.c_str(),
                                           connection_handler,
                                           this,
                                           50, /* priority?!? */
                                           &chan);
            if (result != ECA_NORMAL) {
                const char *s = ca_message(result);
                printf("CA error %s while creating channel to %s!\n",
                        s, pvname.c_str());
                switch (pvignore) {
                case 0:
                    fprintf(stderr, "error CA error %s for %s.\n", s, pvname.c_str());
                    exit(0);
                    break;
                case 1:
                    fprintf(stderr, "warn CA error %s for %s.\n", s, pvname.c_str());
                    break;
                case 2:
                    break;
                }
            }
            ca_poll();
            // If it's in EpicsArch, we assume it's small, not critical, and not a camera.
            xid = register_xtc(strict, name, 0, 0, 0);
            // Do this for non-cameras here
            if (write_hdf) register_hdf_writer(xid, register_hdf(name, 0, 0, 0, nelem, dbrtype, 0, 0));
            return;
        }

        cmask = 0;

        /* Retrieve the camera parameters based on the PV name. */
        char pv[256];
        strcpy(pv, pvname.c_str());
        int pvlen = strlen(pv);
        if (!strcmp(pv + pvlen - 8, ":RAWDATA")) {
            /* NOT a camera, a QADC!! */
	    double p;
            is_qadc = 1;
            pvlen -= 8;
            strcpy(pv + pvlen, ":LENGTH");      // Samples per channel!
            read_ca(pv, &qadc_len, DBR_LONG);
            strcpy(pv + pvlen, ":INTERLEAVE");
            read_ca(pv, &qadc_inter, DBR_LONG);
	    if (qadc_inter) {
		nchan = 1;
		p = (1.0 / QADC_NCHAN) / QADC_BASERATE;
	    } else {
		nchan = QADC_NCHAN;
		p = 1.0 / QADC_BASERATE;
	    }
            for (int i = 0; i < nchan; i++) {
                period[i] = p;
                offset[i] = 0;
                stype[i] = G1DCfg::FLOAT64;
                nsamp[i] = qadc_len;
            }
            qadc_len *= nchan;                    // Total Samples!
            data_size = qadc_len * 8;             // We save the data here as doubles...
            nelem = qadc_len / 2 + 8 + 4 * nchan; // ... but we get it as shorts packed into ints
                                                  // with an 8-word header and a 4-word header per
	                                          // channel.
            qadc_data = (double *) calloc(1, data_size);
        } else if (!strcmp(pv + pvlen - 9, ":RAWDATA0") || !strcmp(pv + pvlen - 9, ":RAWDATA1")) {
            /* NOT a camera, a QADC134!! */
            int prescale;
	    int full, sparse;
	    int lo_thresh, hi_thresh;
	    double p;
            is_qadc134 = 1;
            pvlen -= 9;
            strcpy(pv + pvlen, ":LENGTH_RBV");      // Samples per channel!
            read_ca(pv, &qadc_len, DBR_LONG);
            strcpy(pv + pvlen, ":INTERLEAVE_RBV");
            read_ca(pv, &qadc_inter, DBR_LONG);
            strcpy(pv + pvlen, ":PRESCALE_RBV");
            read_ca(pv, &prescale, DBR_LONG);
            strcpy(pv + pvlen, ":LO_THRESH_RAW_RBV");
            read_ca(pv, &lo_thresh, DBR_LONG);
            strcpy(pv + pvlen, ":HI_THRESH_RAW_RBV");
            read_ca(pv, &hi_thresh, DBR_LONG);
	    qadc_fill = ((lo_thresh + hi_thresh) / 2.0 - 2048.) / 4096. * 1.3;
            strcpy(pv + pvlen, ":FULL_EN_RBV");
            read_ca(pv, &full, DBR_LONG);
	    if (full)
		full = 1;
            strcpy(pv + pvlen, ":SPARSE_EN_RBV");
            read_ca(pv, &sparse, DBR_LONG);
	    if (sparse)
		sparse = 1;
            nchan = qadc_inter ? (full+sparse) : 2;
	    if (qadc_inter) {
		nchan = 1;
		p = (1.0 / QADC134_NCHAN) / QADC134_BASERATE;
	    } else {
		nchan = QADC134_NCHAN;
		p = 1.0 / QADC134_BASERATE;
	    }
	    if (prescale)
		p *= prescale;
	    else
		p *= 4096;
            for (int i = 0; i < nchan; i++) {
		period[i] = p;
                offset[i] = 0;
                stype[i] = G1DCfg::FLOAT64;
                nsamp[i] = qadc_len;
            }
            qadc_len *= nchan;                    // Total Samples!
            data_size = qadc_len * 8;             // We save the data here as doubles...
            nelem = qadc_len / 2 + 8 + 4 * nchan; // ... but we get it as shorts packed into ints
                                                  // with an 8-word header and a 4-word header per
	                                          // channel. NOTE: This is a maximum, since
	                                          // sparcified data is, well, sparse!
            qadc_data = (double *) calloc(1, data_size);
        } else if (!strcmp(pv + pvlen - 4, ":RAW")) {
            /* NOT a camera, a wave8!! */
            is_wv8 = 1;
            nchan = W8_NCHAN;
            pvlen -= 4;
            nelem = 15; /* Header + Footer */
            strcpy(pv + pvlen, ":ChanEnable_RBV");
            read_ca(pv, &cmask, DBR_LONG);
            printf("cmask = 0x%x\n", cmask);
            for (int i = 0; i < nchan; i++) {
                if (cmask & (1 << i)) {
                    sprintf(pv + pvlen, ":NumberOfSamples%d_RBV", i);
                    read_ca(pv, &nsamp[i], DBR_LONG);
                    sprintf(pv + pvlen, ":Delay%d_RBV", i);
                    read_ca(pv, &offset[i], DBR_LONG);
                    nelem += 1 + ((i < 8) ? (nsamp[i] / 2) : nsamp[i]);
                    data_size += ((i < 8) ? 2 : 4) * nsamp[i];
                } else {
                    nsamp[i] = 0;
                    offset[i] = 0;
                }
            }
        } else if (!strcmp(pv + pvlen - 10, ":ArrayData")) {
            /* An area detector camera */
            is_cam = 1;
            pvlen -= 10;
            strcpy(pv + pvlen, ":ArraySize0_RBV");
            read_ca(pv, &w, DBR_LONG);
            strcpy(pv + pvlen, ":ArraySize1_RBV");
            read_ca(pv, &h, DBR_LONG);
            strcpy(pv + pvlen, ":BitsPerPixel_RBV");
            if (read_ca(pv, &d, DBR_LONG)) {
                /* Sigh.  It could be either way... */
                strcpy(pv + pvlen, ":BIT_DEPTH");
                read_ca(pv, &d, DBR_LONG);
            }
            while (--pvlen > 0 && pv[pvlen] != ':');
            strcpy(pv + pvlen, ":Gain_RBV");
            read_ca(pv, &gain, DBR_DOUBLE);
            strcpy(pv + pvlen, ":AcquireTime_RBV");
            read_ca(pv, &exposure, DBR_DOUBLE);
            strcpy(pv + pvlen, ":Model_RBV");
            read_ca(pv, model, DBR_STRING);
            strcpy(pv + pvlen, ":Manufacturer_RBV");
            read_ca(pv, manufacturer, DBR_STRING);
        } else if (!strcmp(pv + pvlen - 5, ".IRAW")) {
            /* A Dehong framegrabber camera */
            is_cam = 1;
            pvlen -= 5;
            strcpy(pv + pvlen, ".NCOL");
            read_ca(pv, &w, DBR_LONG);
            strcpy(pv + pvlen, ".NROW");
            read_ca(pv, &h, DBR_LONG);
            strcpy(pv + pvlen, ".NBIT");
            read_ca(pv, &d, DBR_LONG);
        } else if (!strcmp(pv + pvlen - 16, ":LIVE_IMAGE_FAST")) {
            /* A unixCam camera */
            is_cam = 1;
            pvlen -= 16;
            strcpy(pv + pvlen, ":N_OF_COL");
            read_ca(pv, &w, DBR_LONG);
            strcpy(pv + pvlen, ":N_OF_ROW");
            read_ca(pv, &h, DBR_LONG);
            strcpy(pv + pvlen, ":N_OF_BITS");
            read_ca(pv, &d, DBR_LONG);
            strcpy(pv + pvlen, ":Gain");
            read_ca(pv, &gain, DBR_DOUBLE);
            strcpy(pv + pvlen, ":Model");
            read_ca(pv, model, DBR_STRING);
            strcpy(pv + pvlen, ":ID");
            read_ca(pv, manufacturer, DBR_STRING);
        } else if (!strcmp(pv + pvlen - 11, ":IMAGE_CMPX")) {
            /* A unixCam camera with ROI */
            is_cam = 1;
            pvlen -= 11;
            strcpy(pv + pvlen, ":ROI_XNP");
            read_ca(pv, &w, DBR_LONG);
            strcpy(pv + pvlen, ":ROI_YNP");
            read_ca(pv, &h, DBR_LONG);
            strcpy(pv + pvlen, ":ROI_BITS");
            read_ca(pv, &d, DBR_LONG);
            strcpy(pv + pvlen, ":Gain");
            read_ca(pv, &gain, DBR_DOUBLE);
            strcpy(pv + pvlen, ":Model");
            read_ca(pv, model, DBR_STRING);
            strcpy(pv + pvlen, ":ID");
            read_ca(pv, manufacturer, DBR_STRING);
        } else if (!strcmp(pv + pvlen - 3, ":WF")) {
            /* The NFOV.  Does anything else do this?!? */
            is_cam = 1;
            pvlen -= 3;
            strcpy(pv + pvlen, ":RawNx");
            read_ca(pv, &w, DBR_LONG);
            strcpy(pv + pvlen, ":RawNy");
            read_ca(pv, &h, DBR_LONG);
            d = 16;  /* This isn't in a PV, boo hiss! */
            strcpy(pv + pvlen, ":CamModel");
            read_ca(pv, model, DBR_STRING);
            strcpy(pv + pvlen, ":CamMaker");
            read_ca(pv, manufacturer, DBR_STRING);
        } else {
            /*
	     * We assume this is a single waveform.  We don't know anything else yet.
	     *
	     * Let's further assume it's either a waveform record (so we can read
	     * NORD and FTVL) or it's an aSub (so we can read NEA/FTA for A, and 
	     * NEVA/FTVA for VALA).
	     */
            is_wf = 1;
	    nchan = 1;
	    period[0] = 1.0;  /* We'll never know this! */
	    offset[0] = 0;
	    char *dot = ::index(pv, '.');
	    int kind;
	    if (dot) {
		char last = pv[pvlen - 1];
		if (strlen(++dot) == 1) {   /* A-Z */
		    sprintf(dot, "NE%c", last);
		    read_ca(pv, &nsamp[0], DBR_LONG);
		    sprintf(dot, "FT%c", last);
		    read_ca(pv, &kind, DBR_LONG);
		} else {                    /* VALA-VALZ */
		    sprintf(dot, "NEV%c", last);
		    read_ca(pv, &nsamp[0], DBR_LONG);
		    sprintf(dot, "FTV%c", last);
		    read_ca(pv, &kind, DBR_LONG);
		}
	    } else {
		strcpy(pv + pvlen, ".NORD");
		read_ca(pv, &nsamp[0], DBR_LONG);
		strcpy(pv + pvlen, ".FTVL");
		read_ca(pv, &kind, DBR_LONG);
	    }
	    if (get_generic1d_type(kind, stype, &data_size, &force_dbf)) {
		fprintf(stderr, "warn Cannot find waveform information for PV %s (%d)!\n", pv, kind);
		printf("Cannot find waveform information for PV %s (%d)!\n", pv, kind);
		return;
	    }
	    data_size *= nsamp[0];
        }

        if (is_cam) {
            if (CAMERA_DEPTH(flags))
                d = CAMERA_DEPTH(flags);
	    // Sigh.  psana doesn't listen to us.  If the bit depth is 8, it assumes char,
	    // and if it's 16, it assumes short.  So we must obey our computer overlords.
	    if (d <= 8)
		force_dbf = DBF_CHAR;
	    else if (d <= 16)
		force_dbf = DBF_SHORT;
            nelem = w * h;
        }

        int               pid = getpid(), size;
        DetInfo sourceInfo(pid, det, detversion, dev, devversion);
        Camera::FrameCoord origin(0, 0);
        Camera::FrameCoord bottom(w, h);

        register_alias(name, sourceInfo);

        switch (dev) {
        case DetInfo::TM6740:
            size = 2 * sizeof(Xtc) + sizeof(Pulnix::TM6740ConfigV2) +
                sizeof(Camera::FrameFexConfigV1);
            buf = (char *) calloc(1, size);
            cfg = new (buf) Xtc(TypeId(TypeId::Id_TM6740Config, 2), sourceInfo);
            // Fake a configuration!
            // black_level_a = black_level_b = 32, gaina = gainb = 0x1e8 (max, min is 0x42),
            // gain_balance = false, Depth = 10 bit, hbinning = vbinning = x1, LookupTable = linear.
            new ((void *)cfg->alloc(sizeof(Pulnix::TM6740ConfigV2)))
                Pulnix::TM6740ConfigV2(32, 32, 0x1e8, 0x1e8, false,
                                       Pulnix::TM6740ConfigV2::Ten_bit,
                                       Pulnix::TM6740ConfigV2::x1,
                                       Pulnix::TM6740ConfigV2::x1,
                                       Pulnix::TM6740ConfigV2::Linear);
            break;
        case DetInfo::Opal1000:
            size = 2 * sizeof(Xtc) + sizeof(Opal1k::ConfigV1) +
                sizeof(Camera::FrameFexConfigV1);
            buf = (char *) calloc(1, size);
            cfg = new (buf) Xtc(TypeId(TypeId::Id_Opal1kConfig, 1), sourceInfo);
            // Fake a configuration!
            // black = 32, gain = 100, Depth = 12 bit, binning = x1, mirroring = None,
            // vertical_remap = true, enable_pixel_creation = false.
            new ((void *)cfg->alloc(sizeof(Opal1k::ConfigV1)))
                Opal1k::ConfigV1(32, 100, Opal1k::ConfigV1::Twelve_bit, Opal1k::ConfigV1::x1,
                                 Opal1k::ConfigV1::None, true, false, false, 0, 0, 0);
            break;
        case DetInfo::OrcaFl40:
            size = 2 * sizeof(Xtc) + sizeof(Orca::ConfigV1) +
                sizeof(Camera::FrameFexConfigV1);
            buf = (char *) calloc(1, size);
            cfg = new (buf) Xtc(TypeId(TypeId::Id_OrcaConfig, 1), sourceInfo);
            // Fake a configuration!
            // readoutmode = Subarray, cooling = off, defect pixel correction = on, and rows = h.
            new ((void *)cfg->alloc(sizeof(Orca::ConfigV1)))
                Orca::ConfigV1(Orca::ConfigV1::Subarray, Orca::ConfigV1::Off, 1, h);
            break;
        case DetInfo::ControlsCamera:
            size = 2 * sizeof(Xtc) + sizeof(Camera::ControlsCameraConfigV1) +
                sizeof(Camera::FrameFexConfigV1);
            buf = (char *) calloc(1, size);
            cfg = new (buf) Xtc(TypeId(TypeId::Id_ControlsCameraConfig, 1), sourceInfo);
            // Wow, an *actual* configuration!!
            new ((void *)cfg->alloc(sizeof(Camera::ControlsCameraConfigV1)))
                Camera::ControlsCameraConfigV1(w, h, d, Camera::ControlsCameraConfigV1::Mono,
                                               exposure, gain, manufacturer, model);
            break;
        default:
	    if (is_cam) {
		fprintf(stderr, "warn Unknown camera type for PV %s!\n", pv);
		return;
	    }
            // So now we either have a wave8, a qadc, or a generic waveform.
            size = sizeof(Xtc) + sizeof(Generic1D::ConfigV0) + nchan * (3 * sizeof(int32_t) + sizeof(double));
            buf = (char *) calloc(1, size);
            cfg = new (buf) Xtc(TypeId(TypeId::Id_Generic1DConfig, 0), sourceInfo);
            if (is_wv8)
                new ((void *)cfg->alloc(size - sizeof(Xtc)))
                    Generic1D::ConfigV0(nchan, nsamp, W8_SampleType, offset, W8_Period);
            else
                new ((void *)cfg->alloc(size - sizeof(Xtc)))
                    Generic1D::ConfigV0(nchan, nsamp, stype, offset, period);
            break;
        }
        if (is_cam) {
            // This is on every camera.
            cfg = new ((char *) cfg->next()) Xtc(TypeId(TypeId::Id_FrameFexConfig, 1), sourceInfo);
            new ((void *)cfg->alloc(sizeof(Camera::FrameFexConfigV1)))
                Camera::FrameFexConfigV1(Camera::FrameFexConfigV1::FullFrame, 1,
                                         Camera::FrameFexConfigV1::NoProcessing, origin, bottom, 0, 0, 0);

            // setup hdf5 datasets
	    xid = register_xtc(strict, name, 1, 1, 1);
            if (write_hdf) {
                register_hdf_writer(xid, register_hdf(name, is_cam, w, h, nelem, dbrtype, 0, 0));
            }
            configure_xtc(xid, buf, size, 0, 0);

            hdrlen = sizeof(Xtc) + sizeof(Camera::FrameV1);
            bf2 = (char *) calloc(1, hdrlen);
            frm = hdr = new (bf2) Xtc(TypeId(TypeId::Id_Frame, 1), sourceInfo);

            f = new ((char *)frm->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(w, h, d, 32);

            frm->alloc(f->_sizeof()-sizeof(*f));
            free(buf);
        } else {
	    xid = register_xtc(strict, name, 1, 1, 0);

            /* MCB - Dan, add HDF for wave8 here? */
            configure_xtc(xid, buf, size, 0, 0);

            hdrlen = sizeof(Xtc) + sizeof(Generic1D::DataV0);
            bf2 = (char *) calloc(1, hdrlen);
            frm = hdr = new (bf2) Xtc(TypeId(TypeId::Id_Generic1DData, 0), sourceInfo);

            uint32_t *dataptr = 
                reinterpret_cast<uint32_t*>
                    (new ((char *)frm->alloc(sizeof(Generic1D::DataV0) + data_size))
                         Generic1D::DataV0);
            *dataptr = data_size;
            free(buf);
        }

        int result = ca_create_channel(pvname.c_str(),
                                       connection_handler,
                                       this,
                                       50, /* priority?!? */
                                       &chan);
        if (result != ECA_NORMAL) {
            const char *s = ca_message(result);
            printf("CA error %s while creating channel to %s!\n",
                    s, pvname.c_str());
            fprintf(stderr, "error CA error %s while creating %s.\n", 
                    s, pvname.c_str());
            exit(0);
        }
        ca_poll();
    };
    static caconn *find(chid chan) {
        int i = conns.size() - 1;
        while (i >= 0) {
            if (conns[i]->chan == chan)
                return conns[i];
            i--;
        }
        return NULL;
    }
    static caconn *index(int idx) {
        if (idx >= 0 && idx < (int) conns.size())
            return conns[idx];
        else
            return NULL;
    }
    void disconnect(void) {
        if (event)
            ca_clear_subscription(event);
    }
    const char *getpvname() {
        return pvname.c_str();
    }
    const char *getname() {
        return name.c_str();
    }
 private:
    static vector<caconn *> conns;
    static int nxtcaid;
    string name;
    string detector;
    string camtype;
    string pvname;
 public:
    int    num;
    int    connected;
    int    nelem;
    long   dbrtype;
    int    size;
    int    status;
    chtype dbftype;
    evid event;
    int    xid;
    Xtc   *hdr;
    int    hdrlen;
    int    flags;
    int    strict;
    int    is_epicsarch;
    int    is_cam;
    int    is_wv8;
    int    is_wf;
    int    is_qadc;
    int    is_qadc134;
    double qadc_fill;
    int    nchan;
    int    qadc_inter;
    int    qadc_len;
    double *qadc_data;
    int32_t cmask;
    int    caid;
    chid   chan;
    chtype force_dbf;
};

vector<caconn *> caconn::conns;
int caconn::nxtcaid = 0;

static void event_handler(struct event_handler_args args)
{
    caconn *c = (caconn *)args.usr;

    if (args.status != ECA_NORMAL) {
        printf("Bad status: %d\n", args.status);
    } else if (args.type == c->dbrtype && args.count == c->nelem) {
        if (c->is_qadc) {
            struct dbr_time_long *d = (struct dbr_time_long *) args.dbr;
            int32_t *in32 = (int32_t *)(&d->value + 8);
	    double *data = c->qadc_data;
            for (int i = 0; i < c->nchan; i++) {
		int len = in32[0] & 0x3fffffff;
		int16_t *in = (int16_t *)(in32 + 4);
		for (int j = 0; j < len; j++) {
		    *data++ = (*in++ - 512.) / 2048.; /* Scale the shorts into doubles! */
		}
		in32 = (int32_t *)in;
	    }
            data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
                     c->hdr, c->hdrlen, c->qadc_data);
        } else if (c->is_qadc134) {
            struct dbr_time_long *d = (struct dbr_time_long *) args.dbr;
            int32_t *in32 = (int32_t *)(&d->value + 8);
	    double *data = c->qadc_data;
            for (int i = 0; i < c->nchan; i++) {
		int len = in32[0] & 0x3fffffff;
		int16_t *in = (int16_t *)(in32 + 4);
		for (int j = 0; j < len; j++) {
		    if (*in & 0x8000) {
			for (int k = 0; k < (*in & 0x7fff); k++) /* Unsparcify! */
			    *data++ = c->qadc_fill;
			in++;
		    } else
			*data++ = (*in++ - 2048.) / 4096. * 1.3;/* Scale the shorts into doubles! */
		}
		in32 = (int32_t *)in;
	    }
            data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
                     c->hdr, c->hdrlen, c->qadc_data);
        } else if (c->is_wv8) {
            struct dbr_time_long *d = (struct dbr_time_long *) args.dbr;
            int32_t *in = &d->value + 14;
            int32_t *out = &d->value;
            
            /* Strip out all of the headers!! */
            for (int i = 0; i < 16; i++) {
                if (c->cmask & (1 << i)) {
                    int len = ((*in++ >> 8) & 0x1fff) + 1;
                    if (i < 8)
                        len /= 2;
                    while (len--) {
                        *out++ = *in++;
                    }
                }
            }

            data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
                     c->hdr, c->hdrlen, &d->value);
        } else if (!c->is_epicsarch) {
	    /* Sigh, we can't cheat here, since we want to dereference the value field, not
	       just grab the timestamp from the start and record the entire dbr_time_* record! */
	    switch (args.type) {
	    case DBR_TIME_CHAR: {
		struct dbr_time_char *d = (struct dbr_time_char *) args.dbr;
		data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
			 c->hdr, c->hdrlen, &d->value);
		break;
	    }
	    case DBR_TIME_SHORT: {
		struct dbr_time_short *d = (struct dbr_time_short *) args.dbr;
		data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
			 c->hdr, c->hdrlen, &d->value);
		break;
	    }
	    case DBR_TIME_LONG: {
		struct dbr_time_long *d = (struct dbr_time_long *) args.dbr;
		data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
			 c->hdr, c->hdrlen, &d->value);
		break;
	    }
	    case DBR_TIME_FLOAT: {
		struct dbr_time_float *d = (struct dbr_time_float *) args.dbr;
		data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
			 c->hdr, c->hdrlen, &d->value);
		break;
	    }
	    case DBR_TIME_DOUBLE: {
		struct dbr_time_double *d = (struct dbr_time_double *) args.dbr;
		data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
			 c->hdr, c->hdrlen, &d->value);
		break;
	    }
	    default:
		/* printf("Unknown type?!?\n"); */
		break;
	    }
        } else {
            struct dbr_time_short *d = (struct dbr_time_short *) args.dbr;
            /*
             * This is totally cheating, since we aren't looking at the actual dbr_time_* type.
             * But we know they all start status, severity, stamp, so...
             */
            data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
                     c->hdr, c->hdrlen, const_cast<void *>(args.dbr));
        }
    } else {
        printf("type = %ld, count = %ld -> expected type = %ld, count = %d\n",
                args.type, args.count, c->dbrtype, c->nelem);
    }
    fflush(stdout);
}

static void get_handler(struct event_handler_args args)
{
    caconn *c = (caconn *)args.usr;
    DetInfo sourceInfo(getpid(), DetInfo::EpicsArch, 0, DetInfo::NoDevice, streamno);
    int hdrsize = sizeof(EpicsPvCtrlHeader);
    int ctrlsize = dbr_size_n(args.type, args.count);

#if 0
    /* NO PADDING BETWEEN DATA STRUCTURES IN EpicsPvCtrl*! */
#endif

    char             *buf = (char *) calloc(1, sizeof(Xtc) + hdrsize + ctrlsize);
    Xtc              *cfg = new (buf) Xtc(TypeId(TypeId::Id_Epics, 1), sourceInfo);
    void             *h   = cfg->alloc(hdrsize);
    new (h) EpicsPvCtrlHeader(c->caid, args.type, args.count, c->getpvname());

    void             *buf2 = cfg->alloc(ctrlsize);
    memcpy(buf2, args.dbr, ctrlsize);
    configure_xtc(c->xid, (char *) cfg, cfg->extent, 0, 0);
    free(buf); /* delete ~cfg? */

    hdrsize = sizeof(EpicsPvHeader);
#if 1
    /* BUT THERE *IS* PADDING BETWEEN DATA STRUCTURES IN EpicsPvTime*! */
    if (hdrsize % 4)
        hdrsize += 4 - (hdrsize % 4); /* Sigh.  Padding in the middle! */
#endif
    c->hdrlen = sizeof(Xtc) + hdrsize;
    buf = (char *) calloc(1, c->hdrlen);
    c->hdr = new (buf) Xtc(TypeId(TypeId::Id_Epics, 1), sourceInfo);
    new ((char *)c->hdr->alloc(hdrsize)) EpicsPvHeader(c->caid, c->dbrtype, args.count);
    c->hdr->alloc(c->size); // This is in the data packet, not the header!

    int status = ca_create_subscription(c->dbrtype, c->nelem, c->chan, DBE_VALUE | DBE_ALARM,
                                        event_handler, (void *) c, &c->event);
    if (status != ECA_NORMAL) {
        printf("Failed to create subscription! error %d!\n", status);
        switch (c->is_epicsarch ? pvignore : 0) {
        case 0:
            fprintf(stderr, "error Cannot subscribe to %s (error %d).\n", c->getpvname(), status);
            exit(0);
            break;
        case 1:
            fprintf(stderr, "warn Cannot subscribe to %s (error %d).\n", c->getpvname(), status);
            break;
        case 2:
            break;
        }
    }
}

static void connection_handler(struct connection_handler_args args)
{
    caconn *c = caconn::find(args.chid);

    if (args.op == CA_OP_CONN_UP) {
        printf("%s (%s) is connected.\n", c->getname(), c->getpvname());
        c->connected = 1;
        if (c->nelem == -1)
            c->nelem = ca_element_count(args.chid);
        c->dbftype = ca_field_type(args.chid);
        if (c->is_wv8 || c->is_qadc) {
            printf("Forcing type to LONG!\n");
            c->dbftype = DBR_LONG;
            c->dbrtype = DBR_TIME_LONG;              /* Force this, since we know the wave8 and qadc are really uint32!!! */
        } else if (c->is_cam || c->is_wf) {
            c->dbftype = dbf_type_to_DBR(c->force_dbf);
            c->dbrtype = dbf_type_to_DBR_TIME(c->force_dbf);
	    printf("Using type %d for %s %s\n", (int) c->dbrtype, c->is_cam ? (char *)"camera" : (char *)"waveform", c->getname());
        } else
            c->dbrtype = dbf_type_to_DBR_TIME(c->dbftype);
        c->size = dbr_size_n(c->dbrtype, c->nelem);
        if (!c->is_epicsarch) {
            int status = ca_create_subscription(c->dbrtype, c->nelem, args.chid, DBE_VALUE | DBE_ALARM,
                                                event_handler, (void *) c, &c->event);
            if (status != ECA_NORMAL) {
                printf("Failed to create subscription! error %d!\n", status);
                fprintf(stderr, "error Cannot subscribe to %s (error %d).\n",
                        c->getpvname(), status);
                exit(0);
            }
        } else {
            int status = ca_array_get_callback(dbf_type_to_DBR_CTRL(c->dbftype), c->nelem, args.chid, get_handler, (void *) c);
            if (status != ECA_NORMAL) {
                printf("Get failed! error %d!\n", status);
                switch (pvignore) {
                case 0:
                    fprintf(stderr, "error Cannot get %s (error %d).\n",
                            c->getpvname(), status);
                    exit(0);
                    break;
                case 1:
                    fprintf(stderr, "warn Cannot get %s (error %d).\n",
                            c->getpvname(), status);
                    break;
                case 2:
                    break;
                }
            }
        }
    } else {
        if (c->connected)
            printf("%s (%s) has disconnected!\n", c->getname(), c->getpvname());
        else
            printf("%s unable to connect to PV (%s)!\n", c->getname(), c->getpvname());
        c->connected = 0;
    }
    fflush(stdout);
}

static void fd_handler(void *parg, int fd, int opened)
{
    if (opened)
        add_socket(fd);
    else
        remove_socket(fd);
}

void initialize_ca(void)
{
    ca_add_fd_registration(fd_handler, NULL);
    ca_context_create(ca_disable_preemptive_callback);
}

void create_ca(string name, string detector, string camtype, string pvname, int flags, int strict)
{
    new caconn(name, detector, camtype, pvname, flags, strict);
}

void handle_ca(fd_set *rfds)
{
    ca_poll();
}

void begin_run_ca(void)
{
    int i;
    caconn *c;

    for (i = 0, c = caconn::index(i); c; c = caconn::index(++i)) {
        if (!c->is_cam && !c->strict) {
            int status = ca_array_get_callback(c->dbrtype, c->nelem, c->chan, event_handler, (void *) c);
            if (status != ECA_NORMAL) {
                printf("Get failed! error %d!\n", status);
                switch (c->is_epicsarch ? pvignore : 0) {
                case 0:
                    fprintf(stderr, "error Cannot get %s (error %d).\n", c->getpvname(), status);
                    exit(0);
                    break;
                case 1:
                    fprintf(stderr, "warn Cannot get %s (error %d).\n", c->getpvname(), status);
                    break;
                case 2:
                    break;
                }
            }
        }
    }
}

void cleanup_ca(void)
{
    int i;
    caconn *c;

    for (i = 0, c = caconn::index(i); c; c = caconn::index(++i)) {
        c->disconnect();
    }

    ca_pend_io(0.0);
    ca_context_destroy();
}

char *connection_status(void)
{
    static char buf[1024];
    char *s = buf;
    int i, j;
    caconn *c;
    static int first = 1;

    for (i = 0, j=0, c = caconn::index(i); c; c = caconn::index(++i)) {
        j += c->connected;
    }
    // MCB - This is bad, this is wrong, this is evil...
    if (first) {
        j = i;
        first = 0;
    }

    sprintf(s, "cstat %d %d", i, j);
    
    return buf;
}
