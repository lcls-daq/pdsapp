#define EXACT_TS_MATCH       0
#define FIDUCIAL_MATCH       1
#define APPROXIMATE_MATCH    2
#define MATCH_TYPE           FIDUCIAL_MATCH
// #define TRACE
#include<stdio.h>
#include<signal.h>
#include<string.h>
#include<math.h>

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

struct event;

class xtcsrc {
 public:
    xtcsrc(int _id, int _sync) : id(_id), sync(_sync), val(NULL),
                                 len(0), ref(NULL), sec(0), nsec(0), ev(NULL) {};
    int   id;
    int   sync;                   // Is this a synchronous source?
    char *val;
    int   len;
    int  *ref;                    // Reference count of this value (if asynchronous!)
    unsigned int sec, nsec;       // Timestamp of this value
    struct event *ev;             // Event for this value (if asynchronous!)
};

FILE *fp = NULL;
static sigset_t blockset;
static int started = 0;        // We are actually running.
static int numsrc = 0;         // Total number of sources.
static int syncsrc = 0;        // Number of synchronous sources.
static int asyncsrc = 0;       // Number of asynchronous sources.
static int havedata = 0;       // Number of sources that sent us a value.
static int haveasync = 0;      // Number of asynchronous sources that sent us a value.
static int cfgcnt = 0;         // Number of sources that sent us their configuration.
static int totalcfglen = 0;    // Total number of bytes in all of the configuration records.
static int totaldlen   = 0;    // Total number of bytes in all of the data records.
static vector<xtcsrc *> src;

static Dgram *dg = NULL;
static Xtc   *seg = NULL;
static ProcInfo *evtInfo = NULL;
static ProcInfo *segInfo = NULL;

#define MAX_EVENTS    64
static struct event {
    int    id;
    int    valid;
    int    synccnt;               // How many synchronous sources do we have?
    int    asynccnt;              // How many asynchronous sources do we have?
    unsigned int sec, nsec;       // Timestamp of this event.
    char **data;                  // Array of numsrc data records.
    int  **ref;                   // An array of reference counts for asynchronous data.
    struct event *prev, *next;    // Linked list, sorted by timestamp.
} events[MAX_EVENTS];
static struct event *evlist = NULL; // The oldest event.  Its prev is the newest event.

void debug_list(struct event *ev)
{
    struct event *e = ev;

    do {
        printf("%2d: %08x:%08x\n", e->id, e->sec, e->nsec);
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

static int ts_match(struct event *_ev, int _sec, int _nsec)
{
#if MATCH_TYPE == EXACT_TS_MATCH
    /* Exactly match the timestamps */
    return (_ev->sec != _sec) ? ((int)_ev->sec - _sec) : ((int)_ev->nsec - _nsec);
#endif

#if MATCH_TYPE == FIDUCIAL_MATCH
    /* Exactly match the fiducials, and make the seconds be close. */
    int diff = _ev->sec - _sec;
    if (abs(diff) <= 1) /* Close to the same second! */
        return (0x1ffff & (int)_ev->nsec) - (0x1ffff & _nsec);
    else
        return diff;
#endif

#if MATCH_TYPE == APPROXIMATE_MATCH
    /* Come within 6ms */
    double diff = (double)((int)(_ev->sec - _sec)) + ((double)(int)(_ev->nsec - _nsec))*1.0e-9;
    return (fabs(diff) < 6.0e-3) ? 0 : ((diff < 0) ? -1 : 0);
#endif
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
        if (!fwrite(src[i]->val, src[i]->len, 1, fp)) {
            fprintf(stderr, "Cannot write to file!\n");
            exit(1);
        }
        fflush(fp);
        delete src[i]->val;
        src[i]->val = NULL;
        src[i]->len = 0;
    }
    sigprocmask(SIG_SETMASK, &oldsig, NULL);

    write_datagram(TransitionId::BeginRun,        0);
    write_datagram(TransitionId::BeginCalibCycle, 0);
    write_datagram(TransitionId::Enable,          0);

    begin_run();
    started = 1;
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
        events[i].id = i;
        events[i].valid = 0;
        events[i].synccnt = 0;
        events[i].asynccnt = 0;
        events[i].sec = 0;
        events[i].nsec = 0;
        events[i].data = (char **) calloc(numsrc, sizeof(char *));
        events[i].ref  = (int **)  calloc(numsrc, sizeof(int *));
        events[i].prev = (i == 0) ? &events[MAX_EVENTS - 1] : &events[i - 1];
        events[i].next = (i == MAX_EVENTS - 1) ? &events[0] : &events[i + 1];
    }
    evlist = &events[0];

    if (!strcmp(outfile, "-"))
        fp = stdout;
    else if (!(fp = fopen(outfile, "w"))) {
        fprintf(stderr, "Cannot open %s for output!\n", outfile);
        exit(0);
    }

    sigemptyset(&blockset);
    sigaddset(&blockset, SIGALRM);
    sigaddset(&blockset, SIGINT);

    /*
     * If all of the configuration information has come in, write it!
     */
    if (cfgcnt == numsrc) {
        write_xtc_config();
    }
}

/*
 * Generate a unique id for this source.
 */
int register_xtc(int sync)
{
    src.push_back(new xtcsrc(numsrc, sync));
    if (sync)
        syncsrc++;
    else
        asyncsrc++;
    return numsrc++;
}

