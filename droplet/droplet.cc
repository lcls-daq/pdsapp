/*
 * This is a droplet finder utility that runs through an XTC file, finds images, and runs the droplet
 * finder on it.
 */
#include<stdio.h>
#include<getopt.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include"pdsdata/xtc/DetInfo.hh"
#include"pdsdata/xtc/Xtc.hh"
#include"pdsdata/xtc/TypeId.hh"
#include"pdsdata/xtc/TimeStamp.hh"
#include"pdsdata/xtc/Dgram.hh"
#include"pdsdata/xtc/XtcFileIterator.hh"
#include"pdsdata/epics/EpicsPvData.hh"
#include"pdsdata/camera/FrameV1.hh"
#include"fakerecords.h"
#include <dlfcn.h>

using namespace std;
using namespace Pds;

#define CHUNK_SIZE 107374182400LL

char *infile = NULL;
char *cfgfile = NULL;
char *outfile = NULL;
char *location = NULL;
int infd = -1, outfd = -1;
int inchunk = 0, outchunk = 0;
long long inoff = 0, outoff = 0;
XtcFileIterator *iter = NULL;
Dgram *dg = NULL;

int verbose = 0;
int next_dg();

struct cfginfo {
    int width;
    int height;
    int roix;
    int roiy;
    int roiw;
    int roih;
    int ctrl;
    int lincnt;
    short bgimage[1024*1024];
    double param[25];
    double strength;
    int dx;
    int dy;
    int xproj[1024];
    int yproj[1024];
    int xlin[1024];
    int ylin[1024];
    short *iptr1;
    short *iptr2;
} cfg[2];

genSubRecord rec[2];
waveformRecord wf[2];

static long (*dd_process)(genSubRecord *pgsub) = NULL;
static long (*dd_init)(genSubRecord *pgsub) = NULL;
static void *dl_handle = NULL;

class PvConfig : public Xtc, public EpicsPvCtrl<DBR_LONG> {
public:
    PvConfig(DetInfo src, dbr_ctrl_long *val)
        : Xtc(TypeId(TypeId::Id_Epics, 1), src),
          EpicsPvCtrl<DBR_LONG>(0, 1, "DX1", val, NULL) {
        alloc(sizeof(EpicsPvCtrl<DBR_LONG>));
    };
};

struct PvValue : public Xtc, public EpicsPvTime<DBR_LONG> {
public:
    PvValue(DetInfo src, dbr_time_long *val)
        : Xtc(TypeId(TypeId::Id_Epics, 1), src),
          EpicsPvTime<DBR_LONG>(0, 1, val, NULL) {
        alloc(sizeof(EpicsPvTime<DBR_LONG>));
    };
};

void usage(void)
{
    printf("droplet -c CONFIG -o OUTPUT INPUT, or\n");
    printf("droplet -c CONFIG -l CHUNK:OFFSET INPUT\n\n");
    exit(1);
}

int chunk_open(char *name, int &chunk, int flags)
{
    char buf[1024];
    sprintf(buf, "%s-c%02d.xtc", name, chunk++);
    return open(buf, flags, 0644);
}

/* Fake, fake, fake!!! */
extern "C" int dbGetLinkValue(void *a, short b, void *c, void *d, void *e)
{
    return 0;
}

extern "C" int dbNameToAddr(char *name, struct dbAddr *addr)
{
    if (!strcmp(name, "IMAGE1") || !strcmp(name, "BGIMAGE1")) {
        addr->precord = &wf[0];
        return 0;
    }
    if (!strcmp(name, "IMAGE2") || !strcmp(name, "BGIMAGE2")) {
        addr->precord = &wf[1];
        return 0;
    }
    return 1;
}

