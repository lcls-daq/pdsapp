#include"pdsdata/xtc/Xtc.hh"
#include<string>
#include<iostream>

#define DEFAULT_DIR "/reg/neh/home1/mcbrowne/lib"
#define DEFAULT_CFG ".camrecord"
#define POSIX_TIME_AT_EPICS_EPOCH 631152000u
#define NFSBASE     "/reg/d/cameras/"

/* Flags for how the epicsTime can be messed up */
#define REVTIME_NONE   0
#define REVTIME_WORD   1 
#define REVTIME_OFFSET 2

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
extern void cleanup_ca(void);

/* xtc.cc */
extern void initialize_xtc(char *outfile);
extern int register_xtc(int sync, std::string name);
extern void configure_xtc(int id, char *buf, int size, unsigned int secs, unsigned int nsecs);
extern void data_xtc(int id, unsigned int secs, unsigned int nsecs, Pds::Xtc *hdr, int hdrlen, void *data);
extern void cleanup_xtc(void);
extern void xtc_stats(void);
