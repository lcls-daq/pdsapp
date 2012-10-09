#define EXACT_TS_MATCH
#undef TRACE
#include<stdio.h>
#include<signal.h>
#include<string.h>

#include<new>
#include<vector>

#include"pdsdata/xtc/Xtc.hh"
#include"pdsdata/xtc/TimeStamp.hh"
#include"pdsdata/xtc/Dgram.hh"
#include"pdsdata/xtc/DetInfo.hh"
#include"pdsdata/xtc/ProcInfo.hh"
#include"pdsdata/xtc/TransitionId.hh"
#include"pdsdata/pulnix/TM6740ConfigV2.hh"
#include"pdsdata/opal1k/ConfigV1.hh"
#include"pdsdata/camera/FrameV1.hh"
#include"pdsdata/bld/bldData.hh"
#include"pds/service/NetServer.hh"
#include"pds/service/Ins.hh"

#include"yagxtc.hh"

using namespace std;
using namespace Pds;

class xtcsrc {
 public:
    xtcsrc(int _id) : id(_id), cfg(NULL), cfglen(0) {
    };
    int   id;
    char *cfg;
    int   cfglen;
};

FILE *fp = NULL;
static sigset_t blockset;
static int numsrc = 0;
static int cfgcnt = 0;
static int totalcfglen = 0;
static int totaldlen   = 0;
static vector<xtcsrc *> src;

static Dgram *dg = NULL;
static Xtc   *seg = NULL;
static ProcInfo *evtInfo = NULL;
static ProcInfo *segInfo = NULL;

#define MAX_EVENTS    64
static struct event {
    int    cnt;
    unsigned int sec, nsec;
    char **data;
    int   *len;
    int    fid;
} events[MAX_EVENTS];
static int curev = 0;

static void delete_event(int ev)
{
    int i;

    for (i = 0; i < numsrc; i++) {
        if (events[ev].data[i]) {
            delete events[ev].data[i];
            events[ev].data[i] = NULL;
#ifdef TRACE
            if (events[ev].cnt != numsrc)
                printf("\t\tX%d@0x%x -> del %d\n", ev, events[ev].fid, i);
#endif
        }
    }
    events[ev].cnt = 0;
}

static int find_event(unsigned int sec, unsigned int nsec)
{
    int i;
#ifndef EXACT_TS_MATCH
    int fid = nsec & 0x1ffff;
#endif
    for (i = 0; i < MAX_EVENTS; i++) {
        int ev = (curev + MAX_EVENTS - 1 - i) % MAX_EVENTS;
#ifdef EXACT_TS_MATCH
        if (events[ev].sec == sec && events[ev].nsec == nsec)
            return ev;
#else
        if (events[ev].sec == sec && abs(events[ev].fid - fid) <= 2)
            return ev;
#endif
    }
    i = curev++;
    if (curev == MAX_EVENTS)
        curev = 0;
    delete_event(i);
    events[i].sec = sec;
    events[i].nsec = nsec;
    return i;
}

static void setup_datagram(TransitionId::Value val)
{
    new ((void *) &dg->seq) Sequence(Sequence::Event, val, ClockTime(0, 0), TimeStamp(0, 0x1ffff, 0, 0));
    new ((void *) &dg->env) Env(0);
    new ((char *) &dg->xtc) Xtc(TypeId(TypeId::Id_Xtc, 1), *evtInfo);

    seg = new (&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc, 1), *segInfo);
}

static void write_datagram(TransitionId::Value val, int extra)
{
    sigset_t oldsig;

    setup_datagram(val);
    
    seg->extent    += extra;
    dg->xtc.extent += extra;

    sigprocmask(SIG_BLOCK, &blockset, &oldsig);
    if (!fwrite(dg, sizeof(Dgram) + sizeof(Xtc), 1, fp)) {
        fprintf(stderr, "Write failed!\n");
        exit(1);
    }
    fflush(fp);
    sigprocmask(SIG_SETMASK, &oldsig, NULL);
}

/*
 * This is called once we have all of the configuration data.
 */
static void write_xtc_config(void)
{
    int i;
    sigset_t oldsig;

    dg = (Dgram *) malloc(sizeof(Dgram) + sizeof(Xtc));
    
    int pid = getpid();
    int ipaddr = 0x7f000001;

    evtInfo = new ProcInfo(Level::Event, pid, ipaddr);
    segInfo = new ProcInfo(Level::Segment, pid, ipaddr);

    sigprocmask(SIG_BLOCK, &blockset, &oldsig);
    write_datagram(TransitionId::Configure, totalcfglen);
    for (i = 0; i < numsrc; i++) {
        if (!fwrite(src[i]->cfg, src[i]->cfglen, 1, fp)) {
            fprintf(stderr, "Cannot write to file!\n");
            exit(1);
        }
        fflush(fp);
    }
    sigprocmask(SIG_SETMASK, &oldsig, NULL);

    write_datagram(TransitionId::BeginRun,        0);
    write_datagram(TransitionId::BeginCalibCycle, 0);
    write_datagram(TransitionId::Enable,          0);

    begin_run();
}