// 0 == failure!
int open_cfg()
{
    int i, v;

    // Set up the record!
    for (i = 0; i < 2; i++) {
        rec[i].a = strdup(i ? "IMAGE2" : "IMAGE1");
        rec[i].b = &cfg[i].width;
        rec[i].c = &cfg[i].height;
        rec[i].d = &cfg[i].roix;
        rec[i].e = &cfg[i].roiy;
        rec[i].f = &cfg[i].roiw;
        rec[i].g = &cfg[i].roih;
        rec[i].h = &cfg[i].ctrl;
        rec[i].i = &cfg[i].lincnt;
        rec[i].j = strdup(i ? "BGIMAGE2" : "BGIMAGE1");
        rec[i].k = cfg[i].param;
        rec[i].t = &cfg[i].iptr1;
        rec[i].u = &cfg[i].iptr2;
        rec[i].vala = &cfg[i].dx;
        rec[i].tova = sizeof(cfg[i].dx);
        rec[i].valb = &cfg[i].dy;
        rec[i].tovb = sizeof(cfg[i].dy);
        rec[i].valc = cfg[i].xproj;
        rec[i].tovc = sizeof(cfg[i].xproj);
        rec[i].vald = cfg[i].yproj;
        rec[i].tovd = sizeof(cfg[i].yproj);
        rec[i].vale = cfg[i].xlin;
        rec[i].tove = sizeof(cfg[i].xlin);
        rec[i].valf = cfg[i].ylin;
        rec[i].tovf = sizeof(cfg[i].ylin);
        rec[i].valg = &cfg[i].strength;
        rec[i].tovg = sizeof(cfg[i].strength);

        wf[i].bptr = cfg[i].bgimage;
    }

    FILE *fp = fopen(cfgfile, "r");
    char buf[512], *s;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (!strncmp(buf, "PLUGIN", 6)) {
            for (s = &buf[6]; *s == ' '; s++);
            s[strlen(s) - 1] = 0; /* No newline! */
            dl_handle = dlopen(s, RTLD_NOW);
            if (!dl_handle) {
                printf("Cannot load plugin %s\n", s);
                printf("%s\n", dlerror());
                exit(1);
            }
            void *proc = dlsym(dl_handle, "plugin_process");
            void *init = dlsym(dl_handle, "plugin_init");
            if (!proc || !init) {
                printf("Cannot find symbols!\n");
                exit(1);
            }
            *(void **) (&dd_process) = proc;
            *(void **) (&dd_init) = init;
        } else if (!strncmp(buf, "WIDTH", 5)) {
            v = atoi(&buf[5]);
            cfg[0].width = v;
            cfg[1].width = v;
        } else if (!strncmp(buf, "HEIGHT", 6)) {
            v = atoi(&buf[6]);
            cfg[0].height = v;
            cfg[1].height = v;
        } else if (!strncmp(buf, "ROIX", 4)) {
            i = (buf[4] == '2');
            v = atoi(&buf[5]);
            cfg[i].roix = v;
        } else if (!strncmp(buf, "ROIY", 4)) {
            i = (buf[4] == '2');
            v = atoi(&buf[5]);
            cfg[i].roiy = v;
        } else if (!strncmp(buf, "ROIW", 4)) {
            i = (buf[4] == '2');
            v = atoi(&buf[5]);
            cfg[i].roiw = v;
        } else if (!strncmp(buf, "ROIH", 4)) {
            i = (buf[4] == '2');
            v = atoi(&buf[5]);
            cfg[i].roih = v;
        } else if (!strncmp(buf, "CTRL", 4)) {
            i = (buf[4] == '2');
            v = atoi(&buf[5]);
            cfg[i].ctrl = v;
        } else if (!strncmp(buf, "LINCNT", 6)) {
            i = (buf[6] == '2');
            v = atoi(&buf[7]);
            cfg[i].lincnt = v;
        } else if (!strncmp(buf, "PARAM", 5)) {
            i = (buf[5] == '2');
            int j = atoi(&buf[7]);
            for (s = &buf[7]; *s != ' '; s++);
            if (!sscanf(s, "%lf", &cfg[i].param[j])) {
                printf("Failure reading PARAM%d_%d from line %s\n", i, j, buf);
                exit(1);
            }
        } else {
            printf("Unknown line in config file: %s\n", buf);
            exit(1);
        }
    }
    fclose(fp);

    (*dd_init)(&rec[0]);
    (*dd_init)(&rec[1]);
    return 1;
}

// 0 == failure!
int open_in()
{
    long long offset = 0;
    if (infd >= 0) {
        close(infd);
        delete iter;
    }
    if (location) {
        if (sscanf(location, "%d:%lld", &inchunk, &offset) != 2) {
            printf("Cannot parse location %s!\n", location);
            exit(1);
        }
    }
    infd = chunk_open(infile, inchunk, O_RDONLY | O_LARGEFILE);
    if (infd < 0)
        return 0;
    iter = new XtcFileIterator(infd, 0x40000000);
    lseek64(infd, offset, SEEK_SET);
    return next_dg();
}

