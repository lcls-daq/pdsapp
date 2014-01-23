#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<ctype.h>
#include<signal.h>
#include<sys/select.h>
#include<sys/time.h>
#include<string.h>

#include<fstream>
#include<sstream>  //for std::istringstream
#include<iterator> //for std::istream_iterator
#include<vector>   //for std::vector
#include<iostream>
#include"yagxtc.hh"

using namespace std;

class symbol {
 public:
    enum symtype { CAMERA_TYPE, BLD_TYPE, PV_TYPE };
    /* Camera */
    symbol(string _name, string _det, string _camtype, string _pvname, int _binned)
        : name(_name), detector(_det), camtype(_camtype), pvname(_pvname),
          address(-1), stype(CAMERA_TYPE), binned(_binned) {
        syms.push_back(this);
    };
    /* BLD */
    symbol(string _name, int addr, int _revtime)
        : name(_name), address(addr), stype(BLD_TYPE), revtime(_revtime) {
        syms.push_back(this);
    };
    /* PV */
    symbol(string _name, string _pvname, int _strict)
        : name(_name), pvname(_pvname), address(-1), stype(PV_TYPE), strict(_strict) {
        syms.push_back(this);
    };
    static symbol *find(string _name) {
        int i = syms.size() - 1;
        while (i >= 0) {
            if (syms[i]->name == _name)
                return syms[i];
            i--;
        }
        return NULL;
    }
 private:
    static vector<symbol *> syms;
 public:
    string name;
    string detector;
    string camtype;
    string pvname;
    int address;
    enum symtype stype;
    int binned;
    int revtime;
    int strict;
};

vector<symbol *> symbol::syms;
static int haveint = 0;
static fd_set all_fds;
static int maxfds = -1;
static int delay = 0;
static int keepalive = 0;
static struct timeval start, ka_finish, finish;
static struct itimerval timer;
static int running = 0;
int record_cnt = 0;
static int nrec = 0;
int verbose = 1;
int quiet = 0;
static char *outfile = NULL;
string hostname = "";
string prefix = "";
string username = "";
int expid = -1;
int runnum = -1;
int strnum = -1;
int haveH = 0;
string curdir = "";
string logbook[LCPARAMS];
static string lb_params[LCPARAMS] = { /* This is the order of parameters to LogBook::Connection::open! */
    "logbook_host=",
    "logbook_user=",
    "logbook_password=",
    "logbook_db=",
    "regdb_host=",
    "regdb_user=",
    "regdb_password=",
    "regdb_db=",
    "ifacedb_host=",
    "ifacedb_user=",
    "ifacedb_password=",
    "ifacedb_db=",
};
int start_sec = 0, start_nsec = 0;
int end_sec = 0, end_nsec = 0;

static void int_handler(int signal)
{
    haveint = 1;
    if (signal == SIGALRM) {
        fprintf(stderr, "%salarm expired!\n", prefix.c_str());
        fflush(stderr);
    } else
        printf("^C\n");
    fflush(stdout);
}

void add_socket(int s)
{
    FD_SET(s, &all_fds);
    if (s >= maxfds)
        maxfds = s + 1;
}

void remove_socket(int s)
{
    FD_CLR(s, &all_fds);
}

void begin_run(void)
{
    if (!quiet) {
        fprintf(stderr, "%sinitialized, recording...\n", prefix.c_str());
        fflush(stderr);
    }
    running = 1;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    if (keepalive && (!delay || keepalive < delay))
        timer.it_value.tv_sec = keepalive;
    else
        timer.it_value.tv_sec = delay;
    timer.it_value.tv_usec = 0;
    gettimeofday(&start, NULL);
    finish = start;
    ka_finish = start;
    if (delay) {
        ka_finish.tv_sec += delay - keepalive;
        finish.tv_sec += delay;
    }
    if (delay || keepalive)
        setitimer(ITIMER_REAL, &timer, NULL);
}

static istream *open_config_file(char *name)
{
    char buf[512];

    ifstream *res = new ifstream(name);
    if (res->good())
        return res;
    if (name[0] != '/') {
        sprintf(buf, "%s/%s", getenv("HOME"), name);
        res->open(buf);
        if (res->good())
            return res;
        sprintf(buf, "%s/%s", DEFAULT_DIR, name);
        res->open(buf);
        if (res->good())
            return res;
    }
    delete res;
    return NULL;
}

