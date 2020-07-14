//#define DUPLICATE_SUPPRESSION
#define EXACT_TS_MATCH          0
#define FIDUCIAL_MATCH          1
//#define TRACE
#include"pds/logbookclient/WSLogBook.hh"
#include<stdio.h>
#include<signal.h>
#include<string.h>
#include<strings.h>
#include<math.h>
#include<netdb.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<pthread.h>

#include<new>
#include<vector>

#include"pdsdata/xtc/Xtc.hh"
#include"pdsdata/xtc/TimeStamp.hh"
#include"pdsdata/xtc/Dgram.hh"
#include"pdsdata/xtc/DetInfo.hh"
#include"pdsdata/xtc/ProcInfo.hh"
#include"pdsdata/xtc/TransitionId.hh"
#include"pdsdata/index/IndexList.hh"
#include"pdsdata/psddl/pulnix.ddl.h"
#include"pdsdata/psddl/opal1k.ddl.h"
#include"pdsdata/psddl/camera.ddl.h"
#include"pdsdata/psddl/bld.ddl.h"
#include"pdsdata/psddl/alias.ddl.h"
#include"pdsdata/psddl/epics.ddl.h"
#include"pdsdata/psddl/smldata.ddl.h"
#include"pds/service/NetServer.hh"
#include"pds/service/Ins.hh"

#include"yagxtc.hh"

using namespace std;
using namespace Pds;

struct event;

class xtcsrc {
 public:
    xtcsrc(int _id, int _sync, string _name, int _crit, int _isbig, int _iswv8)
        : id(_id), hdf_id(-1), sync(_sync), name(_name), cnt(0), val(NULL),
          len(0), val2(NULL), len2(0), hdrlen(0), ref(NULL), sec(0), nsec(0),
          sec2(0), nsec2(0), ev(NULL), critical(_crit), damagecnt(0),
          isbig(_isbig), iswv8(_iswv8) {};
    int   id;
    int   hdf_id;
    int   sync;                   // Is this a synchronous source?
    string name;
    int   cnt;
    unsigned char *val;
    int   len;
    unsigned char *val2;
    int   len2;
    int   hdrlen;                 // header length for this value
    int  *ref;                    // Reference count of this value (if asynchronous!)
    unsigned int sec, nsec;       // Timestamp of this value
    unsigned int sec2, nsec2;     // Timestamp of the second value
    struct event *ev;             // Event for this value (if asynchronous!)
    Src   src;
    int   critical;               // If we have a critical source, then if anything else is missing,
                                  // we declare damage.  If we don't have any critical sources, we just
                                  // throw away partials.
    int   damagecnt;
    int   isbig;
    int   iswv8;
};

#define SML_CONFIG_SIZE (sizeof(Xtc) + sizeof(SmlData::ConfigV1))
#define SML_OFFSET_SIZE (sizeof(Xtc) + sizeof(SmlData::OrigDgramOffsetV1))
#define SML_PROXY_SIZE  (sizeof(Xtc) + sizeof(SmlData::ProxyV1))

#define CHUNK_SIZE 107374182400LL
// #define CHUNK_SIZE      100000000LL  // For debug!
FILE *fp = NULL;
FILE *sfp = NULL;
Index::IndexList _indexList;
static char *fname;            // The current output file name.
static char *sfname;           // The current small output file name.
static char *iname;            // The current index file name.
static char *cpos;             // Where in the current file name to change the chunk number.
static char *cspos;            // Where in the current small file name to change the chunk number.
static char *cipos;            // Where in the current index name to change the chunk number.
static int chunk = 0;          // The current chunk number.
static int64_t fsize = 0;      // The size of the current file, so far.
static int64_t sfsize = 0;     // The size of the current small file, so far.
static sigset_t blockset;
static int started = 0;        // We are actually running.
static int paused = 1;         // We are temporarily paused.
static int numsrc = 0;         // Total number of sources.
static int critsrc = 0;        // Number of critical sources.
static int syncsrc = 0;        // Number of synchronous sources.
static int asyncsrc = 0;       // Number of asynchronous sources.
static unsigned int maxname = 0;// Maximum name length.
static int havedata = 0;       // Number of sources that sent us a value.
static int haveasync = 0;      // Number of asynchronous sources that sent us a value.
static int cfgcnt = 0;         // Number of sources that sent us their configuration.
static int totalcfglen = 0;    // Total number of bytes in all of the configuration records.
static int totaldlen   = 0;    // Total number of bytes in all of the data records.
static int totalsdlen  = SML_OFFSET_SIZE;    // Total number of bytes in all of the small data records.
static unsigned int csec = 0;  // Configuration timestamp.
static unsigned int cnsec = 0;
static unsigned int cfid = 0;
static unsigned int dsec = 0;  // Last data timestamp.
static unsigned int dnsec = 0;
static int havetransitions = 0;
static int isinit = 0;
static vector<xtcsrc *> src;
static vector<Alias::SrcAlias *> alias;
static DetInfo pvinfo;
static vector<Epics::PvConfigV1 *> pvalias;

static Dgram *dg = NULL;
static Xtc   *seg = NULL;
static ProcInfo *evtInfo = NULL;
static ProcInfo *segInfo = NULL;
static ProcInfo *ctrlInfo = NULL;
static Xtc    damagextc(TypeId(TypeId::Any, 1));
static pthread_mutex_t datalock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fdlock   = PTHREAD_MUTEX_INITIALIZER;
static int lasttrans = -1;

#define MAX_EVENTS    128
static struct event {
    int    id;
    int    valid;
    int    critcnt;               // How many critical sources do we have?
    int    synccnt;               // How many synchronous sources do we have?
    int    damcnt;                // How many damaged synchronous sources do we have?
    unsigned int sec, nsec;       // Timestamp of this event.
    unsigned char **data;         // Array of numsrc data records.
    int  **ref;                   // An array of reference counts for asynchronous data.
    struct event *prev, *next;    // Linked list, sorted by timestamp.
} events[MAX_EVENTS];
static struct event *evlist = NULL; // The oldest event.  Its prev is the newest event.