// 0 == Failure
int open_out(void)
{
    if (location)
        return 1;
    if (outfd >= 0)
        close(outfd);
    unlink(outfile);
    outfd = chunk_open(outfile, outchunk, O_WRONLY | O_CREAT | O_LARGEFILE);
    return (outfd >= 0);
}

// 0 == failure!
int next_dg()
{
    inoff = lseek64(infd, 0, SEEK_CUR);
    if ((dg = iter->next()) == NULL) {
        if (verbose)
            printf("EOF on infile\n");
        if (!open_in()) {
            if (verbose)
                printf("No more chunks for infile!\n");
            return 0;
        }
    }
    if (verbose)
        printf("IN %s transition: clock 0x%x/0x%x, time 0x%x/0x%x, payloadSize %d\n",
               TransitionId::name(dg->seq.service()),
               dg->seq.clock().seconds(), dg->seq.clock().nanoseconds(),
               dg->seq.stamp().fiducials(),dg->seq.stamp().ticks(),
               dg->xtc.sizeofPayload());
    return 1;
}

int do_write(int fd, char *buf, int size)
{
    int left, cur;

    if (location)
        return size;
    for (left = size, cur = 0; left;) {
        int n = write(fd, &buf[cur], left);
        if (n < 0)
            return size - left;
        left -= n;
        cur += n;
    }
    return size;
}

void printPvHeader(EpicsPvHeader *pv)
{
    printf("\t\tID = %d, dbrtype = %d, count = %d",
           pv->iPvId, pv->iDbrType, pv->iNumElements);
    if (pv->iDbrType >= DBR_CTRL_STRING && pv->iDbrType <= DBR_CTRL_DOUBLE) {
        EpicsPvCtrlHeader *pvc = (EpicsPvCtrlHeader *) pv;
        printf(", name = %s\n", pvc->sPvName);
    } else
        printf("\n");
}

/*
 * So what do we have here?  These routines are all given:
 *           |---------- Dgram -------------|
 *    dg0 -> Seq0, Env0, Xtc0 (segment level), payload0
 *
 * The payload is a sequence of source level XTCs.
 *
 * For a camera:
 *     Configure has an Opal1000 config object, frame feature extraction object, and PimImageConfigV1 object.
 * For epics:
 *     Configure has an EpicsPvCtrlHeader object.
 *
 * After this, we want to write four Epics (TypeID = 13) objects: DX1, DY1, DX2, DY2.
 */

void output_configure(void)
{
    Xtc *xtc = (Xtc *)(dg->xtc.payload());
    int size;

    char buf[sizeof(Dgram) + sizeof(Xtc)];
    Dgram *dgnew = (Dgram *)buf;
    Xtc *segnew = (Xtc *)&buf[sizeof(Dgram)];

    DetInfo sourceInfo(getpid(), DetInfo::EpicsArch, 0, DetInfo::NoDevice, 0);
    Pds::Epics::dbr_ctrl_long val;
    memset(&val, 0, sizeof(val));
    struct PvConfig pv(sourceInfo, &val);

    memcpy(buf, dg, sizeof(Dgram) + sizeof(Xtc));

    if (xtc->contains.id() != TypeId::Id_Xtc) {
        printf("Expected Id_Xtc?!?\n");
        exit(0);
    }

    size = xtc->sizeofPayload();
    xtc = (Xtc *)(xtc->payload());

    while (size > 0) {
        if (xtc->contains.id() == TypeId::Id_Epics) {
#if 0
            printPvHeader((EpicsPvHeader *)xtc->payload());
#endif
            dgnew->xtc.extent -= xtc->extent; // Delete all epics PVs!
            segnew->extent    -= xtc->extent;
        }
        size -= xtc->extent;
        xtc = xtc->next();
    }

    dgnew->xtc.extent += 4 * sizeof(struct PvConfig);
    segnew->extent += 4 * sizeof(struct PvConfig);
    
    if (outoff + dgnew->xtc.extent >= CHUNK_SIZE) {
        if (!open_out()) {
            printf("Cannot create new chunk for output!\n");
            exit(1);
        }
        outoff = 0;
    }
    outoff += dgnew->xtc.extent + sizeof(Dgram) - sizeof(Xtc);

    do_write(outfd, buf, sizeof(buf));

    xtc = (Xtc *)(dg->xtc.payload());
    size = xtc->sizeofPayload();
    xtc = (Xtc *)(xtc->payload());
    while (size > 0) {
        if (xtc->contains.id() != TypeId::Id_Epics) {
            do_write(outfd, (char *)xtc, xtc->extent);
        }
        size -= xtc->extent;
        xtc = xtc->next();
    }

    for (int i = 0; i < 4; i++) {
        pv.iPvId = i;
        pv.sPvName[1] = 'X' + (i / 2);
        pv.sPvName[2] = '1' + (i % 2);
        do_write(outfd, (char *)&pv, sizeof(pv));
    }
}

