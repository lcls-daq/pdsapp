#include"pdsdata/xtc/Xtc.hh"
#include"pdsdata/xtc/DetInfo.hh"
#include<string>
#include<iostream>

#define DEFAULT_CRED "/reg/g/pcds/controls/camrecord/CONFIG/DEFAULT"
#define DEFAULT_CFG  ".camrecord"
#define POSIX_TIME_AT_EPICS_EPOCH 631152000u
#define NFSBASE     "/reg/d/cameras/"

/* Flags for how the epicsTime can be messed up */
#define REVTIME_NONE   0
#define REVTIME_WORD   1 
#define REVTIME_OFFSET 2

/* Flags that can modify cameras */
#define CAMERA_NONE    0
#define CAMERA_BINNED  1    /* The image is binned in a standard way. */
#define CAMERA_ROI     2    /* The image has a size described by :ROI_YNP and :ROI_XNP. */
#define CAMERA_SIZE    4    /* The image has a size described by :N_OF_ROW and :N_OF_COL. */
#define CAMERA_ADET    8    /* The image has a size described by :ArraySize0_RBV and :ArraySize1_RBV */


/* How many parameters does LogBook::Connection::open have? */
#define LCPARAMS 12

/* main.cc */
extern void add_socket(int s);
extern void remove_socket(int s);
extern void begin_run(void);
extern int record_cnt;
extern int verbose;
extern std::string hostname;
extern std::string prefix;
extern std::string username;
extern std::string curdir;
extern int expid, runnum, strnum;
extern std::string logbook[LCPARAMS];
extern int start_sec, start_nsec;
extern int end_sec, end_nsec;
extern int streamno;

/* bld.cc */
extern void initialize_bld(void);
extern void create_bld(std::string name, int address, std::string device, int revtime);
extern void handle_bld(fd_set *rfds);
extern void cleanup_bld(void);

/* ca.cc */
extern void initialize_ca(void);
extern void create_ca(std::string name, std::string detector, std::string camtype,
                      std::string pvname, int binned, int strict);
extern void handle_ca(fd_set *rfds);
extern void begin_run_ca(void);
extern void cleanup_ca(void);

/* xtc.cc */
extern void nofid(void);
extern void initialize_xtc(char *outfile);
extern int register_xtc(int sync, std::string name, int critical, int isbig);
extern void register_alias(std::string name, Pds::DetInfo &sourceInfo);
extern void register_pv_alias(std::string name, int idx, Pds::DetInfo &sourceInfo);
extern void configure_xtc(int id, char *buf, int size, unsigned int secs, unsigned int nsecs);
extern void data_xtc(int id, unsigned int secs, unsigned int nsecs, Pds::Xtc *hdr, int hdrlen, void *data);
extern void cleanup_xtc(void);
extern void cleanup_index(void);
extern void xtc_stats(void);
extern void do_transition(int id, unsigned int secs, unsigned int nsecs, unsigned int fid,
                          int force);
extern char *damage_report(void);