#define MAX_TRANS    128
static struct transition_queue {
    unsigned int id;
    unsigned int secs;
    unsigned int nsecs;
    unsigned int fid;
} saved_trans[MAX_TRANS];
static int transidx = 0;
static int cfgdone = 0;
static int match_type = FIDUCIAL_MATCH;

static struct event *find_event(unsigned int sec, unsigned int nsec);

void nofid(void)
{
    match_type = EXACT_TS_MATCH;
}

static FILE *myfopen(const char *name, const char *flags, int doreg)
{
    FILE *fp = fopen(name, flags);
    if (!fp)
        return fp;
    if (!expname.empty()) {
        /* We set this in a dbinfo, so we need to do some data mover magic! */
        struct flock flk;
        int rc;

        flk.l_type  = F_WRLCK;
        flk.l_whence = SEEK_SET;
        flk.l_start = 0;
        flk.l_len = 0;
        do {
            rc = fcntl(fileno(fp), F_SETLKW, &flk);
        } while (rc < 0 && errno == EINTR);
        if (rc < 0) {
            /* This can't be good.  Better not tell the data mover about it! */
	    printf("myfopen(%s): lock failed?\n", name);
            return fp;
        }
        if (!doreg)
            return fp;
        try {
            WSLogbookClient* client = WSLogbookClient::createWSLogbookClient(logbook[0].c_str(), logbook[1].c_str(), logbook[2].c_str(), logbook[3] == "true", expname.c_str());
            std::cout << "Current experiment is " << client->current_experiment() << std::endl;
	    std::string newfile = ((std::string)curdir) + name;
            client->report_open_file(newfile.c_str(), strnum, chunk, hostname.c_str(), true);
        } catch(const std::runtime_error& e){
            printf("Caught exception %s\n", e.what());
            fprintf(stderr, "Caught exception %s. Aborting!!!\n", e.what());
            exit(1);
        }
    }
    return fp;
}

void debug_list(struct event *ev)
{
    struct event *e = ev;

    printf("List starting with %d:\n", ev->id);
    do {
        if (e->valid)
            printf("    %2d: %08x:%08x\n", e->id, e->sec, e->nsec);
        e = e->next;
    } while (e != ev);
}

int length_list(struct event *ev)
{
    struct event *e = ev;
    int i = 0;
    do {
        i++;
        e = e->next;
    } while (e != ev);
    return i;
}

/*
 * Return 0 if equal, > 0 if event time > given time, < 0 if event time < given time.
 */
static int ts_match(struct event *_ev, int _sec, int _nsec)
{
    int diff;

    switch (match_type) {
    case EXACT_TS_MATCH:
        /* Exactly match the timestamps */
        return ((int) _ev->sec != _sec) ? ((int)_ev->sec - _sec) : ((int)_ev->nsec - _nsec);
    case FIDUCIAL_MATCH:
        /* Exactly match the fiducials, and make the seconds be close. */
        diff = _ev->sec - _sec;
        if (abs(diff) <= 1) /* Close to the same second! */
            return (0x1ffff & (int)_ev->nsec) - (0x1ffff & _nsec);
        else
            return diff;
    }
    return 0;
}

static void setup_datagram(TransitionId::Value val)
{
    if (val == TransitionId::BeginCalibCycle)
        _indexList.addCalibCycle(fsize, csec, cnsec);
    new ((void *) &dg->seq) Sequence(Sequence::Event, val, ClockTime(csec, cnsec),
                                     TimeStamp(0, cfid, 0, 0));
    if ((cnsec += 0x20000) > 1000000000) {
        cnsec -= 1000000000;
        csec++;
    }
    new ((void *) &dg->env) Env(0);
    new ((char *) &dg->xtc) Xtc(TypeId(TypeId::Id_Xtc, 1), *evtInfo);

    seg = new (&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc, 1), *segInfo);
}

static char *get_sml_config(int threshold)
{
    static char buf[SML_CONFIG_SIZE];
    memset(buf, 0, sizeof(buf));
    Xtc *xtc = new ((char *) buf) Xtc(TypeId(TypeId::Id_SmlDataConfig, 1),
                                      Src(Level::Recorder));
    new (xtc->alloc(sizeof(SmlData::ConfigV1)))
        SmlData::ConfigV1(threshold);
    return buf;
}

static char *get_sml_offset(int64_t offset, uint32_t extent)
{
    static char buf[SML_OFFSET_SIZE];
    memset(buf, 0, sizeof(buf));
    Xtc *xtc = new ((char *) buf) Xtc(TypeId(TypeId::Id_SmlDataOrigDgramOffset, 1),
                                      Src(Level::Recorder));
    new (xtc->alloc(sizeof(SmlData::OrigDgramOffsetV1)))
         SmlData::OrigDgramOffsetV1(offset, extent);
    return buf;
}

static char *get_sml_proxy(int64_t offset, uint32_t extent, Src &src, int iswv8)
{
    static char buf[SML_PROXY_SIZE];
    memset(buf, 0, sizeof(buf));
    Xtc *xtc = new ((char *) buf) Xtc(TypeId(TypeId::Id_SmlDataProxy, 1), src);
    new (xtc->alloc(sizeof(SmlData::ProxyV1)))
        SmlData::ProxyV1(offset, iswv8 ? TypeId(TypeId::Id_Generic1DData, 0) : TypeId(TypeId::Id_Frame, 1), extent);
    return buf;
}

static int myfwrite(void *buf, int size)
{
    if (!fwrite(buf, size, 1, fp))
        return 0;
    fsize += size;
    fflush(fp);

    if (!fwrite(buf, size, 1, sfp))
        return 0;
    sfsize += size;
    fflush(sfp);

    return 1;
}