static void record(string name, const char *arg)
{
    symbol *s = symbol::find(name);
    if (!s) {
        printf("Can't find symbol %s!\n", name.c_str());
        exit(1);
    }
    switch (s->stype) {
    case symbol::BLD_TYPE:
        printf("Found %s -> BLD at 239.255.24.%d\n", name.c_str(), s->address);
        create_bld(s->name, s->address, (string)(arg ? arg : "eth0"), s->revtime);
        break;
    case symbol::CAMERA_TYPE:
        printf("Found %s -> CA to (%s,%s) at %s.\n", name.c_str(), 
               s->detector.c_str(), s->camtype.c_str(), s->pvname.c_str());
        create_ca(s->name, s->detector, s->camtype, s->pvname, s->binned, 1);
        break;
    case symbol::PV_TYPE:
        printf("Found %s -> CA to (EpicsArch, NoDevice) at %s.\n",
               name.c_str(), s->pvname.c_str());
        create_ca(s->name, "EpicsArch", "NoDevice", s->pvname, 0, s->strict);
        break;
    }
    nrec++;
}

static void *getstdin(char *buf, int n)
{
    int c, i;
    for (i = 0; i < n-1;) {
        c = read(0, &buf[i], 1);  /* This will hang.  But that's OK. */
        if (c < 0)
            return NULL; /* EOF! */
        if (buf[i++] == '\n') {
            buf[i] = 0;
            return buf;
        }
    }
    buf[i] = 0;
    return buf;
}

static void read_config_file(const char *name)
{
    char buf[1024];
    int lineno = 0;
    istream *in = NULL;
    string line;

    if (name) {
        in = open_config_file(const_cast<char *>(name));
        if (!in)
            return;
    }

    while(name ? getline(*in, line) : getstdin(buf, 1024)) {
        if (!name)
            line = buf;
        istringstream ss(line);
        istream_iterator<std::string> begin(ss), end;
        vector<string> arrayTokens(begin, end); 
        lineno++;

        if (arrayTokens.size() == 0 || arrayTokens[0][0] == '#' || 
            arrayTokens[0] == "camera-per-row" || arrayTokens[0] == "bld-per-row" ||
            arrayTokens[0] == "pv-per-row" || arrayTokens[0] == "host" ||
            arrayTokens[0] == "defhost" || arrayTokens[0] == "defport") {
            /* Ignore blank lines, comments, and client commands! */
            continue;
        } else if (arrayTokens[0] == "quiet") {
            quiet = 1;
        } else if (arrayTokens[0] == "starttime") {
            if (arrayTokens.size() >= 3) {
                start_sec = atoi(arrayTokens[1].c_str());
                start_nsec = atoi(arrayTokens[2].c_str());
            }
        } else if (arrayTokens[0] == "endtime") {
            if (arrayTokens.size() >= 3) {
                end_sec = atoi(arrayTokens[1].c_str());
                end_nsec = atoi(arrayTokens[2].c_str());
            }
        } else if (arrayTokens[0] == "dbinfo") {
            /* dbinfo username expid run stream */
            if (arrayTokens.size() >= 5) {
                username = arrayTokens[1];
                expid = atoi(arrayTokens[2].c_str());
                runnum = atoi(arrayTokens[3].c_str());
                strnum = atoi(arrayTokens[4].c_str());
            }
        } else if (arrayTokens[0] == "logbook") {
            if (arrayTokens.size() >= 2) {
                for (int idx = 0; idx < LCPARAMS; idx++) {
                    if (!arrayTokens[1].compare(0, lb_params[idx].length(), lb_params[idx])) {
                        logbook[idx] = arrayTokens[1].substr(lb_params[idx].length(), string::npos);
                        break;
                    }
                }
            }
        } else if (arrayTokens[0] == "end") {
            break;
        } else if (arrayTokens[0] == "trans") {
            if (arrayTokens.size() >= 4)
                do_transition(atoi(arrayTokens[1].c_str()),
                              atoi(arrayTokens[2].c_str()),
                              atoi(arrayTokens[3].c_str()),
                              atoi(arrayTokens[4].c_str()));
            break;
        } else if (arrayTokens[0] == "output") {
            outfile = strdup(arrayTokens[1].c_str());
        } else if (arrayTokens[0] == "hostname") {
            hostname = arrayTokens[1];
            if (!haveH)
                curdir = NFSBASE + arrayTokens[1] + "/";
            prefix = arrayTokens[1] + " ";
        } else if (arrayTokens[0] == "timeout") {
            delay = atoi(arrayTokens[1].c_str());
            if (delay < 0)
                delay = 0;
        } else if (arrayTokens[0] == "keepalive") {
            keepalive = atoi(arrayTokens[1].c_str());
            if (keepalive < 0)
                keepalive = 0;
        } else if (arrayTokens[0] == "include") {
            if (arrayTokens.size() >= 1)
                read_config_file(arrayTokens[1].c_str());
            else
                read_config_file(NULL);
        } else if (arrayTokens[0] == "camera") {
            if (arrayTokens.size() >= 5) {
                int binned = 0;
                if (arrayTokens.size() >= 6) {
                    if (arrayTokens[5] == "binned")
                        binned |= CAMERA_BINNED;
                    if (arrayTokens[5] == "roi")
                        binned |= CAMERA_ROI;
                    if (arrayTokens[5] == "size")
                        binned |= CAMERA_SIZE;
                    if (arrayTokens[5] == "areadet")
                        binned |= CAMERA_ADET;
                }
                printf("New symbol %s, binned = %d\n", arrayTokens[1].c_str(), binned);
                new symbol(arrayTokens[1], arrayTokens[2], arrayTokens[3], arrayTokens[4], binned);
            }
        } else if (arrayTokens[0] == "pv") {
            if (arrayTokens.size() >= 3) {
                int strict = (arrayTokens.size() >= 4 && arrayTokens[3] == "strict");
                new symbol(arrayTokens[1], arrayTokens[2].c_str(), strict);
            }
        } else if (arrayTokens[0] == "bld") {
            if (arrayTokens.size() >= 3 && isdigit(arrayTokens[2][0])) {
                int revtime = REVTIME_NONE;
                if (arrayTokens.size() >= 4) {
                    if (arrayTokens[3] == "revtime")
                        revtime = REVTIME_WORD;
                    else if (arrayTokens[3] == "offset")
                        revtime = REVTIME_OFFSET;
                    else if (arrayTokens[3] == "revtime-offset")
                        revtime = REVTIME_OFFSET | REVTIME_WORD;
                }
                new symbol(arrayTokens[1], atoi(arrayTokens[2].c_str()), revtime);
            }
        } else if (arrayTokens[0] == "record") {
            if (arrayTokens.size() == 2)
                record(arrayTokens[1], NULL);
            else
                record(arrayTokens[1], arrayTokens[2].c_str());
        } else {
            /* "record" is optional! */
            if (arrayTokens.size() == 1)
                record(arrayTokens[0], NULL);
            else
                record(arrayTokens[0], arrayTokens[1].c_str());
        }
    }
    if (in)
        delete in;
}