/*
 * Give the configuration Xtc for a particular source.
 */
void configure_xtc(int id, Pds::Xtc *xtc)
{
    int size = xtc->extent;
    src[id]->val = new char[size];
    src[id]->len = size;
    totalcfglen += size;
    memcpy((void *)src[id]->val, (void *)xtc, size);
    cfgcnt++;

    /*
     * If we have already finished initialization and were waiting for this,
     * write the configuration!
     */
    if (fp != NULL && cfgcnt == numsrc) {
        write_xtc_config();
    }
}

static void reset_event(struct event *ev)
{
    int i;

#ifdef TRACE
    printf("%08x:%08x R %d\n", ev->sec, ev->nsec, ev->id);
#endif
    for (i = 0; i < numsrc; i++) {
        if (ev->data[i]) {
            if (!ev->ref[i])
                delete ev->data[i];
            else if (!--(*ev->ref[i])) {
                delete ev->ref[i];
                ev->ref[i] = NULL;
                delete ev->data[i];
            }
            ev->data[i] = NULL;
        }
    }
    ev->synccnt = 0;
    ev->asynccnt = 0;
    ev->valid = 0;
}

void send_event(struct event *ev)
{
    sigset_t oldsig;
#ifdef TRACE
    printf("%08x:%08x T event %d\n", ev->sec, ev->nsec, ev->id);
#endif
    new ((void *) &dg->seq) Sequence(Sequence::Event, TransitionId::L1Accept, 
                                     ClockTime(ev->sec, ev->nsec), 
                                     TimeStamp(0, ev->nsec & 0x1ffff, 0, 0));

    sigprocmask(SIG_BLOCK, &blockset, &oldsig);
    if (!fwrite(dg, sizeof(Dgram) + sizeof(Xtc), 1, fp)) {
        fprintf(stderr, "Write failed!\n");
        exit(1);
    }
    for (int i = 0; i < numsrc; i++) {
        if (!fwrite(ev->data[i], src[i]->len, 1, fp)) {
            fprintf(stderr, "Write failed!\n");
            exit(1);
        }
    }
    fflush(fp);
    sigprocmask(SIG_SETMASK, &oldsig, NULL);
    record_cnt++;
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
    if (evlist->synccnt == syncsrc && evlist->valid) {
        for (int i = 0; i < numsrc; i++) {
            if (!evlist->data[i]) {
                evlist->data[i] = src[i]->val;
                evlist->ref[i]  = src[i]->ref;
                (*evlist->ref[i])++;
            }
        }
        send_event(evlist);
    }
#ifdef TRACE
    else if (evlist->valid)
        printf("%08x:%08x M %d\n", evlist->sec, evlist->nsec, evlist->id);
#endif
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

        /* Let ev point to the new element. */
        ev = tmp;
    }
    ev->sec = sec;
    ev->nsec = nsec;
    ev->valid = 1;
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
    struct event *ev;

    /*
     * Just go home if:
     *    - We aren't running yet.
     *    - We don't have all of the asynchronous PVs.
     */
    if (!started || (s->sync && haveasync != asyncsrc))
        return;

    /*
     * Make the data buffer.
     */
    char *buf = new char[hdr->extent];
    memcpy(buf, hdr, hdrlen);
    if (data)
        memcpy(&buf[hdrlen], data, hdr->extent - hdrlen);

    if (!s->len) { // First time we've seen this data!
        s->len = hdr->extent;
        totaldlen += hdr->extent;
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
            haveasync++;
            return;
        }
    }

    /*
     * At this point, we know we have all of the asynchronous values.
     */
    if (!(ev = find_event(sec, nsec))) {
        delete buf;
        return;
    }
    if (s->sync) {
        ev->data[id] = buf;
#ifdef TRACE
        printf("%08x:%08x S%d -> event %d\n", sec, nsec, id, ev->id);
#endif
        if (ev->valid && ++ev->synccnt == syncsrc && ev->asynccnt == asyncsrc) {
            send_event(ev);
            reset_event(ev);
        }
    } else {
        struct event *cur = s->ev;

        /*
         * If the event associated with the data doesn't have the same timestamp, we must have
         * reused it.  In this case, we can start with the oldest!
         */
        if (cur->sec != s->sec || cur->nsec != s->nsec)
            cur = evlist;
#ifdef TRACE
        printf("%08x:%08x A%d -> event %d\n", sec, nsec, id, ev->id);
#endif

        while (cur != ev) {
#ifdef TRACE
            printf("                  event %d\n", cur->id);
#endif
            cur->data[id] = s->val;
            cur->ref[id]  = s->ref;
            (*s->ref)++;
            if (cur->valid && ++cur->asynccnt == asyncsrc && cur->synccnt == syncsrc) {
                send_event(cur);
                reset_event(cur);
            }
            cur = cur->next;
        }
        if (!--(*s->ref)) {
            delete s->val;
            delete s->ref;
        }
        s->val = buf;
        s->sec = sec;
        s->nsec = nsec;
        s->ref = new int;
        *(s->ref) = 1;
        s->ev = ev;
    }
}

void cleanup_xtc(void)
{
    write_datagram(TransitionId::Disable,       0);
    write_datagram(TransitionId::EndCalibCycle, 0);
    write_datagram(TransitionId::EndRun,        0);
    write_datagram(TransitionId::Unconfigure,   0);
    fclose(fp);
}