static void write_datagram(TransitionId::Value val, int extra, int diff)
{
    sigset_t oldsig;

    setup_datagram(val);

    seg->extent    += extra;
    dg->xtc.extent += extra;

    sigprocmask(SIG_BLOCK, &blockset, &oldsig);
    if (write_xtc) {
        if (!fwrite(dg, sizeof(Dgram) + sizeof(Xtc), 1, fp)) {
            printf("Write failed!\n");
            fprintf(stderr, "error Write to %s failed.\n", fname);
            exit(1);
        }
        fsize += sizeof(Dgram) + sizeof(Xtc);
        fflush(fp);
    }

    seg->extent    += diff;
    dg->xtc.extent += diff;

    if (write_xtc) {
        if (!fwrite(dg, sizeof(Dgram) + sizeof(Xtc), 1, sfp)) {
            printf("Write failed!\n");
            fprintf(stderr, "error Write to %s failed.\n", fname);
            exit(1);
        }
        sfsize += sizeof(Dgram) + sizeof(Xtc);
        fflush(fp);
    }

    sigprocmask(SIG_SETMASK, &oldsig, NULL);
}

/*
 * This is called once we have all of the configuration data.
 */
static void write_xtc_config(void)
{
    int i;
    sigset_t oldsig;
    Xtc *xtcpv = NULL, *xtc1 = NULL, *xtc = NULL;

    dg = (Dgram *) malloc(sizeof(Dgram) + sizeof(Xtc));

    int pid = getpid();
    int ipaddr = 0x7f000001;  // Default to 127.0.0.1, if we can't find better.
    char hostname[256];
    if (!gethostname(hostname, sizeof(hostname))) {
        struct hostent *host = gethostbyname(hostname);
        if (host->h_addrtype == AF_INET && host->h_length == 4) {
            ipaddr =
                (((unsigned char *)host->h_addr)[0] << 24) |
                (((unsigned char *)host->h_addr)[1] << 16) |
                (((unsigned char *)host->h_addr)[2] << 8) |
                ((unsigned char *)host->h_addr)[3];
        }
    }

    evtInfo = new ProcInfo(Level::Event, pid, ipaddr);
    segInfo = new ProcInfo(Level::Segment, pid, ipaddr);
    ctrlInfo = new ProcInfo(Level::Control, pid, ipaddr);

    sigprocmask(SIG_BLOCK, &blockset, &oldsig);
    if (start_sec != 0) {
        csec = start_sec;
        cnsec = start_nsec;
        cfid = start_nsec & 0x1ffff;
    }

    if (write_hdf) {
        for (i = 0; i < numsrc; i++) {
            if (src[i]->hdf_id >= 0)
                configure_hdf(src[i]->hdf_id, 0, 0);
        }
    }

    if (write_xtc) {
        int cnt = alias.size();
        if (cnt) {
            int len = 2 * sizeof(Xtc) + sizeof(Alias::ConfigV1) + alias.size() * sizeof(Alias::SrcAlias);
            xtc1 = new ((char *) malloc(len)) Xtc(TypeId(TypeId::Id_Xtc, 1), *ctrlInfo);
            xtc1->extent = len;
            xtc = new ((char *) (xtc1+1)) Xtc(TypeId(TypeId::Id_AliasConfig, 1), *ctrlInfo);
            totalcfglen += len;
            printf("Alias info = %d bytes\n", len);
        } else
            printf("No alias info!\n");
        int pvcnt = pvalias.size();
        if (pvcnt) {
            int len = sizeof(Xtc) + sizeof(Epics::ConfigV1) + pvalias.size() * sizeof(Epics::PvConfigV1);
            xtcpv = new ((char *) malloc(len)) Xtc(TypeId(TypeId::Id_EpicsConfig, 1), pvinfo);
            totalcfglen += len;
            printf("PV Alias info = %d bytes\n", len);
        } else
            printf("No PV alias info!\n");

        write_datagram(TransitionId::Configure, totalcfglen, SML_CONFIG_SIZE);
        if (pvcnt) {
            *reinterpret_cast<uint32_t *>(xtcpv->alloc(sizeof(uint32_t))) = pvcnt;
            for (vector<Epics::PvConfigV1 *>::iterator it = pvalias.begin(); it != pvalias.end(); it++) {
                new (xtcpv->alloc(sizeof(Epics::PvConfigV1))) Epics::PvConfigV1(**it);
            }
            if (!myfwrite(xtcpv, xtcpv->extent)) {
                printf("Cannot write to file!\n");
                fprintf(stderr, "error Write to %s failed.\n", fname);
                exit(1);
            }
        }
        for (i = 0; i < numsrc; i++) {
            if (!myfwrite(src[i]->val, src[i]->len)) {
                printf("Cannot write to file!\n");
                fprintf(stderr, "error Write to %s failed.\n", fname);
                exit(1);
            }
            delete src[i]->val;
            src[i]->val = NULL;
            src[i]->len = 0;
            if (src[i]->len2) {
                src[i]->val = src[i]->val2;
                src[i]->sec = src[i]->sec2;
                src[i]->nsec = src[i]->nsec2;
                src[i]->ref = new int;
                *(src[i]->ref) = 1;
                src[i]->ev = find_event(src[i]->sec, src[i]->nsec);
#ifdef TRACE
                printf("%08x:%08x AI%d -> event %d\n", 
                       src[i]->sec, src[i]->nsec, src[i]->id, src[i]->ev->id);
#endif
            }
        }
        if (cnt) {
            *reinterpret_cast<uint32_t *>(xtc->alloc(sizeof(uint32_t))) = cnt;
            for (vector<Alias::SrcAlias *>::iterator it = alias.begin(); it != alias.end(); it++) {
                new (xtc->alloc(sizeof(Alias::SrcAlias))) Alias::SrcAlias(**it);
            }
            if (!myfwrite(xtc1, xtc1->extent)) {
                printf("Cannot write to file!\n");
                fprintf(stderr, "error Write to %s failed.\n", fname);
                exit(1);
            }
        }
        if (!fwrite(get_sml_config(1024), SML_CONFIG_SIZE, 1, sfp)) {
            printf("Cannot write to file!\n");
            fprintf(stderr, "error Write to %s failed.\n", sfname);
            exit(1);
        }
        sfsize += SML_CONFIG_SIZE;
    } else {
        // if we aren't writing XTC still cleanup config objects
        for (i = 0; i < numsrc; i++) {
            delete src[i]->val;
            src[i]->val = NULL;
            src[i]->len = 0;
            if (src[i]->len2) {
                delete src[i]->val2;
                src[i]->val2 = NULL;
                src[i]->len2 = 0;
            }
        }
    }

    sigprocmask(SIG_SETMASK, &oldsig, NULL);

    if (!havetransitions) {
        write_datagram(TransitionId::BeginRun,        0, 0);
        write_datagram(TransitionId::BeginCalibCycle, 0, 0);
        write_datagram(TransitionId::Enable,          0, 0);

        begin_run();
        started = 1;
        paused = 0;
    } else {
        /*
         * Replay what we've got so far!
         *
         * MCB: There is a race condition here, in that we might get
         * another transition after we finish the loop but before we
         * set cfgdone.  I think we can take our chances though.
         */
        for (i = 0; i < transidx; i++)
            do_transition(saved_trans[i].id, saved_trans[i].secs,
                          saved_trans[i].nsecs, saved_trans[i].fid, 1);
    }
    cfgdone = 1;
}

