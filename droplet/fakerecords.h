/*
 * MCB - Hacked up to work here.  notXXX is ugly, as are the typedefs for DBLINK and ELLLIST.
 */
#include "epicsMutex.h"
#include "epicsTime.h"
#include "epicsTypes.h"
typedef long long DBLINK[6];
typedef int ELLLIST[6];

struct dbAddr {
        void *precord;          /* address of record                   */
};

typedef struct genSubRecord {
	char		name[61]; /*Record Name*/
	char		desc[29]; /*Descriptor*/
	char		asg[29]; /*Access Security Group*/
	epicsEnum16	scan;	/*Scan Mechanism*/
	epicsEnum16	pini;	/*Process at iocInit*/
	short		phas;	/*Scan Phase*/
	short		evnt;	/*Event Number*/
	short		tse;	/*Time Stamp Event*/
	DBLINK		tsel;	/*Time Stamp Link*/
	epicsEnum16	dtyp;	/*Device Type*/
	short		disv;	/*Disable Value*/
	short		disa;	/*Disable*/
	DBLINK		sdis;	/*Scanning Disable*/
	epicsMutexId	mlok;	/*Monitor lock*/
	ELLLIST		mlis;	/*Monitor List*/
	unsigned char	disp;	/*Disable putField*/
	unsigned char	proc;	/*Force Processing*/
	epicsEnum16	stat;	/*Alarm Status*/
	epicsEnum16	sevr;	/*Alarm Severity*/
	epicsEnum16	nsta;	/*New Alarm Status*/
	epicsEnum16	nsev;	/*New Alarm Severity*/
	epicsEnum16	acks;	/*Alarm Ack Severity*/
	epicsEnum16	ackt;	/*Alarm Ack Transient*/
	epicsEnum16	diss;	/*Disable Alarm Sevrty*/
	unsigned char	lcnt;	/*Lock Count*/
	unsigned char	pact;	/*Record active*/
	unsigned char	putf;	/*dbPutField process*/
	unsigned char	rpro;	/*Reprocess */
	void		*asp;	/*Access Security Pvt*/
	struct putNotify *ppn;	/*addr of PUTNOTIFY*/
	struct putNotifyRecord *ppnr;	/*pputNotifyRecord*/
	struct scan_element *spvt;	/*Scan Private*/
	struct rset	*rset;	/*Address of RSET*/
	struct dset	*dset;	/*DSET address*/
	void		*dpvt;	/*Device Private*/
	struct dbRecordType *rdes;	/*Address of dbRecordType*/
	struct lockRecord *lset;	/*Lock Set*/
	epicsEnum16	prio;	/*Scheduling Priority*/
	unsigned char	tpro;	/*Trace Processing*/
	char bkpt;	/*Break Point*/
	unsigned char	udf;	/*Undefined*/
	epicsTimeStamp	time;	/*Time*/
	DBLINK		flnk;	/*Forward Process Link*/
	double		vers;	/*Version Number*/
	epicsInt32		val;	/*Subr. return value*/
	epicsInt32		oval;	/*Old return value*/
	void *           sadr;	/*Subroutine Address*/
	void *           osad;	/*Old Subr. Address*/
	epicsEnum16	lflg;	/*Link Flag*/
	epicsEnum16	eflg;	/*Event Flag*/
	DBLINK		subl;	/*Subroutine Input Link*/
	char		inam[40]; /*Init Routine Name*/
	char		snam[40]; /*Process Subr. Name*/
	char		onam[40]; /*Old Subroutine Name*/
	epicsEnum16	brsv;	/*Bad Return Severity*/
	short		prec;	/*Display Precision*/
	DBLINK		inpa;	/*Input Link A*/
	DBLINK		inpb;	/*Input Link B*/
	DBLINK		inpc;	/*Input Link C*/
	DBLINK		inpd;	/*Input Link D*/
	DBLINK		inpe;	/*Input Link E*/
	DBLINK		inpf;	/*Input Link F*/
	DBLINK		inpg;	/*Input Link G*/
	DBLINK		inph;	/*Input Link H*/
	DBLINK		inpi;	/*Input Link I*/
	DBLINK		inpj;	/*Input Link J*/
	DBLINK		inpk;	/*Input Link K*/
	DBLINK		inpl;	/*Input Link L*/
	DBLINK		inpm;	/*Input Link M*/
	DBLINK		inpn;	/*Input Link N*/
	DBLINK		inpo;	/*Input Link O*/
	DBLINK		inpp;	/*Input Link P*/
	DBLINK		inpq;	/*Input Link Q*/
	DBLINK		inpr;	/*Input Link R*/
	DBLINK		inps;	/*Input Link S*/
	DBLINK		inpt;	/*Input Link T*/
	DBLINK		inpu;	/*Input Link U*/
	char		ufa[40]; /*Input Structure A*/
	char		ufb[40]; /*Input Structure B*/
	char		ufc[40]; /*Input Structure C*/
	char		ufd[40]; /*Input Structure D*/
	char		ufe[40]; /*Input Structure E*/
	char		uff[40]; /*Input Structure F*/
	char		ufg[40]; /*Input Structure G*/
	char		ufh[40]; /*Input Structure H*/
	char		ufi[40]; /*Input Structure I*/
	char		ufj[40]; /*Input Structure J*/
	char		ufk[40]; /*Input Structure K*/
	char		ufl[40]; /*Input Structure L*/
	char		ufm[40]; /*Input Structure M*/
	char		ufn[40]; /*Input Structure N*/
	char		ufo[40]; /*Input Structure O*/
	char		ufp[40]; /*Input Structure P*/
	char		ufq[40]; /*Input Structure Q*/
	char		ufr[40]; /*Input Structure R*/
	char		ufs[40]; /*Input Structure S*/
	char		uft[40]; /*Input Structure T*/
	char		ufu[40]; /*Input Structure U*/
	void *a;	/*Value of Input A*/
	void *b;	/*Value of Input B*/
	void *c;	/*Value of Input C*/
	void *d;	/*Value of Input D*/
	void *e;	/*Value of Input E*/
	void *f;	/*Value of Input F*/
	void *g;	/*Value of Input G*/
	void *h;	/*Value of Input H*/
	void *i;	/*Value of Input I*/
	void *j;	/*Value of Input J*/
	void *k;	/*Value of Input K*/
	void *l;	/*Value of Input L*/
	void *m;	/*Value of Input M*/
	void *n;	/*Value of Input N*/
	void *o;	/*Value of Input O*/
	void *p;	/*Value of Input P*/
	void *q;	/*Value of Input Q*/
	void *r;	/*Value of Input R*/
	void *s;	/*Value of Input S*/
	void *t;	/*Value of Input T*/
	void *u;	/*Value of Input U*/
	epicsEnum16	fta;	/*Type of A*/
	epicsEnum16	ftb;	/*Type of B*/
	epicsEnum16	ftc;	/*Type of C*/
	epicsEnum16	ftd;	/*Type of D*/
	epicsEnum16	fte;	/*Type of E*/
	epicsEnum16	ftf;	/*Type of F*/
	epicsEnum16	ftg;	/*Type of G*/
	epicsEnum16	fth;	/*Type of H*/
	epicsEnum16	fti;	/*Type of I*/
	epicsEnum16	ftj;	/*Type of J*/
	epicsEnum16	ftk;	/*Type of K*/
	epicsEnum16	ftl;	/*Type of L*/
	epicsEnum16	ftm;	/*Type of M*/
	epicsEnum16	ftn;	/*Type of N*/
	epicsEnum16	fto;	/*Type of O*/
	epicsEnum16	ftp;	/*Type of P*/
	epicsEnum16	ftq;	/*Type of Q*/
	epicsEnum16	ftr;	/*Type of R*/
	epicsEnum16	fts;	/*Type of S*/
	epicsEnum16	ftt;	/*Type of T*/
	epicsEnum16	ftu;	/*Type of U*/
	unsigned long	noa;	/*No. in A*/
	unsigned long	nob;	/*No. in B*/
	unsigned long	noc;	/*No. in C*/
	unsigned long	nod;	/*No. in D*/
	unsigned long	noe;	/*No. in E*/
	unsigned long	nof;	/*No. in F*/
	unsigned long	nog;	/*No. in G*/
	unsigned long	noh;	/*No. in H*/
	unsigned long	noi;	/*No. in I*/
	unsigned long	noj;	/*No. in J*/
	unsigned long	nok;	/*No. in K*/
	unsigned long	nol;	/*No. in L*/
	unsigned long	nom;	/*No. in M*/
	unsigned long	non;	/*No. in N*/
	unsigned long	noo;	/*No. in O*/
	unsigned long	nop;	/*No. in P*/
	unsigned long	noq;	/*No. in Q*/
	unsigned long	nor;	/*No. in R*/
	unsigned long	nos;	/*No. in S*/
	unsigned long	notXXX;	/*No. in T*/
	unsigned long	nou;	/*No. in U*/
	DBLINK		outa;	/*Output Link A*/
	DBLINK		outb;	/*Output Link B*/
	DBLINK		outc;	/*Output Link C*/
	DBLINK		outd;	/*Output Link D*/
	DBLINK		oute;	/*Output Link E*/
	DBLINK		outf;	/*Output Link F*/
	DBLINK		outg;	/*Output Link G*/
	DBLINK		outh;	/*Output Link H*/
	DBLINK		outi;	/*Output Link I*/
	DBLINK		outj;	/*Output Link J*/
	DBLINK		outk;	/*Output Link K*/
	DBLINK		outl;	/*Output Link L*/
	DBLINK		outm;	/*Output Link M*/
	DBLINK		outn;	/*Output Link N*/
	DBLINK		outo;	/*Output Link O*/
	DBLINK		outp;	/*Output Link P*/
	DBLINK		outq;	/*Output Link Q*/
	DBLINK		outr;	/*Output Link R*/
	DBLINK		outs;	/*Output Link S*/
	DBLINK		outt;	/*Output Link T*/
	DBLINK		outu;	/*Output Link U*/
	char		ufva[40]; /*Output Structure A*/
	char		ufvb[40]; /*Output Structure B*/
	char		ufvc[40]; /*Output Structure C*/
	char		ufvd[40]; /*Output Structure D*/
	char		ufve[40]; /*Output Structure E*/
	char		ufvf[40]; /*Output Structure F*/
	char		ufvg[40]; /*Output Structure G*/
	char		ufvh[40]; /*Output Structure H*/
	char		ufvi[40]; /*Output Structure I*/
	char		ufvj[40]; /*Output Structure J*/
	char		ufvk[40]; /*Output Structure K*/
	char		ufvl[40]; /*Output Structure L*/
	char		ufvm[40]; /*Output Structure M*/
	char		ufvn[40]; /*Output Structure N*/
	char		ufvo[40]; /*Output Structure O*/
	char		ufvp[40]; /*Output Structure P*/
	char		ufvq[40]; /*Output Structure Q*/
	char		ufvr[40]; /*Output Structure R*/
	char		ufvs[40]; /*Output Structure S*/
	char		ufvt[40]; /*Output Structure T*/
	char		ufvu[40]; /*Output Structure U*/
	void *vala;	/*Value of Output A*/
	void *valb;	/*Value of Output B*/
	void *valc;	/*Value of Output C*/
	void *vald;	/*Value of Output D*/
	void *vale;	/*Value of Output E*/
	void *valf;	/*Value of Output F*/
	void *valg;	/*Value of Output G*/
	void *valh;	/*Value of Output H*/
	void *vali;	/*Value of Output I*/
	void *valj;	/*Value of Output J*/
	void *valk;	/*Value of Output K*/
	void *vall;	/*Value of Output L*/
	void *valm;	/*Value of Output M*/
	void *valn;	/*Value of Output N*/
	void *valo;	/*Value of Output O*/
	void *valp;	/*Value of Output P*/
	void *valq;	/*Value of Output Q*/
	void *valr;	/*Value of Output R*/
	void *vals;	/*Value of Output S*/
	void *valt;	/*Value of Output T*/
	void *valu;	/*Value of Output U*/
	void *ovla;	/*Old Output A*/
	void *ovlb;	/*Old Output B*/
	void *ovlc;	/*Old Output C*/
	void *ovld;	/*Old Output D*/
	void *ovle;	/*Old Output E*/
	void *ovlf;	/*Old Output F*/
	void *ovlg;	/*Old Output G*/
	void *ovlh;	/*Old Output H*/
	void *ovli;	/*Old Output I*/
	void *ovlj;	/*Old Output J*/
	void *ovlk;	/*Old Output K*/
	void *ovll;	/*Old Output L*/
	void *ovlm;	/*Old Output M*/
	void *ovln;	/*Old Output N*/
	void *ovlo;	/*Old Output O*/
	void *ovlp;	/*Old Output P*/
	void *ovlq;	/*Old Output Q*/
	void *ovlr;	/*Old Output R*/
	void *ovls;	/*Old Output S*/
	void *ovlt;	/*Old Output T*/
	void *ovlu;	/*Old Output U*/
	epicsEnum16	ftva;	/*Type of VALA*/
	epicsEnum16	ftvb;	/*Type of VALB*/
	epicsEnum16	ftvc;	/*Type of VALC*/
	epicsEnum16	ftvd;	/*Type of VALD*/
	epicsEnum16	ftve;	/*Type of VALE*/
	epicsEnum16	ftvf;	/*Type of VALF*/
	epicsEnum16	ftvg;	/*Type of VALG*/
	epicsEnum16	ftvh;	/*Type of VALH*/
	epicsEnum16	ftvi;	/*Type of VALI*/
	epicsEnum16	ftvj;	/*Type of VALJ*/
	epicsEnum16	ftvk;	/*Type of VALK*/
	epicsEnum16	ftvl;	/*Type of VALL*/
	epicsEnum16	ftvm;	/*Type of VALM*/
	epicsEnum16	ftvn;	/*Type of VALN*/
	epicsEnum16	ftvo;	/*Type of VALO*/
	epicsEnum16	ftvp;	/*Type of VALP*/
	epicsEnum16	ftvq;	/*Type of VALQ*/
	epicsEnum16	ftvr;	/*Type of VALR*/
	epicsEnum16	ftvs;	/*Type of VALS*/
	epicsEnum16	ftvt;	/*Type of VALT*/
	epicsEnum16	ftvu;	/*Type of VALU*/
	unsigned long	nova;	/*No. in VALA*/
	unsigned long	novb;	/*No. in VALB*/
	unsigned long	novc;	/*No. in VALC*/
	unsigned long	novd;	/*No. in VALD*/
	unsigned long	nove;	/*No. in VALE*/
	unsigned long	novf;	/*No. in VALF*/
	unsigned long	novg;	/*No. in VALG*/
	unsigned long	novh;	/*No. in VAlH*/
	unsigned long	novi;	/*No. in VALI*/
	unsigned long	novj;	/*No. in VALJ*/
	unsigned long	novk;	/*No. in VALK*/
	unsigned long	novl;	/*No. in VALL*/
	unsigned long	novm;	/*No. in VALM*/
	unsigned long	novn;	/*No. in VALN*/
	unsigned long	novo;	/*No. in VALO*/
	unsigned long	novp;	/*No. in VALP*/
	unsigned long	novq;	/*No. in VALQ*/
	unsigned long	novr;	/*No. in VALR*/
	unsigned long	novs;	/*No. in VALS*/
	unsigned long	novt;	/*No. in VALT*/
	unsigned long	novu;	/*No. in VALU*/
	unsigned long	tova;	/*Total bytes for VALA*/
	unsigned long	tovb;	/*Total bytes for VALB*/
	unsigned long	tovc;	/*Total bytes for VALC*/
	unsigned long	tovd;	/*Total bytes for VALD*/
	unsigned long	tove;	/*Total bytes for VALE*/
	unsigned long	tovf;	/*Total bytes for VALF*/
	unsigned long	tovg;	/*Total bytes for VALG*/
	unsigned long	tovh;	/*Total bytes for VAlH*/
	unsigned long	tovi;	/*Total bytes for VALI*/
	unsigned long	tovj;	/*Total bytes for VALJ*/
	unsigned long	tovk;	/*Total bytes for VALK*/
	unsigned long	tovl;	/*Total bytes for VALL*/
	unsigned long	tovm;	/*Total bytes for VALM*/
	unsigned long	tovn;	/*Total bytes for VALN*/
	unsigned long	tovo;	/*Total bytes for VALO*/
	unsigned long	tovp;	/*Total bytes for VALP*/
	unsigned long	tovq;	/*Total bytes for VALQ*/
	unsigned long	tovr;	/*Total bytes for VALR*/
	unsigned long	tovs;	/*Total bytes for VALS*/
	unsigned long	tovt;	/*Total bytes for VALT*/
	unsigned long	tovu;	/*Total bytes for VALU*/
} genSubRecord;

