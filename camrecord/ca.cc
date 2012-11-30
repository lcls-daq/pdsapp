#include<stdlib.h>
#include<string.h>
#include<vector>   //for std::vector

#include"cadef.h"
#include"alarm.h"
#include"pdsdata/xtc/Xtc.hh"
#include"pdsdata/xtc/TimeStamp.hh"
#include"pdsdata/xtc/DetInfo.hh"
#include"pdsdata/xtc/ProcInfo.hh"
#include"pdsdata/pulnix/TM6740ConfigV2.hh"
#include"pdsdata/opal1k/ConfigV1.hh"
#include"pdsdata/camera/FrameFexConfigV1.hh"
#include"pdsdata/lusi/PimImageConfigV1.hh"
#include"pdsdata/camera/FrameV1.hh"
#include"pdsdata/epics/EpicsPvData.hh"
#include"yagxtc.hh"

using namespace std;
using namespace Pds;

static void connection_handler(struct connection_handler_args args);

static DetInfo::Detector find_detector(const char *name)
{
    int i;
    for (i = 0; i != (int)DetInfo::NumDetector; i++) {
        if (!strcmp(name, DetInfo::name((DetInfo::Detector)i)))
            break;
    }
    return (DetInfo::Detector)i;
}

DetInfo::Device find_device(const char *name)
{
    int i;
    for (i = 0; i != (int)DetInfo::NumDevice; i++) {
        if (!strcmp(name, DetInfo::name((DetInfo::Device)i)))
            break;
    }
    return (DetInfo::Device)i;
}