/*
 * This is called *after* the config file has been read and all of the
 * sources have been declared.  Therefore, we can use the global fp to
 * tell if we are running pre- or post-initialization.
 */
void initialize_xtc(char *outfile)
{
    int i;
    char *base;

    damagextc.damage = Damage::DroppedContribution;

    for (i = 0; i < MAX_EVENTS; i++) {
        events[i].id = i;
        events[i].valid = 0;
        events[i].critcnt = 0;
        events[i].synccnt = 0;
        events[i].damcnt = 0;
        events[i].sec = 0;
        events[i].nsec = 0;
        events[i].data = (unsigned char **) calloc(numsrc, sizeof(char *));
        events[i].ref  = (int **)  calloc(numsrc, sizeof(int *));
        events[i].prev = (i == 0) ? &events[MAX_EVENTS - 1] : &events[i - 1];
        events[i].next = (i == MAX_EVENTS - 1) ? &events[0] : &events[i + 1];
    }
    evlist = &events[0];

    /*
     * outfile can either end in ".xtc" or not.  Strip this off.
     */
    if (write_xtc) {
        i = strlen(outfile);
        if (i >= 4 && !strcmp(outfile + i - 4, ".xtc")) {
            i -= 4;
            outfile[i] = 0;
        }

        fname = new char[i + 8]; // Add space for "-cNN.xtc"
        sprintf(fname, "%s-c00.xtc", outfile);
        cpos = fname + i;
        base = rindex(fname, '/');

        sfname = new char[i + 22]; // Add space for "smalldata/" and "-cNN.smd.xtc"
        if (base) {
            *base++ = 0;
            sprintf(sfname, "%s/smalldata/%s", fname, base);
            *--base = '/';
        } else {
            sprintf(sfname, "smalldata/%s", fname);
        }
        strcpy(sfname + i + 10, "-c00.smd.xtc");
        cspos = sfname + i + 10;

        if (!(fp = myfopen(fname, "w", 1))) {
            printf("Cannot open %s for output!\n", fname);
            fprintf(stderr, "error Open %s failed.\n", fname);
            exit(0);
        } else
            printf("Opened %s for writing.\n", fname);
        if (!(sfp = myfopen(sfname, "w", 1))) {
            printf("Cannot open %s for output!\n", sfname);
            fprintf(stderr, "error Open %s failed.\n", sfname);
            exit(0);
        } else
            printf("Opened %s for writing.\n", sfname);
        iname = new char[i + 25];
        if (base) {
            *base++ = 0;
            sprintf(iname, "%s/index/%s.idx", fname, base);
            *--base = '/';
        } else {
            sprintf(iname, "index/%s.idx", fname);
        }
        cipos = iname + strlen(iname) - 12; // "-c00.xtc.idx"
        _indexList.reset();
        _indexList.setXtcFilename(fname);
    }

    isinit = 1; // finished xtc initialization

    sigemptyset(&blockset);
    sigaddset(&blockset, SIGALRM);
    sigaddset(&blockset, SIGINT);

    /*
     * If all of the configuration information has come in, write it!
     */
    if (cfgcnt == numsrc && csec != 0) {
        write_xtc_config();
    }
}

/*
 * Generate a unique id for this source.
 */
int register_xtc(int sync, string name, int critical, int isbig, int iswv8)
{
    string dname = name;
    // Make all of the names equal length!
    if (name.length() > maxname) {
        maxname = name.length();
        for (int i = 0; i < numsrc; i++) {
            while (src[i]->name.length() != maxname)
                src[i]->name.append(" ");
        }
    }
    while (name.length() != maxname)
        name.append(" ");
    src.push_back(new xtcsrc(numsrc, sync, name, critical, isbig, iswv8));
    if (sync)
        syncsrc++;
    else
        asyncsrc++;
    if (critical)
        critsrc++;
    return numsrc++;
}

/*
 * Add a unique hdf5 writer id to this source.
 */
void register_hdf_writer(int id, int hid)
{
    src[id]->hdf_id = hid;
}

/*
 * Create an alias.
 */
void register_alias(std::string name, DetInfo &sourceInfo)
{
    alias.push_back(new Alias::SrcAlias(sourceInfo, name.c_str()));
}

/*
 * Create a PV alias.
 */
void register_pv_alias(std::string name, int idx, DetInfo &sourceInfo)
{
    pvinfo = sourceInfo;
    pvalias.push_back(new Epics::PvConfigV1(idx, name.c_str(), 0.0));
}

/*
 * Give the configuration Xtc for a particular source.
 */
void configure_xtc(int id, char *xtc, int size, unsigned int secs, unsigned int nsecs)
{
    if (pthread_mutex_trylock(&datalock)) {
        printf("configure_xtc(%p) can't get lock!\n", (void *)pthread_self());
        pthread_mutex_lock(&datalock);
    }
#ifdef TRACE
    printf("%08x:%08x C %d\n", secs, nsecs, id);
#endif
    src[id]->cnt++;
    src[id]->val = new unsigned char[size];
    src[id]->len = size;
    src[id]->src = ((Xtc *)xtc)->src;
    totalcfglen += size;
    memcpy((void *)src[id]->val, (void *)xtc, size);
    cfgcnt++;
    if (csec == 0 && cnsec == 0) {
        /*
         * For channel access, secs and nsecs are zero.  So this must be a BLD,
         * which means we *aren't* running in DAQ mode and have no transitions!
         */
        csec = secs;
        cnsec = nsecs;
        cfid = nsecs & 0x1ffff;
    }

    /*
     * If we have already finished initialization and were waiting for this,
     * write the configuration!
     */
    if (isinit && cfgcnt == numsrc && csec != 0) {
        write_xtc_config();
    }
    pthread_mutex_unlock(&datalock);
}