static void usage(void)
{
    printf("Usage: camrecord [ OPTION ]...\n");
    printf("Record camera PVs and BLDs into a file.  Options are:\n");
    printf("    -h, --help                       = Print this help text.\n");
    printf("    -c FILE, --config FILE           = Specify the configuration file.\n");
    printf("    -o FILE, --output FILE           = The name of the XTC file to be saved.\n");
    printf("    -t SECS, --timeout SECS          = Seconds to record after connecting.\n");
    printf("    -d DIRECTORY                     = Change to the specified directory.\n");
    printf("    -k SECS, --keepalive SECS        = Seconds to wait for input before closing down.\n");
    printf("    -s                               = Run silently (for use as a daemon).\n");
    printf("    -H HOSTNAME                      = Specify the hostname to use for the NFS mount point.\n");
    printf("If no timeout is specified, recording will continue until interrupted with ^C.\n");
    exit(0);
}

static void initialize(char *config)
{
    char *s;

    FD_ZERO(&all_fds);
    add_socket(0);
    initialize_bld();
    initialize_ca();
    read_config_file(config);
    if (!outfile) {
        printf("No output file specified!\n\n");
        usage();
        /* No return! */
    }
    if ((s = rindex(outfile, '/'))) { /* Make sure the directory exists! */
        char buf[1024];
        *s = 0;
        sprintf(buf, "mkdir -p %s", outfile);
        *s = '/';
        system(buf);
    }
    initialize_xtc(outfile);
}

static void stats(int v)
{
    double runtime;
    struct timeval now;

    if (running) {
        gettimeofday(&now, NULL);

        runtime = (1000000LL * now.tv_sec + now.tv_usec) - 
                  (1000000LL * start.tv_sec + start.tv_usec);
        runtime /= 1000000.;
        fprintf(stderr, "%sruntime: %.4lf seconds, Records: %d, Rate: %.2lf Hz\n",
                prefix.c_str(), runtime, record_cnt, ((double) record_cnt) / runtime);
        fflush(stderr);
    } else {
        fprintf(stderr, "%sstill waiting to initialize.\n", prefix.c_str());
        fflush(stderr);
    }
    if (v)
        xtc_stats();
}