class caconn {
 public:
    caconn(string _name, string _det, string _camtype, string _pvname, int _binned)
        : name(_name), detector(_det), camtype(_camtype), pvname(_pvname), connected(0), event(0), binned(_binned) {
        const char       *ds  = detector.c_str();
        int               bld = !strncmp(ds, "BLD-", 4);
        DetInfo::Detector det = bld ? DetInfo::BldEb : find_detector(ds);
        DetInfo::Device   ctp = find_device(camtype.c_str());
        DetInfo::Device   dev = bld ? DetInfo::NoDevice : ctp;
        char             *buf = NULL;
        char             *bf2 = NULL;
        Xtc              *cfg = NULL;
        Xtc              *frm = NULL;
        Xtc              *r1  = NULL;
        Xtc              *r2  = NULL;
        Camera::FrameV1  *f   = NULL;

        if (det == DetInfo::NumDetector || (det != DetInfo::EpicsArch && dev == DetInfo::NumDevice)) {
            fprintf(stderr, "Cannot find (%s, %s) values!\n", detector.c_str(), camtype.c_str());
            exit(1);
        }
        bld = bld ? atoi(ds + 4) : -1;

        num = conns.size();
        conns.push_back(this);
        xid = register_xtc(det != DetInfo::EpicsArch);
        if (det == DetInfo::EpicsArch) {
            caid = nxtcaid++;
            is_cam = 0;
            int result = ca_create_channel(pvname.c_str(),
                                           connection_handler,
                                           this,
                                           50, /* priority?!? */
                                           &chan);
            if (result != ECA_NORMAL) {
                fprintf(stderr, "CA error %s while creating channel to %s!\n",
                        ca_message(result), pvname.c_str());
                exit(0);
            }
            ca_poll();
            return;
        } else
            is_cam = 1;

        int               pid = getpid(), size;
        DetInfo sourceInfo(pid, det, 0, dev, 0);
        ProcInfo bldInfo(Level::Reporter, pid, bld);
        Camera::FrameCoord origin(0, 0);

        switch (ctp) {
        case DetInfo::TM6740:
            size = ((bld < 0) ? 3 : 4) * sizeof(Xtc) + sizeof(Pulnix::TM6740ConfigV2) +
                   sizeof(Camera::FrameFexConfigV1) + sizeof(Lusi::PimImageConfigV1);
            buf = (char *) calloc(1, size);
            if (bld < 0) {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_TM6740Config, 2), sourceInfo);
            } else {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_Xtc, 1), sourceInfo);
                cfg = new ((char *)cfg->alloc(size - sizeof(Xtc)))
                          Xtc(TypeId(TypeId::Id_TM6740Config, 2), bldInfo);
                r1 = cfg;
            }
            // Fake a configuration!
            // black_level_a = black_level_b = 32, gaina = gainb = 0x1e8 (max, min is 0x42),
            // gain_balance = false, Depth = 10 bit, hbinning = vbinning = x1, LookupTable = linear.
            if (binned) {
                new ((void *)cfg->alloc(sizeof(Pulnix::TM6740ConfigV2)))
                    Pulnix::TM6740ConfigV2(32, 32, 0x1e8, 0x1e8, false,
                                           Pulnix::TM6740ConfigV2::Eight_bit,
                                           Pulnix::TM6740ConfigV2::x2,
                                           Pulnix::TM6740ConfigV2::x2,
                                           Pulnix::TM6740ConfigV2::Linear);
            } else {
                new ((void *)cfg->alloc(sizeof(Pulnix::TM6740ConfigV2)))
                    Pulnix::TM6740ConfigV2(32, 32, 0x1e8, 0x1e8, false,
                                           Pulnix::TM6740ConfigV2::Ten_bit,
                                           Pulnix::TM6740ConfigV2::x1,
                                           Pulnix::TM6740ConfigV2::x1,
                                           Pulnix::TM6740ConfigV2::Linear);
            }
            break;
        case DetInfo::Opal1000:
            size = ((bld < 0) ? 3 : 4) * sizeof(Xtc) + sizeof(Pulnix::TM6740ConfigV2) +
                   sizeof(Camera::FrameFexConfigV1) + sizeof(Lusi::PimImageConfigV1);
            buf = (char *) calloc(1, size);
            if (bld < 0) {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_Opal1kConfig, 1), sourceInfo);
            } else {
                cfg = new (buf) Xtc(TypeId(TypeId::Id_Xtc, 1), sourceInfo);
                cfg = new ((char *)cfg->alloc(size - sizeof(Xtc)))
                           Xtc(TypeId(TypeId::Id_Opal1kConfig, 2), bldInfo);
                r1 = cfg;
            }
            // Fake a configuration!
            // black = 32, gain = 100, Depth = 12 bit, binning = x1, mirroring = None,
            // vertical_remap = true, enable_pixel_creation = false.
            if (binned) {
                new ((void *)cfg->alloc(sizeof(Opal1k::ConfigV1)))
                    Opal1k::ConfigV1(32, 100, Opal1k::ConfigV1::Twelve_bit, Opal1k::ConfigV1::x1,
                                     Opal1k::ConfigV1::None, true, false);
            } else {
                new ((void *)cfg->alloc(sizeof(Opal1k::ConfigV1)))
                    Opal1k::ConfigV1(32, 100, Opal1k::ConfigV1::Twelve_bit, Opal1k::ConfigV1::x1,
                                     Opal1k::ConfigV1::None, true, false);
            }
            break;
        default:
            fprintf(stderr, "Unknown device: %s!\n", DetInfo::name(ctp));
            exit(1);
        }
        // These are on every camera.
        if (bld < 0)
            cfg = new ((char *) cfg->next()) Xtc(TypeId(TypeId::Id_FrameFexConfig, 1), sourceInfo);
        else
            cfg = new ((char *) cfg->next()) Xtc(TypeId(TypeId::Id_FrameFexConfig, 1), bldInfo);
        new ((void *)cfg->alloc(sizeof(Camera::FrameFexConfigV1)))
            Camera::FrameFexConfigV1(Camera::FrameFexConfigV1::FullFrame, 1,
                                     Camera::FrameFexConfigV1::NoProcessing, origin, origin, 0, 0, NULL);

        if (bld < 0)
            cfg = new ((char *) cfg->next()) Xtc(TypeId(TypeId::Id_PimImageConfig, 1), sourceInfo);
        else {
            cfg = new ((char *) cfg->next()) Xtc(TypeId(TypeId::Id_PimImageConfig, 1), bldInfo);
            r2 = cfg;
        }
        new ((void *)cfg->alloc(sizeof(Lusi::PimImageConfigV1)))
            Lusi::PimImageConfigV1(1.0, 1.0);   // What should these be?!?
        configure_xtc(xid, buf, size);

        if (buf < 0) {
            hdrlen = sizeof(Xtc) + sizeof(Camera::FrameV1);
            bf2 = (char *) calloc(1, hdrlen);
            frm = hdr = new (bf2) Xtc(TypeId(TypeId::Id_Frame, 1), sourceInfo);
        } else {
            hdrlen = size - sizeof(Camera::FrameFexConfigV1) + sizeof(Camera::FrameV1);
            bf2 = (char *) calloc(1, hdrlen);
            hdr = new (bf2) Xtc(TypeId(TypeId::Id_Xtc, 1), bldInfo);

            // Copy the configuration xtc.
            memcpy((void *)hdr->alloc(r1->extent), (void *)r1, r1->extent);

            // Copy the PimImageConfig xtc.
            memcpy((void *)hdr->alloc(r2->extent), (void *)r2, r2->extent);

            frm = new ((char *)hdr->alloc(sizeof(Xtc) + sizeof(Camera::FrameV1)))
                Xtc(TypeId(TypeId::Id_Frame, 1), bldInfo);
        }
        switch (ctp) {
        case DetInfo::TM6740:
            if (binned)
                f = new ((char *)frm->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(320, 240, 8, 32);
            else
                f = new ((char *)frm->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(640, 480, 10, 32);
            break;
        case DetInfo::Opal1000:
            if (binned)
                f = new ((char *)frm->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(256, 1024, 12, 32);
            else
                f = new ((char *)frm->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(1024, 1024, 12, 32);
            break;
        default:
            /* This is to keep the compiler happy.  We never get here! */
            exit(1);
        }
        frm->alloc(f->data_size());
        if (bld >= 0)
            hdr->alloc(f->data_size());
        free(buf); /* We're done with r, which lives in buf. */

        int result = ca_create_channel(pvname.c_str(),
                                       connection_handler,
                                       this,
                                       50, /* priority?!? */
                                       &chan);
        if (result != ECA_NORMAL) {
            fprintf(stderr, "CA error %s while creating channel to %s!\n",
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
        fprintf(stderr, "Bad status: %d\n", args.status);
    } else if (args.type == c->dbrtype && args.count == c->nelem) {
        struct dbr_time_short *d = (struct dbr_time_short *) args.dbr;
        if (c->is_cam) {
            data_xtc(c->xid, d->stamp.secPastEpoch, d->stamp.nsec,
                     c->hdr, c->hdrlen, &d->value);
        } else {
            /*
             * This is totally cheating, since we aren't looking at the actual dbr_time_* type.
             * But we know they all start status, severity, stamp, so...
             */
            data_xtc(c->xid, d->stamp.secPastEpoch, d->stamp.nsec,
                     c->hdr, c->hdrlen, const_cast<void *>(args.dbr));
        }
    } else {
        fprintf(stderr, "type = %ld, count = %ld -> expected type = %ld, count = %d\n",
                args.type, args.count, c->dbrtype, c->nelem);
    }
    fflush(stdout);
}

static void get_handler(struct event_handler_args args)
{
    caconn *c = (caconn *)args.usr;
    DetInfo sourceInfo(getpid(), DetInfo::EpicsArch, 0, DetInfo::NoDevice, 0);
    int hdrsize = sizeof(EpicsPvCtrlHeader);
    int ctrlsize = dbr_size_n(args.type, 1);

    if (hdrsize % 4)
        hdrsize += 4 - (hdrsize % 4); /* Sigh.  Padding in the middle! */

    char             *buf = (char *) calloc(1, sizeof(Xtc) + hdrsize + ctrlsize);
    Xtc              *cfg = new (buf) Xtc(TypeId(TypeId::Id_Epics, 1), sourceInfo);
    new ((char *)cfg->alloc(hdrsize)) EpicsPvCtrlHeader(c->caid, args.type, 1, c->getpvname());
    void             *buf2 = cfg->alloc(ctrlsize);
    memcpy(buf2, args.dbr, ctrlsize);
    configure_xtc(c->xid, (char *) cfg, cfg->extent);
    free(buf); /* delete ~cfg? */

    hdrsize = sizeof(EpicsPvHeader);
    if (hdrsize % 4)
        hdrsize += 4 - (hdrsize % 4); /* Sigh.  Padding in the middle! */
    c->hdrlen = sizeof(Xtc) + hdrsize;
    buf = (char *) calloc(1, c->hdrlen);
    c->hdr = new (buf) Xtc(TypeId(TypeId::Id_Epics, 1), sourceInfo);
    new ((char *)c->hdr->alloc(hdrsize)) EpicsPvHeader(c->caid, c->dbrtype, 1);
    c->hdr->alloc(c->size); // This is in the data packet, not the header!

    int status = ca_create_subscription(c->dbrtype, c->nelem, c->chan, DBE_VALUE | DBE_ALARM,
                                        event_handler, (void *) c, &c->event);
    if (status != ECA_NORMAL) {
        fprintf(stderr, "Failed to create subscription! error %d!\n", status);
        exit(0);
    }
}

static void connection_handler(struct connection_handler_args args)
{
    caconn *c = caconn::find(args.chid);

    if (args.op == CA_OP_CONN_UP) {
        c->connected = 1;
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
                fprintf(stderr, "Failed to create subscription! error %d!\n", status);
                exit(0);
            }
        } else {
            if (c->nelem != 1) {
                fprintf(stderr, "%s is not a scalar PV!\n", c->getpvname());
                exit(0);
            }
            int status = ca_get_callback(dbf_type_to_DBR_CTRL(c->dbftype), args.chid, get_handler, (void *) c);
            if (status != ECA_NORMAL) {
                fprintf(stderr, "Get failed! error %d!\n", status);
                exit(0);
            }
        }
    } else {
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

void create_ca(string name, string detector, string camtype, string pvname, int binned)
{
    new caconn(name, detector, camtype, pvname, binned);
}

void handle_ca(fd_set *rfds)
{
    ca_poll();
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
