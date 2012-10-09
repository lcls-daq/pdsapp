#include<stdlib.h>

#include<vector>   //for std::vector

#include"pdsdata/xtc/Xtc.hh"
#include"pdsdata/xtc/TimeStamp.hh"
#include"pdsdata/xtc/DetInfo.hh"
#include"pdsdata/xtc/ProcInfo.hh"
#include"pdsdata/pulnix/TM6740ConfigV2.hh"
#include"pdsdata/opal1k/ConfigV1.hh"
#include"pdsdata/camera/FrameV1.hh"

#include"cadef.h"
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
        DetInfo::Detector det = find_detector(detector.c_str());
        DetInfo::Device   dev = find_device(camtype.c_str());
        int               pid = getpid();
        char             *buf = NULL;
        Xtc              *cfg = NULL;
        Camera::FrameV1  *f   = NULL;

        if (det == DetInfo::NumDetector || dev == DetInfo::NumDevice) {
            fprintf(stderr, "Cannot find (%s, %s) values!\n", detector.c_str(), camtype.c_str());
            exit(1);
        }

        num = conns.size();
        conns.push_back(this);
        xid = register_xtc();

        DetInfo sourceInfo(pid, det, 0, dev, 0);

        switch (dev) {
        case DetInfo::TM6740:
            buf = (char *) calloc(1, sizeof(Xtc) + sizeof(Pulnix::TM6740ConfigV2));
            cfg = new (buf) Xtc(TypeId(TypeId::Id_TM6740Config, 2), sourceInfo);
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
            buf = (char *) calloc(1, sizeof(Xtc) + sizeof(Opal1k::ConfigV1));
            cfg = new (buf) Xtc(TypeId(TypeId::Id_Opal1kConfig, 1), sourceInfo);
            // Fake a configuration!
            // black = 32, gain = 100, Depth = 12 bit, binning = x1, mirroring = None,
            // vertical_remap = true, enable_pixel_creation = false.
            if (binned) {
                new ((void *)cfg->alloc(sizeof(Opal1k::ConfigV1)))
                    Opal1k::ConfigV1(32, 100, Opal1k::ConfigV1::Eight_bit, Opal1k::ConfigV1::x2,
                                     Opal1k::ConfigV1::None, true, false);
            } else {
                new ((void *)cfg->alloc(sizeof(Opal1k::ConfigV1)))
                    Opal1k::ConfigV1(32, 100, Opal1k::ConfigV1::Twelve_bit, Opal1k::ConfigV1::x1,
                                     Opal1k::ConfigV1::None, true, false);
            }
            break;
        default:
            fprintf(stderr, "Unknown device: %s!\n", DetInfo::name(dev));
            exit(1);
        }
        configure_xtc(xid, cfg);
        free(buf); /* delete ~cfg? */

        hdrlen = sizeof(Xtc) + sizeof(Camera::FrameV1);
        buf = (char *) calloc(1, hdrlen);
        hdr = new (buf) Xtc(TypeId(TypeId::Id_Frame, 1), sourceInfo);
        switch (dev) {
        case DetInfo::TM6740:
            if (binned)
                f = new ((char *)hdr->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(320, 240, 8, 32);
            else
                f = new ((char *)hdr->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(640, 480, 10, 32);
            break;
        case DetInfo::Opal1000:
            if (binned)
                f = new ((char *)hdr->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(512, 512, 8, 32);
            else
                f = new ((char *)hdr->alloc(sizeof(Camera::FrameV1))) Camera::FrameV1(1024, 1024, 12, 32);
            break;
        default:
            /* This is to keep the compiler happy.  We never get here! */
            exit(1);
        }
        hdr->alloc(f->data_size()); // This is in the data packet, not the header!

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
 private:
    static vector<caconn *> conns;
    string name;
    string detector;
    string camtype;
    string pvname;
    chid   chan;
 public:
    int    num;
    int    connected;
    int nelem, dbrtype, size, status;
    chtype dbftype;
    evid event;
    int    xid;
    Xtc   *hdr;
    int    hdrlen;
    int    binned;
};

vector<caconn *> caconn::conns;

static void event_handler(struct event_handler_args args)
{
    caconn *c = (caconn *)args.usr;

    if (args.status != ECA_NORMAL) {
        fprintf(stderr, "Bad status: %d\n", args.status);
    } else if (args.type == c->dbrtype && args.count == c->nelem) {
        struct dbr_time_short *d = (struct dbr_time_short *) args.dbr;
        data_xtc(c->xid, d->stamp.nsec & 0x1ffff, d->stamp.secPastEpoch, d->stamp.nsec,
                 c->hdr, c->hdrlen, &d->value);
    } else {
        fprintf(stderr, "type = %ld, count = %ld -> expected type = %d, count = %d\n",
                args.type, args.count, c->dbrtype, c->nelem);
    }
    fflush(stdout);
}

static void connection_handler(struct connection_handler_args args)
{
    caconn *c = caconn::find(args.chid);

    if (args.op == CA_OP_CONN_UP) {
        c->connected = 1;
        c->nelem = ca_element_count(args.chid);
        c->dbftype = ca_field_type(args.chid);
        if (c->dbftype == DBF_LONG)
            c->dbrtype = DBR_TIME_SHORT;             /* Force this, since we know the cameras are at most 16 bit!!! */
        else
            c->dbrtype = dbf_type_to_DBR_TIME(c->dbftype);
        c->size = dbr_size_n(c->dbrtype, c->nelem);
#if 0
        fprintf(stderr, "Size of %d elements of type (%d,%d): %d bytes\n",
                c->nelem, c->dbftype, c->dbrtype, c->size);
#endif

        int status = ca_create_subscription(c->dbrtype, c->nelem, args.chid, DBE_VALUE | DBE_ALARM,
                                            event_handler, (void *) c, &c->event);
        if (status != ECA_NORMAL) {
            fprintf(stderr, "Failed to create subscription! error %d!\n", status);
            exit(0);
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