static void process_command(char *buf)
{
    struct timeval now;

    printf("%s\n", buf);
    if (!strncmp(buf, "stop", 4)) {
        printf("Got stop!\n");
        if (buf[4]) {
            int sec, nsec;
            if (sscanf(buf, "stop %d %d", &sec, &nsec) == 2) {
                end_sec = sec;
                end_nsec = nsec;
            }
        }
        haveint = 1;
    } else if (!strncmp(buf, "trans", 5)) {
        int id;
        unsigned int sec, nsec, fid;
        if (sscanf(buf, "trans %d %d %d %d", &id, &sec, &nsec, &fid) == 4)
            do_transition(id, sec, nsec, fid);
    } else if (buf[0] == 0 || !strcmp(buf, "stats")) {
        gettimeofday(&now, NULL);
        if (delay && (now.tv_sec > ka_finish.tv_sec || 
                      (now.tv_sec == ka_finish.tv_sec && now.tv_usec > ka_finish.tv_usec))) {
            if (now.tv_sec > finish.tv_sec || 
                (now.tv_sec == finish.tv_sec && now.tv_usec > finish.tv_usec)) {
                int_handler(SIGALRM);      /* Past the time?!? */
                timer.it_value.tv_sec = keepalive;
                timer.it_value.tv_usec = 0;
            } else if (now.tv_usec <= ka_finish.tv_usec) {
                timer.it_value.tv_usec = ka_finish.tv_usec - now.tv_usec;
                timer.it_value.tv_sec = keepalive + ka_finish.tv_sec - now.tv_sec;
            } else {
                timer.it_value.tv_usec = 1000000 + ka_finish.tv_usec - now.tv_usec;
                timer.it_value.tv_sec = keepalive - 1 + ka_finish.tv_sec - now.tv_sec;
            }
        } else {
            timer.it_value.tv_sec = keepalive;
            timer.it_value.tv_usec = 0;
        }
        setitimer(ITIMER_REAL, &timer, NULL);
        stats(buf[0] != 0);
    } else {
        // This is an error, which we will silently ignore.
    }
}

static void handle_stdin(fd_set *rfds)
{
    if (FD_ISSET(0, rfds)) {
        char buf[1024];
        int cnt = read(0, buf, sizeof(buf) - 1);
        if (cnt > 0) {
            buf[cnt - 1] = 0; // Kill the newline!
            process_command(buf);
        } else {
            printf("Standard input is closed, terminating!\n");
            haveint = 1;
        }
    }
}

static void do_poll(void)
{
    struct timeval timeout;
    fd_set rfds = all_fds;
    int nfds;

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; /* 1ms */
    if ((nfds = select(maxfds, &rfds, NULL, NULL, &timeout)) < 0)   // If we got a signal, the masks are bad!
        return;
    handle_stdin(&rfds);
    handle_bld(&rfds);
    handle_ca(&rfds);
}

void cleanup(void)
{
    double runtime;
    struct timeval stop;

    gettimeofday(&stop, NULL);

    cleanup_bld();
    cleanup_ca();
    cleanup_xtc();

    if (running) {
        runtime = (1000000LL * stop.tv_sec + stop.tv_usec) - 
                  (1000000LL * start.tv_sec + start.tv_usec);
        runtime /= 1000000.;
        fprintf(stderr, "%sstopped, runtime: %.4lf seconds, Records: %d, Rate: %.2lf Hz\n",
                prefix.c_str(), runtime, record_cnt, ((double) record_cnt) / runtime);
        fflush(stderr);
    } else {
        fprintf(stderr, "%sstopped, failed to initialize, no data recorded!\n",
                prefix.c_str());
        fflush(stderr);
    }
}

int main(int argc, char **argv)
{
    int c;
    char *config = NULL;
    int idx = 0;
    static struct option long_options[] = {
        {"help",      0, 0, 'h'},
        {"config",    1, 0, 'c'},
        {"output",    1, 0, 'o'},
        {"timeout",   1, 0, 't'},
        {"directory", 1, 0, 'd'},
        {"silent",    0, 0, 's'},
        {"keepalive", 1, 0, 'k'},
        {"hostname",  1, 0, 'H'},
        {NULL, 0, NULL, 0}
    };

    while ((c = getopt_long(argc, argv, "hc:o:t:d:sk:H:", long_options, &idx)) != -1) {
        switch (c) {
        case 'h':
            usage();
            break;
        case 'c':
            config = optarg;
            break;
        case 'o':
            outfile = strdup(optarg);
            break;
        case 't':
            delay = atoi(optarg);
            break;
        case 'k':
            keepalive = atoi(optarg);
            break;
        case 'd':
            chdir(optarg);
            break;
        case 'H':
            curdir = ((std::string) NFSBASE) + optarg + "/";
            haveH = 1;
            break;
        case 's':
            verbose = 0;
            break;
        }
    }

    initialize(config);

    signal(SIGINT, int_handler);
    signal(SIGALRM, int_handler);

    while (!haveint) {
        do_poll();
    }

    cleanup();
    printf("Exiting.\n");
}