static void reset_event(struct event *ev)
{
    int i;

#ifdef TRACE
    printf("%08x:%08x R %d\n", ev->sec, ev->nsec, ev->id);
#endif
    for (i = 0; i < numsrc; i++) {
        if (ev->data[i]) {
            if (!ev->ref[i])              // Synchronous!
                delete ev->data[i];
            else if (!--(*ev->ref[i])) {  // Asynchronous, and no more refs!
                delete ev->ref[i];
                ev->ref[i] = NULL;
                delete ev->data[i];
            }
            ev->data[i] = NULL;
        }
    }
    ev->critcnt = 0;
    ev->synccnt = 0;
    ev->damcnt = 0;
    ev->valid = 0;
}

static void write_idx_file()
{
    _indexList.finishList();
    int fd = open(iname, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    _indexList.writeToFile(fd);
    close(fd);
}

/*
 * OK, when we get here, dg is partially set up.  The outer xtcs (dg->xtc and seg) have their
 * extent set to the total data size (totaldlen).
 */
void send_event(struct event *ev)
{
    sigset_t oldsig;
    int damagesize = 0;
    int sdamagesize = 0;
    unsigned int segext, dgxext;
    bool bInvalidData = false;
    bool bStopUpdate = false;

    if (paused || (ev->nsec & 0x1ffff) == 0x1ffff) {
        /* Do nothing if we are paused or if the fiducial is bad! */
#ifdef TRACE
        printf("%08x:%08x D1 event %d\n", ev->sec, ev->nsec, ev->id);
#endif
        dsec = ev->sec;
        dnsec = ev->nsec;
        return;
    }
    if (dsec == ev->sec && dnsec == ev->nsec) {
        printf("%08x:%08x Sending duplicate data id=%d\n", ev->sec, ev->nsec, ev->id);
#ifdef DUPLICATE_SUPPRESSION
        /* Do nothing if this is exactly what we just sent! */
        return;
#endif /* DUPLICATE_SUPPRESSION */
    }
    // When do we *not* send an event?
    // If we have any critical sources in the event, we always send it,
    // even if damaged.
    // But if nothing is critical, we send it if it is complete.  We know 
    // we have asynchronous values, so we only need to check the synchronous
    // ones.
    if (critsrc ? (!ev->critcnt) : (ev->synccnt != syncsrc))
        return;

    // Complete this as best as we can.
    for (int i = 0; i < numsrc; i++) {
        if (!ev->data[i] && !src[i]->sync) {
            ev->data[i] = src[i]->val;
            ev->ref[i]  = src[i]->ref;
            (*ev->ref[i])++;
        }
        if (!ev->data[i]) {
            damagesize += src[i]->len - sizeof(Xtc); // Instead of the full data, we're going to write
                                                     // an empty Xtc!
            if (src[i]->isbig)
                sdamagesize += SML_PROXY_SIZE - sizeof(Xtc);
            else
                sdamagesize += src[i]->len - sizeof(Xtc);
            src[i]->damagecnt++;
        }
    }
    pthread_mutex_lock(&fdlock);
    if (paused) {   // OOPS, lost the race!
        pthread_mutex_unlock(&fdlock);
#ifdef TRACE
        printf("%08x:%08x D2 event %d\n", ev->sec, ev->nsec, ev->id);
#endif
        dsec = ev->sec;
        dnsec = ev->nsec;
        return;
    }

    // OK, from here on out it's write or die!
    segext = seg->extent;
    dgxext = dg->xtc.extent;
    if (damagesize) {
        seg->extent    -= damagesize;
        dg->xtc.extent -= damagesize;
        seg->damage     = Damage::DroppedContribution;
        dg->xtc.damage  = Damage::DroppedContribution;
    }

#ifdef TRACE
    printf("%08x:%08x T event %d\n", ev->sec, ev->nsec, ev->id);
#endif
    new ((void *) &dg->seq) Sequence(Sequence::Event, TransitionId::L1Accept,
                                     ClockTime(ev->sec, ev->nsec),
                                     TimeStamp(0, ev->nsec & 0x1ffff, 0, 0));
    dsec = ev->sec;
    dnsec = ev->nsec;

    sigprocmask(SIG_BLOCK, &blockset, &oldsig);

    /* Write hdf5 first before */
    if (write_hdf) {
        for (int i = 0; i < numsrc; i++) {
            if (src[i]->hdf_id < 0) continue;

            if (ev->data[i]) {
                data_hdf(src[i]->hdf_id, ev->sec, ev->nsec, &ev->data[i][src[i]->hdrlen]);
            } else {
                //TODO damage
            }
        }
    }

    /* Now write out the xtc */
    if (write_xtc) {
        _indexList.startNewNode(*dg, fsize, bInvalidData);

        int64_t svoff = fsize;

        if (!fwrite(dg, sizeof(Dgram) + sizeof(Xtc), 1, fp)) {
            printf("Write failed!\n");
            fprintf(stderr, "error Write to %s failed.\n", fname);
            exit(1);
        }
        fsize += sizeof(Dgram) + sizeof(Xtc);

        uint32_t svext = dg->xtc.extent;

        /* Remove the full size, add in the small size. */
        seg->extent    += totalsdlen - totaldlen;
        dg->xtc.extent += totalsdlen - totaldlen;
        if (damagesize) {
            seg->extent    += damagesize - sdamagesize;
            dg->xtc.extent += damagesize - sdamagesize;
        }

        if (!fwrite(dg, sizeof(Dgram) + sizeof(Xtc), 1, sfp)) {
            printf("Write failed!\n");
            fprintf(stderr, "error Write to %s failed.\n", fname);
            exit(1);
        }
        sfsize += sizeof(Dgram) + sizeof(Xtc);

        if (!fwrite(get_sml_offset(svoff, svext), SML_OFFSET_SIZE, 1, sfp)) {
            printf("Cannot write to file!\n");
            fprintf(stderr, "error Write to %s failed.\n", sfname);
            exit(1);
        }
        sfsize += SML_OFFSET_SIZE;

        if (!bInvalidData)
            _indexList.updateSegment(*seg);

    }

    /* Restore the datagram! */
    seg->extent = segext;
    dg->xtc.extent = dgxext;
    if (damagesize) {
        seg->damage     = 0;
        dg->xtc.damage  = 0;
    }

    if (write_xtc) {
        for (int i = 0; i < numsrc; i++) {
            if (ev->data[i]) {
                if (!bInvalidData && !bStopUpdate)
                    _indexList.updateSource(*(Xtc *)ev->data[i], bStopUpdate);
                if (src[i]->isbig) {
                    if (!fwrite(ev->data[i], src[i]->len, 1, fp)) {
                        printf("Write failed!\n");
                        fprintf(stderr, "error Write to %s failed.\n", sfname);
                        exit(1);
                    }
                    if (!fwrite(get_sml_proxy(fsize, src[i]->len, src[i]->src, src[i]->iswv8),
                                SML_PROXY_SIZE, 1, sfp)) {
                        printf("Write failed!\n");
                        fprintf(stderr, "error Write to %s failed.\n", sfname);
                        exit(1);
                    }
                    fsize += src[i]->len;
                    sfsize += SML_PROXY_SIZE;
                } else {
                    if (!myfwrite(ev->data[i], src[i]->len)) {
                        printf("Write failed!\n");
                        fprintf(stderr, "error Write to %s failed.\n", fname);
                        exit(1);
                    }
                }
            } else {
                damagextc.src = src[i]->src;
                if (!bInvalidData && !bStopUpdate)
                    _indexList.updateSource(damagextc, bStopUpdate);
                if (!myfwrite(&damagextc, sizeof(Xtc))) {
                    printf("Write failed!\n");
                    fprintf(stderr, "error Write to %s failed.\n", fname);
                    exit(1);
                }
            }
        }
        if (!bInvalidData) {
            bool bPrintNode = false;
            _indexList.finishNode(bPrintNode);
        }
        fflush(fp);
    }
    sigprocmask(SIG_SETMASK, &oldsig, NULL);
    if (fsize >= CHUNK_SIZE && write_xtc) { /* Next chunk! */
        write_idx_file();
        fclose(fp);
        fclose(sfp);
        sprintf(cpos, "-c%02d.xtc", ++chunk);
        sprintf(cspos, "-c%02d.smd.xtc", chunk);
        sprintf(cipos, "-c%02d.xtc.idx", chunk);
        if (!(fp = myfopen(fname, "w", 1))) {
            printf("Cannot open %s for output!\n", fname);
            fprintf(stderr, "error Open %s failed.\n", fname);
            exit(0);
        } else
            printf("Opened %s for writing.\n", fname);
        if (!(sfp = myfopen(sfname, "w", 0))) {
            printf("Cannot open %s for output!\n", sfname);
            fprintf(stderr, "error Open %s failed.\n", sfname);
            exit(0);
        } else
            printf("Opened %s for writing.\n", sfname);
        _indexList.reset();
        _indexList.setXtcFilename(fname);
        fsize = 0;
    }
    record_cnt++;
    pthread_mutex_unlock(&fdlock);   // I guess it was write. :-)
}

static struct event *find_event(unsigned int sec, unsigned int nsec)
{
    struct event *ev;
    int match;
    ev = evlist->prev;
    do {
        if (!(match = ts_match(ev, sec, nsec)))
            return ev;
        if (match < 0)
            break;
        ev = ev->prev;
    } while (ev != evlist->prev);

    /*
     * No match.  If we are here, either match < 0, in which case ev is
     * the event immediately before the new event, or every single event
     * in the list is newer than this one.  In this rare case, we'll return
     * NULL and just skip this.
     */
    if (match > 0)
        return NULL;
    /*
     * evlist is the oldest event, so we want to delete it and reuse it.
     * But first we want to see if we can complete it with asynchronous
     * events.
     */
    if (evlist->valid)
        send_event(evlist);
    reset_event(evlist);
    /*
     * In the most common case, ev == evlist->prev and we don't have to
     * relink anything!  In a more rare case, if ev == evlist, we delete
     * the oldest only to replace it with a *new* oldest, leaving the
     * links untouched.
     */
    if (ev == evlist->prev) {
        evlist = evlist->next; /* Delete and add as the newest in one step! */
        ev = ev->next;         /* Let ev point to the new element */
    } else if (ev != evlist) {
        struct event *tmp = evlist;

        /* Unlink tmp (evlist) from the list and move ahead evlist. */
        evlist = tmp->next;
        tmp->prev->next = evlist;
        evlist->prev = tmp->prev;

        /* Link tmp into the list after ev. */
        tmp->next = ev->next;
        ev->next->prev = tmp;
        tmp->prev = ev;
        ev->next = tmp;
#ifdef TRACE
        printf("%08x:%08x event %d moved after %08x:%08x event %d\n",
               sec, nsec, tmp->id, ev->sec, ev->nsec, ev->id);
        tmp->sec = sec;
        tmp->nsec = nsec;
        tmp->valid = 1;
        debug_list(evlist);
#endif

        /* Let ev point to the new element. */
        ev = tmp;
    }
    ev->sec = sec;
    ev->nsec = nsec;
    ev->valid = 1;
    for (int i = 0; i < numsrc; i++) {
        if (src[i]->sync && src[i]->ev && ts_match(src[i]->ev, sec, nsec) > 0)
            ev->damcnt++;
    }
    if (length_list(evlist) != MAX_EVENTS) {
        printf("OOPS! length=%d\n", length_list(evlist));
        debug_list(evlist);
    }
    return ev;
}

/*
 * Give the data Xtc for a particular source.  We assume that the Xtc header is
 * in one area and the data is in another, so we pass in a second pointer.  That
 * is, hdr contains hdrlen bytes, and data contains (hdr->extent - hdrlen) bytes.
 */
void data_xtc(int id, unsigned int sec, unsigned int nsec, Pds::Xtc *hdr, int hdrlen, void *data)
{
    xtcsrc *s = src[id];
    struct event *ev, *cur;

    if (pthread_mutex_trylock(&datalock)) {
        printf("data_xtc(%p) can't get lock!\n", (void *)pthread_self());
        pthread_mutex_lock(&datalock);
    }
    s->cnt++;
    /*
     * Just go home if:
     *    - We aren't running yet.
     *    - We don't have all of the asynchronous PVs.
     */
    if (s->sync && haveasync != asyncsrc) {
        pthread_mutex_unlock(&datalock);
        return;
    }
    if (!started) {
        /*
         * Only BLD configurations give us a timestamp.  So if we are only recording
         * cameras and PVs, we have delayed writing the configuration until now.
         * In this case, we have finished with initialize_xtc (isinit) and
         * we have all of the configuration information (cfgcnt == numsrc), but
         * we don't have a timestamp (csec == 0).
         */
        if (isinit && cfgcnt == numsrc && csec == 0) {
            if (!havetransitions || !transidx) {
                csec = sec;
                cnsec = nsec;
            } else {
                int i;
                for (i = 0; i < transidx; i++)
                    if (saved_trans[i].id == TransitionId::Configure)
                        break;
                if (i != transidx) {
                    csec = saved_trans[i].secs;
                    cnsec = saved_trans[i].nsecs;
                } else {
                    csec = sec;
                    cnsec = nsec;
                }
            }
            cfid = 0x1ffff;
            write_xtc_config();
        } else {
            if (!s->sync) {
                // OK, asynchronous data might be static!  So it's now
                // or never!!
                unsigned char *buf;
                int inc = 0;
                if (s->len2)
                    buf = s->val2;
                else {
                    buf = new unsigned char[hdr->extent];
                    inc = 1;
                }
                memcpy(buf, hdr, hdrlen);
                if (data)
                    memcpy(&buf[hdrlen], data, hdr->extent - hdrlen);
                s->len2 = hdr->extent;
                s->val2 = buf;
                s->sec2 = sec;
                s->nsec2 = nsec;
#ifdef TRACE
                printf("%08x:%08x AI%d -> caching %d!\n", sec, nsec, id, inc);
#endif
                haveasync += inc;
            }
            pthread_mutex_unlock(&datalock);
            return;
        }
    }

    /*
     * Make the data buffer.
     */
    unsigned char *buf = new unsigned char[hdr->extent];
    memcpy(buf, hdr, hdrlen);
    if (data)
        memcpy(&buf[hdrlen], data, hdr->extent - hdrlen);

    if (!s->len) { // First time we've seen this data!
        s->len = hdr->extent;
        s->hdrlen = hdrlen;
        totaldlen += hdr->extent;
        if (s->isbig)
            totalsdlen += SML_PROXY_SIZE;
        else
            totalsdlen += hdr->extent;
        if (++havedata == numsrc) {
            // Initialize the header now that we know its length.
            setup_datagram(TransitionId::L1Accept);
            seg->extent    += totaldlen;
            dg->xtc.extent += totaldlen;
        }
        if (!s->sync) {  // Just save asynchronous data for now.
            s->val = buf;
            s->sec = sec;
            s->nsec = nsec;
            s->ref = new int;
            *(s->ref) = 1;
            s->ev = find_event(sec, nsec);
#ifdef TRACE
            printf("%08x:%08x AI%d -> event %d\n", sec, nsec, id, s->ev->id);
#endif
            if (!s->val2)
                haveasync++;
            pthread_mutex_unlock(&datalock);
            return;
        }
    }

    /*
     * At this point, we know we have all of the asynchronous values.
     */
    if (!(ev = find_event(sec, nsec))) {
        delete buf;
        pthread_mutex_unlock(&datalock);
        return;
    }
    if (ev->data[id]) {
        /* Duplicate data.  This is *not* good. */
#if 1
        printf("%08x:%08x received duplicate data (id=%d, ev->id=%d)\n", sec, nsec, id, ev->id);
#else
        printf("%08x:%08x S%d -> DUPLICATE event %d\n", sec, nsec, id, ev->id);
        printf("OLD: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
               ev->data[id][0], ev->data[id][1], ev->data[id][2], ev->data[id][3],
               ev->data[id][4], ev->data[id][5], ev->data[id][6], ev->data[id][7],
               ev->data[id][8], ev->data[id][9], ev->data[id][10], ev->data[id][11],
               ev->data[id][12], ev->data[id][13], ev->data[id][14], ev->data[id][15]);
        printf("NEW: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
               buf[0], buf[1], buf[2], buf[3],
               buf[4], buf[5], buf[6], buf[7],
               buf[8], buf[9], buf[10], buf[11],
               buf[12], buf[13], buf[14], buf[15]);
#endif
        delete buf;
        pthread_mutex_unlock(&datalock);
        return;
    }

    if (s->critical)
        ev->critcnt++;

    /*
     * If the event associated with the data doesn't have the same timestamp, we must have
     * reused it.  In this case, we can start with the oldest!
     */
    cur = s->ev;
    if (!cur || cur->sec != s->sec || cur->nsec != s->nsec)
        cur = evlist;
    else
        cur = cur->next;
#ifdef TRACE
    printf("%08x:%08x %c%d -> event %d (%d, %d)\n",
           sec, nsec, s->sync ? 'S' : 'A', id,
           ev->id, ev->synccnt, ev->damcnt);
#endif

    /* Iterate over all events from the last time we received this source until now. */
    while (cur != ev) {
        if (cur->valid) {
#ifdef TRACE
            printf("                  event %d\n", cur->id);
#endif
            if (s->sync) {  // Synchronous -> call anything missing damage!
                if (++cur->damcnt + cur->synccnt == syncsrc) {
                    send_event(cur);
                    reset_event(cur);
                }
            } else {        // Asynchronous -> fill in with the previous data!
                cur->data[id] = s->val;
                cur->ref[id]  = s->ref;
                (*s->ref)++;
            }
        }
        cur = cur->next;
    }

    ev->data[id] = buf;
    if (s->sync) {          // Synchronous -> count one more piece of data.
        if (ev->damcnt + ++ev->synccnt == syncsrc) {
            send_event(ev);
            reset_event(ev);
        }
    } else {                // Asynchronous -> save it to fill in set ref counts.
        if (!--(*s->ref)) {
            delete s->val;
            delete s->ref;
        }
        s->val = buf;
        s->ref = new int;
        *(s->ref) = 1;
        cur->ref[id] = s->ref;
        (*s->ref)++;
    }
    s->sec = sec;
    s->nsec = nsec;
    s->ev = ev;
    pthread_mutex_unlock(&datalock);
}

void cleanup_xtc(void)
{
    if (dg) {
        if (end_sec != 0) {
            csec = end_sec;
            cnsec = end_nsec;
            cfid = end_nsec & 0x1ffff;
        } else {
            csec = dsec;
            cnsec = dnsec;
            cfid = dnsec & 0x1ffff;
        }
        write_datagram(TransitionId::Disable,       0, 0);
        write_datagram(TransitionId::EndCalibCycle, 0, 0);
        write_datagram(TransitionId::EndRun,        0, 0);
        write_datagram(TransitionId::Unconfigure,   0, 0);
    }
}

void cleanup_index(void)
{
    if (fp) {
        fflush(fp);
        fflush(sfp);
        write_idx_file();
        fclose(fp);
        fclose(sfp);
    }
}

void xtc_stats(void)
{
    for (int i = 0; i < numsrc; i++) {
        fprintf(stderr, "     %s     %5d received\n", src[i]->name.c_str(), src[i]->cnt);
    }
    fflush(stderr);
}

char *trans_name(int state)
{
    switch (state) {
    case -1:
        return (char *) "startup";
    case TransitionId::Configure:
        return (char *) "Configure";
    case TransitionId::BeginCalibCycle:
        return (char *) "BeginCalibCycle";
    case TransitionId::BeginRun:
        return (char *) "BeginRun";
    case TransitionId::Enable:
        return (char *) "Enable";
    case TransitionId::Disable:
        return (char *) "Disable";
    case TransitionId::EndRun:
        return (char *) "EndRun";
    case TransitionId::EndCalibCycle:
        return (char *) "EndCalibCycle";
    case TransitionId::Unconfigure:
        return (char *) "Unconfigure";
    default:
        return (char *) "Unknown";
    }
}

#define TRANS_ERROR_BODY                                                 \
        fprintf(stderr, "error Received %s (%d) transition after %s!\n", \
                trans_name(id), id, trans_name(lasttrans));              \
        exit(1);                                                         \
    } else {                                                             \
        lasttrans = id;                                                  \
    }