/*
 * This is called *after* the config file has been read and all of the
 * sources have been declared.  Therefore, we can use the global fp to
 * tell if we are running pre- or post-initialization.
 */
void initialize_xtc(char *outfile)
{
    int i;

    for (i = 0; i < MAX_EVENTS; i++) {
        events[i].cnt = 0;
        events[i].data = (char **) calloc(numsrc, sizeof(char *));
        events[i].len  = (int *)   calloc(numsrc, sizeof(int));
    }

    if (!strcmp(outfile, "-"))
        fp = stdout;
    else if (!(fp = fopen(outfile, "w"))) {
        fprintf(stderr, "Cannot open %s for output!\n", outfile);
        exit(0);
    }

    sigemptyset(&blockset);
    sigaddset(&blockset, SIGALRM);
    sigaddset(&blockset, SIGINT);

    if (cfgcnt == numsrc) {
        write_xtc_config();
    }
}

/*
 * Generate a unique id for this source.
 */
int register_xtc(void)
{
    src.push_back(new xtcsrc(numsrc));
    return numsrc++;
}

/*
 * Give the configuration Xtc for a particular source.
 */
void configure_xtc(int id, Pds::Xtc *xtc)
{
    int size = xtc->extent;
    src[id]->cfg = new char[size];
    src[id]->cfglen = size;
    totalcfglen += size;
    memcpy((void *)src[id]->cfg, (void *)xtc, size);
    cfgcnt++;

    if (fp != NULL && cfgcnt == numsrc) {
        write_xtc_config();
    }
}

/*
 * Give the data Xtc for a particular source.  We assume that the Xtc header is
 * in one area and the data is in another, so we pass in a second pointer.  That
 * is, hdr contains hdrlen bytes, and data contains (hdr->extent - hdrlen) bytes.
 */
void data_xtc(int id, int fid, unsigned int sec, unsigned int nsec, Pds::Xtc *hdr, int hdrlen, void *data)
{
    int ev, i;
    sigset_t oldsig;

    if (!fp || !data_cnt)
        return; /* Don't do anything until we've initialized! */

    char *buf = new char[hdr->extent];
    memcpy(buf, hdr, hdrlen);
    memcpy(&buf[hdrlen], data, hdr->extent - hdrlen);

#if 0
    DetInfo& info = *(DetInfo*)(&hdr->src);
    printf("%08x:%08x (%5x) BLD contains %d.%d, src %s,%d  %s,%d\n",
           sec, nsec, fid, hdr->contains.id(), hdr->contains.version(), 
           DetInfo::name(info.detector()), info.detId(),
           DetInfo::name(info.device()), info.devId());
#endif

    ev = find_event(sec, nsec);
    events[ev].fid = fid;

#ifdef TRACE
    printf("\tD%d@0x%x (%08x:%08x) -> got %d\n", ev, fid, sec, nsec, id);
#endif

    if (events[ev].data[id]) {
        fprintf(stderr, "Warning: duplicate data for source %d at time %08x:%08x\n",
                id, sec, nsec);
        delete buf;
        return;
    }
    events[ev].data[id] = buf;
    events[ev].len[id] = hdr->extent;
    data_cnt[id]++;
    if (++events[ev].cnt != numsrc)
        return;

#ifdef TRACE
    printf("C%d@0x%x\n", ev, fid);
#endif

    // We have a complete event!
    if (!totaldlen) {
        for (i = 0; i < numsrc; i++)
            totaldlen += events[ev].len[i];

        setup_datagram(TransitionId::L1Accept);
        seg->extent    += totaldlen;
        dg->xtc.extent += totaldlen;
    }

    new ((void *) &dg->seq) Sequence(Sequence::Event, TransitionId::L1Accept, 
                                     ClockTime(sec, nsec), 
                                     TimeStamp(0, fid, 0, 0));

    sigprocmask(SIG_BLOCK, &blockset, &oldsig);
    if (!fwrite(dg, sizeof(Dgram) + sizeof(Xtc), 1, fp)) {
        fprintf(stderr, "Write failed!\n");
        exit(1);
    }
    for (i = 0; i < numsrc; i++) {
        if (!fwrite(events[ev].data[i], events[ev].len[i], 1, fp)) {
            fprintf(stderr, "Write failed!\n");
            exit(1);
        }
    }
    fflush(fp);
    sigprocmask(SIG_SETMASK, &oldsig, NULL);
    record_cnt++;

    delete_event(ev);

#if 0
    // Events from curev to ev, are invalid now!
    for (i = curev; i != ev; i = (i + 1) % MAX_EVENTS)
        delete_event(i);
#endif
}

void cleanup_xtc(void)
{
    write_datagram(TransitionId::Disable,       0);
    write_datagram(TransitionId::EndCalibCycle, 0);
    write_datagram(TransitionId::EndRun,        0);
    write_datagram(TransitionId::Unconfigure,   0);
    fclose(fp);
}
