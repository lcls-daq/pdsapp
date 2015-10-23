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
#include"yagxtc.hh"

#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<vector>   //for std::vector

#include"cadef.h"
#include"alarm.h"

using namespace std;
using namespace Pds;

using Pds::Epics::EpicsPvCtrlHeader;
using Pds::Epics::EpicsPvHeader;

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

static int read_ca_long(char *name, unsigned int *value)
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
    result = ca_array_get(DBR_LONG, 1, chan, value);
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

static int get_image_size(const char *image, const char *xname, const char *yname, unsigned int *x, unsigned int *y)
{
    char buf[256], *s;

    strncpy(buf, image, sizeof(buf));
    if (!(s = rindex(buf, ':')))
        return 1;
    strcpy(s, xname);
    *x = 0;
    if (read_ca_long(buf, x))
        return 1;
    strcpy(s, yname);
    *y = 0;
    if (read_ca_long(buf, y))
        return 1;
    printf("get_image_size finds %u by %u.\n", *x, *y);
    return 0;
}

class caconn {
 public:
    caconn(string _name, string _det, string _camtype, string _pvname,
           int _binned, int _strict)
        : name(_name), detector(_det), camtype(_camtype), pvname(_pvname),
          connected(0), nelem(-1), event(0), binned(_binned), strict(_strict) {
        const char       *ds  = detector.c_str();
        int               bld = !strncmp(ds, "BLD-", 4);
        int               detversion = 0;
        DetInfo::Detector det = bld ? DetInfo::BldEb : find_detector(ds, detversion);
        int               devversion = 0;
        DetInfo::Device   ctp = find_device(camtype.c_str(), devversion);
        DetInfo::Device   dev = bld ? DetInfo::NoDevice : ctp;
        char             *buf = NULL;
        char             *bf2 = NULL;
        Xtc              *cfg = NULL;
        Xtc              *frm = NULL;
        Camera::FrameV1  *f   = NULL;
        unsigned int     w = 0, h = 0, d = 0;

        if (det == DetInfo::NumDetector || (det != DetInfo::EpicsArch && dev == DetInfo::NumDevice)) {
            printf("Cannot find (%s, %s) values!\n", detector.c_str(), camtype.c_str());
            exit(1);
        }
        bld = bld ? atoi(ds + 4) : -1;

        num = conns.size();
        conns.push_back(this);
        xid = register_xtc(strict, name, det != DetInfo::EpicsArch,
                           det != DetInfo::EpicsArch); // PVs -> not critical or big
                                                       // cameras -> critical and big
        if (det == DetInfo::EpicsArch) {
            DetInfo sourceInfo(getpid(), DetInfo::EpicsArch, 0, DetInfo::NoDevice, streamno);
            
            caid = nxtcaid++;
            register_pv_alias(name, caid, sourceInfo);
            is_cam = 0;
            int result = ca_create_channel(pvname.c_str(),
                                           connection_handler,
                                           this,
                                           50, /* priority?!? */
                                           &chan);
            if (result != ECA_NORMAL) {
                printf("CA error %s while creating channel to %s!\n",
                        ca_message(result), pvname.c_str());
                exit(0);
            }
            ca_poll();
            return;
        } else
            is_cam = 1;

        /* Set default camera size */
        switch (ctp) {
        case DetInfo::TM6740:
            w = 640;
            h = 480;
            d = 10;
            break;
        case DetInfo::Opal1000:
            w = 1024;
            h = 1024;
            d = 12;
            break;
        case DetInfo::OrcaFl40:
            w = 2048;
            h = 2048;
            d = 16;
            break;
        default:
            /* This is bad. We should probably print something. */
            exit(0);
            break;
        }

        /* Now find the real camera size. */
        unsigned int hh, ww;
        switch (binned) {
        case CAMERA_NONE:
            break;
        case CAMERA_BINNED:
            switch (ctp) {
            case DetInfo::TM6740:
                w = 320;
                h = 240;
                d = 8;
                break;
            case DetInfo::Opal1000:
                w = 1024;
                h = 256;
                break;
            case DetInfo::OrcaFl40:
                /* MCB ??? */
                w = 2048;
                h = 2048;
                break;
            default:
                break;
            }
            break;
        case CAMERA_ROI:
            if (!get_image_size(pvname.c_str(), ":ROI_XNP", ":ROI_YNP", &ww, &hh)) {
                w = ww;
                h = hh;
            } else {
                /* Print an error? */
            }
            break;
        case CAMERA_SIZE:
            if (!get_image_size(pvname.c_str(), ":N_OF_COL", ":N_OF_ROW", &ww, &hh)) {
                w = ww;
                h = hh;
            } else {
                /* Print an error? */
            }
            break;
        case CAMERA_ADET:
            if (!get_image_size(pvname.c_str(), ":ArraySize0_RBV", ":ArraySize1_RBV", &ww, &hh)) {
                w = ww;
                h = hh;
            } else {
                /* Print an error? */
            }
            break;
        }
        nelem = w * h;

        int               pid = getpid(), size;
        DetInfo sourceInfo(pid, det, detversion, dev, devversion);
        ProcInfo bldInfo(Level::Reporter, pid, bld);
        Camera::FrameCoord origin(0, 0);

        if (bld < 0)
            register_alias(name, sourceInfo);

        switch (ctp) {
        case DetInfo::TM6740:
            size = ((bld < 0) ? 2 : 3) * sizeof(Xtc) + sizeof(Pulnix::TM6740ConfigV2) +
                sizeof(Camera::FrameFexConfigV1);
            buf = (char *) calloc(1, size);
            if (bld < 0) {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_TM6740Config, 2), sourceInfo);
            } else {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_Xtc, 1), sourceInfo);
                cfg = new ((char *)cfg->alloc(size - sizeof(Xtc)))
                          Xtc(TypeId(TypeId::Id_TM6740Config, 2), bldInfo);
            }
            // Fake a configuration!
            // black_level_a = black_level_b = 32, gaina = gainb = 0x1e8 (max, min is 0x42),
            // gain_balance = false, Depth = 10 bit, hbinning = vbinning = x1, LookupTable = linear.
            if (binned == CAMERA_BINNED)
                new ((void *)cfg->alloc(sizeof(Pulnix::TM6740ConfigV2)))
                    Pulnix::TM6740ConfigV2(32, 32, 0x1e8, 0x1e8, false,
                                           Pulnix::TM6740ConfigV2::Eight_bit,
                                           Pulnix::TM6740ConfigV2::x2,
                                           Pulnix::TM6740ConfigV2::x2,
                                           Pulnix::TM6740ConfigV2::Linear);
            else
                new ((void *)cfg->alloc(sizeof(Pulnix::TM6740ConfigV2)))
                    Pulnix::TM6740ConfigV2(32, 32, 0x1e8, 0x1e8, false,
                                           Pulnix::TM6740ConfigV2::Ten_bit,
                                           Pulnix::TM6740ConfigV2::x1,
                                           Pulnix::TM6740ConfigV2::x1,
                                           Pulnix::TM6740ConfigV2::Linear);
            break;
        case DetInfo::Opal1000:
            size = ((bld < 0) ? 2 : 3) * sizeof(Xtc) + sizeof(Opal1k::ConfigV1) +
                sizeof(Camera::FrameFexConfigV1);
            buf = (char *) calloc(1, size);
            if (bld < 0) {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_Opal1kConfig, 1), sourceInfo);
            } else {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_Xtc, 1), sourceInfo);
                cfg = new ((char *)cfg->alloc(size - sizeof(Xtc)))
                           Xtc(TypeId(TypeId::Id_Opal1kConfig, 2), bldInfo);
            }
            // Fake a configuration!
            // black = 32, gain = 100, Depth = 12 bit, binning = x1, mirroring = None,
            // vertical_remap = true, enable_pixel_creation = false.
            new ((void *)cfg->alloc(sizeof(Opal1k::ConfigV1)))
                Opal1k::ConfigV1(32, 100, Opal1k::ConfigV1::Twelve_bit, Opal1k::ConfigV1::x1,
                                 Opal1k::ConfigV1::None, true, false, false, 0, 0, 0);
            break;
        case DetInfo::OrcaFl40:
            size = ((bld < 0) ? 2 : 3) * sizeof(Xtc) + sizeof(Orca::ConfigV1) +
                sizeof(Camera::FrameFexConfigV1);
            buf = (char *) calloc(1, size);
            if (bld < 0) {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_OrcaConfig, 1), sourceInfo);
            } else {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_Xtc, 1), sourceInfo);
                cfg = new ((char *)cfg->alloc(size - sizeof(Xtc)))
                          Xtc(TypeId(TypeId::Id_OrcaConfig, 1), bldInfo);
            }
            // Fake a configuration!
            // readoutmode = Subarray, cooling = off, defect pixel correction = on, and rows = h.
            new ((void *)cfg->alloc(sizeof(Orca::ConfigV1)))
                Orca::ConfigV1(Orca::ConfigV1::Subarray, Orca::ConfigV1::Off, 1, h);
            break;
        default:
            printf("Unknown device: %s!\n", DetInfo::name(ctp));
            exit(1);
        }
        // This is on every camera.
        if (bld < 0)
            cfg = new ((char *) cfg->next()) Xtc(TypeId(TypeId::Id_FrameFexConfig, 1), sourceInfo);
        else
            cfg = new ((char *) cfg->next()) Xtc(TypeId(TypeId::Id_FrameFexConfig, 1), bldInfo);
        new ((void *)cfg->alloc(sizeof(Camera::FrameFexConfigV1)))
            Camera::FrameFexConfigV1(Camera::FrameFexConfigV1::FullFrame, 1,
                                     Camera::FrameFexConfigV1::NoProcessing, origin, origin, 0, 0, 0);

        configure_xtc(xid, buf, size, 0, 0);

        hdrlen = sizeof(Xtc) + sizeof(Camera::FrameV1);
        bf2 = (char *) calloc(1, hdrlen);
        frm = hdr = new (bf2) Xtc(TypeId(TypeId::Id_Frame, 1), sourceInfo);

        f = new ((char *)frm->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(w, h, d, 32);

        frm->alloc(f->_sizeof()-sizeof(*f));
        if (bld >= 0)
          hdr->alloc(f->_sizeof()-sizeof(*f));
        free(buf); /* We're done with r, which lives in buf. */

        int result = ca_create_channel(pvname.c_str(),
                                       connection_handler,
                                       this,
                                       50, /* priority?!? */
                                       &chan);
        if (result != ECA_NORMAL) {
            printf("CA error %s while creating channel to %s!\n",
                    ca_message(result), pvname.c_str());
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
    int    binned;
    int    strict;
    int    is_cam;
    int    caid;
    chid   chan;
};

vector<caconn *> caconn::conns;
int caconn::nxtcaid = 0;

static void event_handler(struct event_handler_args args)
{
    caconn *c = (caconn *)args.usr;

    if (args.status != ECA_NORMAL) {
        printf("Bad status: %d\n", args.status);
    } else if (args.type == c->dbrtype && args.count == c->nelem) {
        struct dbr_time_short *d = (struct dbr_time_short *) args.dbr;
        if (c->is_cam) {
            data_xtc(c->xid, d->stamp.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH, d->stamp.nsec,
                     c->hdr, c->hdrlen, &d->value);
        } else {
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
        exit(0);
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
        if (c->dbftype == DBF_LONG && c->is_cam)
            c->dbrtype = DBR_TIME_SHORT;             /* Force this, since we know the cameras are at most 16 bit!!! */
        else
            c->dbrtype = dbf_type_to_DBR_TIME(c->dbftype);
        c->size = dbr_size_n(c->dbrtype, c->nelem);
        if (c->is_cam) {
            int status = ca_create_subscription(c->dbrtype, c->nelem, args.chid, DBE_VALUE | DBE_ALARM,
                                                event_handler, (void *) c, &c->event);
            if (status != ECA_NORMAL) {
                printf("Failed to create subscription! error %d!\n", status);
                exit(0);
            }
        } else {
            int status = ca_array_get_callback(dbf_type_to_DBR_CTRL(c->dbftype), c->nelem, args.chid, get_handler, (void *) c);
            if (status != ECA_NORMAL) {
                printf("Get failed! error %d!\n", status);
                exit(0);
            }
        }
    } else {
        c->connected = 0;
        printf("%s (%s) has disconnected!\n", c->getname(), c->getpvname());
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

void create_ca(string name, string detector, string camtype, string pvname, int binned, int strict)
{
    new caconn(name, detector, camtype, pvname, binned, strict);
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
                exit(0);
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