#define TRANS_ERROR1(ex1)                                                \
    if (lasttrans != (ex1)) { TRANS_ERROR_BODY

#define TRANS_ERROR2(ex1,ex2)                                            \
    if (lasttrans != (ex1) && lasttrans != (ex2)) { TRANS_ERROR_BODY

void do_transition(int id, unsigned int secs, unsigned int nsecs, unsigned int fid, int force)
{
    havetransitions = 1;
    if (!force && !cfgdone) { /* If we're not initialized, just save it! */
        saved_trans[transidx].id = id;
        saved_trans[transidx].secs = secs;
        saved_trans[transidx].nsecs = nsecs;
        saved_trans[transidx].fid = fid;
        transidx++;
        return;
    }
    printf("TRANS: %d.%09d (0x%x) %s\n", secs, nsecs, fid,
           TransitionId::name((TransitionId::Value) id));
    csec = secs;
    cnsec = nsecs;
    cfid = fid;
    pthread_mutex_lock(&fdlock);    // Can't be in the middle of an L1Accept after this!
    switch (id) {
    case TransitionId::Configure:
        TRANS_ERROR1(-1);
        /*
         * We don't have to do anything, since we are breaking out of
         * read_config_file and will immediately call initialize_xtc,
         * which will write this transition.
         */
        break;
    case TransitionId::BeginRun:
        write_datagram(TransitionId::BeginRun, 0, 0);
        TRANS_ERROR2(TransitionId::Configure, TransitionId::EndRun);
        break;
    case TransitionId::BeginCalibCycle:
        write_datagram(TransitionId::BeginCalibCycle, 0, 0);
        TRANS_ERROR2(TransitionId::BeginRun, TransitionId::EndCalibCycle);
        break;
    case TransitionId::Enable:
        write_datagram(TransitionId::Enable, 0, 0);
        TRANS_ERROR2(TransitionId::BeginCalibCycle, TransitionId::Disable);
        if (!started)
            begin_run();
        if (havedata == numsrc) {
            // Re-initialize the header!
            setup_datagram(TransitionId::L1Accept);
            seg->extent    += totaldlen;
            dg->xtc.extent += totaldlen;
        }
        started = 1;
        paused = 0;
        break;
    case TransitionId::Disable:
        write_datagram(TransitionId::Disable, 0, 0);
        TRANS_ERROR1(TransitionId::Enable);
        paused = 1;
        break;
    case TransitionId::EndCalibCycle:
        write_datagram(TransitionId::EndCalibCycle, 0, 0);
        TRANS_ERROR2(TransitionId::BeginCalibCycle, TransitionId::Disable);
        break;
    case TransitionId::EndRun:
        write_datagram(TransitionId::EndRun, 0, 0);
        TRANS_ERROR2(TransitionId::BeginRun, TransitionId::EndCalibCycle);
        cleanup_ca();
        cleanup_index();
        exit(0);
    }
    pthread_mutex_unlock(&fdlock);
}

char *damage_report(void)
{
    static char buf[1024];
    char *s = buf;
    int i;

    sprintf(s, "dstat %d", record_cnt);
    s += strlen(s);
    for (i = 0; i < numsrc; i++) {
        if (src[i]->critical) {
            sprintf(s, " %d", src[i]->damagecnt);
            s += strlen(s);
        }
    }
    return buf;
}
