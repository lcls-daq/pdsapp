#include<stdio.h>
#include<getopt.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<new>
#include<vector>
#include"pdsdata/xtc/Xtc.hh"
#include"pdsdata/xtc/TypeId.hh"
#include"pdsdata/xtc/TimeStamp.hh"
#include"pdsdata/xtc/Dgram.hh"
#include"pdsdata/xtc/XtcIterator.hh"
#include"pdsdata/xtc/XtcFileIterator.hh"
#include"pdsdata/psddl/bld.ddl.h"
#include"pdsdata/psddl/evr.ddl.h"

using namespace std;
using namespace Pds;

#define CHUNK_SIZE 107374182400LL

char *dname = NULL;
char *ename = NULL;
char *hutch = NULL;
int   expno = 0;
int   runno = 0;
int   stream = 0;
int   chunk = 0;
int   fd = -1;
long long off = 0;
XtcFileIterator *iter = NULL;
Dgram *dg = NULL;
int verbose = 0;
int next_dg();

// 0 == failure!
int open_in()
{
    char buf[1024];
    if (fd >= 0) {
        close(fd);
        delete iter;
    }
    sprintf(buf, "%s/%s/%s/xtc/e%03d-r%04d-s%02d-c%02d.xtc", dname, hutch, ename, expno, runno, stream, chunk++);
    fd = open(buf, O_RDONLY | O_LARGEFILE, 0644);
    if (fd < 0) {
        chunk = 0;
        stream++;
        sprintf(buf, "%s/%s/%s/xtc/e%03d-r%04d-s%02d-c%02d.xtc", dname, hutch, ename, expno, runno, stream, chunk++);
        fd = open(buf, O_RDONLY | O_LARGEFILE, 0644);
        if (fd < 0)
            return 0;
    }
    iter = new XtcFileIterator(fd, 0x40000000);
    return 1;
}

// 0 == failure!
int next_dg()
{
    off = lseek64(fd, 0, SEEK_CUR);
    if ((dg = iter->next()) == NULL) {
        if (verbose)
            printf("EOF!\n");
        if (!open_in()) {
            if (verbose)
                printf("No more chunks!\n");
            return 0;
        } else {
            return next_dg();
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

void find_info(Xtc *root, Xtc **bld, Xtc **evr)
{
    Xtc* xtc;
    int remaining;

    switch (root->contains.id()) {
    case TypeId::Id_Xtc:
        /* Copied, with mods, from XtcIterator.cc */
        if (root->damage.value() & ( 1 << Damage::IncompleteContribution))
            return;

        xtc = (Xtc*)root->payload();
        remaining = root->sizeofPayload();

        while(remaining > 0) {
            if (xtc->extent==0)
                break; // try to skip corrupt event
            find_info(xtc, bld, evr);
            if (*bld != NULL && *evr != NULL)
                return;
            remaining -= xtc->sizeofPayload() + sizeof(Xtc);
            xtc      = xtc->next();
        }
        break;
    case TypeId::Id_EvrData:
        *evr = root;
        break;
    case TypeId::Id_FEEGasDetEnergy:
        *bld = root;
        break;
    default:
        break;
    }
}


/* Args: dropcheck directory expname expno runno */
int main(int argc, char **argv)
{
    double mf11 = 0, mf12 = 0;
    double mf21 = 0, mf22 = 0;
    double mf63 = 0, mf64 = 0;
    int cnt = 0;
    Xtc *bld, *evr;

    if (argc < 5) {
        printf("Usage: dropcheck DIRNAME EXPNAME EXPNO RUNNO\n");
        exit(0);
    }
    dname = argv[1];
    ename = argv[2];
    hutch = strdup(ename);
    hutch[3] = 0;
    expno = atoi(argv[3]);
    runno = atoi(argv[4]);
    open_in();

    for (;;) {
        if (!next_dg())
            break;
        if (dg->seq.service() == TransitionId::L1Accept) {
            bld = NULL;
            evr = NULL;
            find_info(&dg->xtc, &bld, &evr);
            if (bld && evr) {
                double f11, f12;
                double f21, f22;
                double f63, f64;
                int drop = 0, fid140 = 0, fid162 = 0;
                switch (evr->contains.version()) {
                case 3: {
                    EvrData::DataV3 *e = (EvrData::DataV3 *)evr->payload();
                    ndarray<const EvrData::FIFOEvent, 1> d = e->fifoEvents();
                    for (int i = e->numFifoEvents() - 1; i >= 0; i--) {
                        const EvrData::FIFOEvent& event = d[i];
                        if (event.eventCode() == 140) {
                            fid140 = event.timestampHigh();
                        }
                        if (event.eventCode() == 162) {
                            fid162 = event.timestampHigh();
                        }
                    }
                    if (fid162 && fid140 == fid162) {
                        drop = 1;
                        cnt++;
                    }
                    break;
                } 
                case 4: {
                    EvrData::DataV4 *e = (EvrData::DataV4 *)evr->payload();
                    ndarray<const EvrData::FIFOEvent, 1> d = e->fifoEvents();
                    for (int i = e->numFifoEvents() - 1; i >= 0; i--) {
                        const EvrData::FIFOEvent& event = d[i];
                        if (event.eventCode() == 140) {
                            fid140 = event.timestampHigh();
                        }
                        if (event.eventCode() == 162) {
                            fid162 = event.timestampHigh();
                        }
                    }
                    if (fid162 && fid140 == fid162) {
                        drop = 1;
                        cnt++;
                    }
                    break;
                }
                default:
                    printf("EVR v%d?!?\n", evr->contains.version());
                    exit(1);
                }
                if (!drop)
                    continue;
                switch (bld->contains.version()) {
                case 0: {
                    Bld::BldDataFEEGasDetEnergy *b = (Bld::BldDataFEEGasDetEnergy *)bld->payload();
                    f11 = b->f_11_ENRC();
                    f12 = b->f_12_ENRC();
                    f21 = b->f_21_ENRC();
                    f22 = b->f_22_ENRC();
                    f63 = 0.0;
                    f64 = 0.0;
                    break;
                }
                case 1: {
                    Bld::BldDataFEEGasDetEnergyV1 *b = (Bld::BldDataFEEGasDetEnergyV1 *)bld->payload();
                    f11 = b->f_11_ENRC();
                    f12 = b->f_12_ENRC();
                    f21 = b->f_21_ENRC();
                    f22 = b->f_22_ENRC();
                    f63 = b->f_63_ENRC();
                    f64 = b->f_64_ENRC();
                    break;
                }
                default:
                    printf("BLD v%d?!?\n", bld->contains.version());
                    exit(1);
                }
                if (f11 > mf11)
                    mf11 = f11;
                if (f12 > mf12)
                    mf12 = f12;
                if (f21 > mf21)
                    mf21 = f21;
                if (f22 > mf22)
                    mf22 = f22;
                if (f63 > mf63)
                    mf63 = f63;
                if (f64 > mf64)
                    mf64 = f64;
                printf("%10s %4d: 0x%05x --> %g %g %g %g %g %g\n",
                       ename, runno, fid162, f11, f12, f21, f22, f63, f64);
            }
        }
    }
    printf("%10s %4d: Found %d dropped shots with FEEGasDetEnergy (max %g %g %g %g %g %g).\n\n",
           ename, runno, cnt, mf11, mf12, mf21, mf22, mf63, mf64);
}