void write_image(char *name, short *data, int w, int h)
{
    short max = 0;
    FILE *fp = fopen(name, "w");
    for (int i = 0; i < w * h; i++) {
        if (data[i] > max)
            max = data[i];
    }
    fprintf(fp, "P2 %d %d %d\n", w, h, max);
    for (int i = 0; i < w * h; i++)
        fprintf(fp, "%d\n", data[i]);
    fclose(fp);
}

void write_file(char *name, int *data, int len)
{
    FILE *fp = fopen(name, "w");
    if (!fp) {
        printf("Cannot open %s for writing!\n", name);
        return;
    }
    for (int i = 0; i < len; i++)
        fprintf(fp, " %d", data[i]);
    fprintf(fp, "\n");
    fclose(fp);
}

void output_l1accept(void)
{
    epicsTimeStamp ts;
    int values[4];
    Xtc *xtc = (Xtc *)(dg->xtc.payload());
    int size;

    /*
     * Create an EPICS timestamp for this event.  Make sure that the low bits
     * of the nanoseconds contain the fiducial!
     */
    ts.secPastEpoch = dg->seq.clock().seconds() - POSIX_TIME_AT_EPICS_EPOCH;
    ts.nsec         = dg->seq.clock().nanoseconds();
    if ((ts.nsec & 0x1ffff) != dg->seq.stamp().fiducials()) {
        ts.nsec &= ~0x1ffff;
        ts.nsec |= dg->seq.stamp().fiducials();
    }

    char buf[sizeof(Dgram) + sizeof(Xtc)];
    Dgram *dgnew = (Dgram *)buf;
    Xtc *segnew = (Xtc *)&buf[sizeof(Dgram)];

    DetInfo sourceInfo(getpid(), DetInfo::EpicsArch, 0, DetInfo::NoDevice, 0);
    Pds::Epics::dbr_time_long val;
    memset(&val, 0, sizeof(val));
    struct PvValue pv(sourceInfo, &val);

    memcpy(buf, dg, sizeof(Dgram) + sizeof(Xtc));

    if (xtc->contains.id() != TypeId::Id_Xtc) {
        printf("Expected Id_Xtc?!?\n");
        exit(0);
    }

    size = xtc->sizeofPayload();
    xtc = (Xtc *)(xtc->payload());

    while (size > 0) {
        if (xtc->contains.id() == TypeId::Id_Epics) {
            dgnew->xtc.extent -= xtc->extent; // Delete all epics PVs!
            segnew->extent    -= xtc->extent;
        }
        size -= xtc->extent;
        xtc = xtc->next();
    }

    /*
     * Add in four new epics PVs for the droplet locations.
     */
    dgnew->xtc.extent += 4 * sizeof(struct PvValue);
    segnew->extent += 4 * sizeof(struct PvValue);
    
    if (outoff + dgnew->xtc.extent >= CHUNK_SIZE) {
        if (!open_out()) {
            printf("Cannot create new chunk for output!\n");
            exit(1);
        }
        outoff = 0;
    }
    outoff += dgnew->xtc.extent + sizeof(Dgram) - sizeof(Xtc);

    do_write(outfd, buf, sizeof(buf));

    xtc = (Xtc *)(dg->xtc.payload());
    size = xtc->sizeofPayload();
    xtc = (Xtc *)(xtc->payload());
    while (size > 0) {
        if (xtc->contains.id() != TypeId::Id_Epics) {
            do_write(outfd, (char *)xtc, xtc->extent);
        }
        if (xtc->contains.id() == TypeId::Id_Frame) {
            Camera::FrameV1 *f = (Camera::FrameV1 *)(xtc->payload());
            short *data = (short *)f->data();

            *(void **)rec[0].u = data;
            *(void **)rec[1].u = data;
            (*dd_process)(&rec[0]);
            (*dd_process)(&rec[1]);
            values[0] = cfg[0].dx;
            values[1] = cfg[1].dx;
            values[2] = cfg[0].dy;
            values[3] = cfg[1].dy;
        }
        size -= xtc->extent;
        xtc = xtc->next();
    }

    if (location) {
        FILE *fp = fopen("droplets", "w");
        fprintf(fp, "%4d %4d %10g\n", cfg[0].dx, cfg[0].dy, cfg[0].strength);
        fprintf(fp, "%4d %4d %10g\n", cfg[1].dx, cfg[1].dy, cfg[1].strength);
        fclose(fp);

        write_image("image_data.ppm", *(short **)rec[0].u, cfg[0].width, cfg[0].height);
        write_image("bgimage1.ppm", *(short **)rec[0].t, cfg[0].width, cfg[0].height);
        write_image("bgimage2.ppm", *(short **)rec[1].t, cfg[0].width, cfg[0].height);

        write_file("xproj1", cfg[0].xproj, cfg[0].roiw);
        write_file("xproj2", cfg[1].xproj, cfg[1].roiw);
        write_file("yproj1", cfg[0].yproj, cfg[0].roih);
        write_file("yproj2", cfg[1].yproj, cfg[1].roih);

        write_file("xlin1", cfg[0].xlin, cfg[0].roiw);
        write_file("xlin2", cfg[1].xlin, cfg[1].roiw);
        write_file("ylin1", cfg[0].ylin, cfg[0].roih);
        write_file("ylin2", cfg[1].ylin, cfg[1].roih);
    }

    for (int i = 0; i < 4; i++) {
        pv.iPvId = i;
        pv.value = values[i];
        memcpy(&pv.stamp, &ts, sizeof(ts));
        do_write(outfd, (char *)&pv, sizeof(pv));
    }
}

