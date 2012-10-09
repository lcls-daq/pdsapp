#include"pdsdata/xtc/Xtc.hh"
#include<string>

#define DEFAULT_DIR "/reg/neh/home/mcbrowne/lib"
#define DEFAULT_CFG ".camrecord"

/* main.c */
extern void add_socket(int s);
extern void remove_socket(int s);
extern void begin_run(void);
extern int record_cnt;
extern int *data_cnt;

/* bld.c */
extern void initialize_bld(void);
extern void create_bld(std::string name, int address, std::string device);
extern void handle_bld(fd_set *rfds);
extern void cleanup_bld(void);

/* ca.c */
extern void initialize_ca(void);
extern void create_ca(std::string name, std::string detector, std::string camtype, std::string pvname, int binned);
extern void handle_ca(fd_set *rfds);
extern void cleanup_ca(void);

/* xtc.c */
extern void initialize_xtc(char *outfile);
extern int register_xtc(void);
extern void configure_xtc(int id, Pds::Xtc *xtc);
extern void data_xtc(int id, int fid, unsigned int secs, unsigned int nsecs, Pds::Xtc *hdr, int hdrlen, void *data);
extern void cleanup_xtc(void);