typedef struct waveformRecord {
	char		name[61]; /*Record Name*/
	char		desc[29]; /*Descriptor*/
	char		asg[29]; /*Access Security Group*/
	epicsEnum16	scan;	/*Scan Mechanism*/
	epicsEnum16	pini;	/*Process at iocInit*/
	short		phas;	/*Scan Phase*/
	short		evnt;	/*Event Number*/
	short		tse;	/*Time Stamp Event*/
	DBLINK		tsel;	/*Time Stamp Link*/
	epicsEnum16	dtyp;	/*Device Type*/
	short		disv;	/*Disable Value*/
	short		disa;	/*Disable*/
	DBLINK		sdis;	/*Scanning Disable*/
	epicsMutexId	mlok;	/*Monitor lock*/
	ELLLIST		mlis;	/*Monitor List*/
	unsigned char	disp;	/*Disable putField*/
	unsigned char	proc;	/*Force Processing*/
	epicsEnum16	stat;	/*Alarm Status*/
	epicsEnum16	sevr;	/*Alarm Severity*/
	epicsEnum16	nsta;	/*New Alarm Status*/
	epicsEnum16	nsev;	/*New Alarm Severity*/
	epicsEnum16	acks;	/*Alarm Ack Severity*/
	epicsEnum16	ackt;	/*Alarm Ack Transient*/
	epicsEnum16	diss;	/*Disable Alarm Sevrty*/
	unsigned char	lcnt;	/*Lock Count*/
	unsigned char	pact;	/*Record active*/
	unsigned char	putf;	/*dbPutField process*/
	unsigned char	rpro;	/*Reprocess */
	void		*asp;	/*Access Security Pvt*/
	struct putNotify *ppn;	/*addr of PUTNOTIFY*/
	struct putNotifyRecord *ppnr;	/*pputNotifyRecord*/
	struct scan_element *spvt;	/*Scan Private*/
	struct rset	*rset;	/*Address of RSET*/
	struct dset	*dset;	/*DSET address*/
	void		*dpvt;	/*Device Private*/
	struct dbRecordType *rdes;	/*Address of dbRecordType*/
	struct lockRecord *lset;	/*Lock Set*/
	epicsEnum16	prio;	/*Scheduling Priority*/
	unsigned char	tpro;	/*Trace Processing*/
	char bkpt;	/*Break Point*/
	unsigned char	udf;	/*Undefined*/
	epicsTimeStamp	time;	/*Time*/
	DBLINK		flnk;	/*Forward Process Link*/
	void *		val;	/*Value*/
	short		rarm;	/*Rearm the waveform*/
	short		prec;	/*Display Precision*/
	DBLINK		inp;	/*Input Specification*/
	char		egu[16]; /*Engineering Units Name*/
	double		hopr;	/*High Operating Range*/
	double		lopr;	/*Low Operating Range*/
	unsigned long	nelm;	/*Number of Elements*/
	epicsEnum16	ftvl;	/*Field Type of Value*/
	short		busy;	/*Busy Indicator*/
	unsigned long	nord;	/*Number elements read*/
	void *		bptr;	/*Buffer Pointer*/
	DBLINK		siol;	/*Sim Input Specifctn*/
	DBLINK		siml;	/*Sim Mode Location*/
	epicsEnum16	simm;	/*Simulation Mode*/
	epicsEnum16	sims;	/*Sim mode Alarm Svrty*/
} waveformRecord;