/*
 * Just dump the transition.
 *
 */
void output_transition(void)
{
    do_write(outfd, (char *)dg, sizeof(*dg));
    do_write(outfd, dg->xtc.payload(), dg->xtc.sizeofPayload());
}

int main(int argc, char **argv)
{
    int c;
    memset(cfg, 0, sizeof(cfg));
    memset(rec, 0, sizeof(rec));
    static struct option long_options[] = {
        {"help",      0, 0, 'h'},
        {"config",    1, 0, 'c'},
        {"output",    1, 0, 'o'},
        {"location",  1, 0, 'l'},
        {"verbose",   0, 0, 'v'},
        {NULL, 0, NULL, 0}
    };
    int idx = 0;
    int done = 0;

    while ((c = getopt_long(argc, argv, "ho:c:l:v", long_options, &idx)) != -1) {
        switch (c) {
        case 'h':
            usage();
            break;
        case 'c':
            cfgfile = optarg;
            break;
        case 'o':
            outfile = optarg;
            break;
        case 'l':
            location = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        }
    }
    infile = argv[optind];
    if (!infile) {
        printf("No input file!\n");
        exit(1);
    }
    if (!outfile && !location) {
        printf("No output file or location specification!\n");
        exit(1);
    }
    if (!cfgfile) {
        printf("No configuration file!\n");
        exit(1);
    }

    if (!open_cfg()) {
        printf("Cannot open configuration %s!\n", cfgfile);
        exit(1);
    }
    if (!open_in()) {
        printf("Cannot open input %s!\n", infile);
        exit(1);
    }
    if (!open_out()) {
        printf("Cannot create %s!\n", outfile);
        exit(1);
    }

    while (!done) {
        switch (dg->seq.service()) {
        case TransitionId::Configure:
            if (location) {
                printf("Single frame isn't an L1Accept?!?\n");
                exit(0);
            }
            output_configure();
            if (!next_dg())
                done = 1;
            break;
        case TransitionId::L1Accept:
            output_l1accept();
            if (location || !next_dg())
                done = 1;
            break;
        default:
            if (location) {
                printf("Single frame isn't an L1Accept?!?\n");
                exit(0);
            }
            output_transition();
            if (!next_dg())
                done = 1;
            break;
        }
    }
}
