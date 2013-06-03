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
static char *outfile = NULL;
string hostname = "";

static void int_handler(int signal)
{
    haveint = 1;
    if (signal == SIGALRM) {
        fprintf(stderr, "%salarm expired!\n", hostname.c_str());
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
    fprintf(stderr, "%sinitialized, recording...\n", hostname.c_str());
    fflush(stderr);
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

    if (!name)
        return &cin;
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

static void read_config_file(const char *name)
{
    string line;
    istream *in = open_config_file(const_cast<char *>(name));
    int lineno = 0;

    if (!in)
        return;
    while(getline(*in, line)) {
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
        } else if (arrayTokens[0] == "end") {
            break;
        } else if (arrayTokens[0] == "output") {
            outfile = strdup(arrayTokens[1].c_str());
        } else if (arrayTokens[0] == "hostname") {
            hostname = arrayTokens[1] + " ";
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
                int binned = (arrayTokens.size() >= 6 && arrayTokens[5] == "binned");
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
    if (in != &cin)
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

static void stats(int verbose)
{
    double runtime;
    struct timeval now;

    if (running) {
        gettimeofday(&now, NULL);

        runtime = (1000000LL * now.tv_sec + now.tv_usec) - 
                  (1000000LL * start.tv_sec + start.tv_usec);
        runtime /= 1000000.;
        fprintf(stderr, "%sruntime: %.4lf seconds, Records: %d, Rate: %.2lf Hz\n",
                hostname.c_str(), runtime, record_cnt, ((double) record_cnt) / runtime);
        fflush(stderr);
    } else {
        fprintf(stderr, "%sstill waiting to initialize.\n", hostname.c_str());
        fflush(stderr);
    }
    if (verbose)
        xtc_stats();
}

static void handle_stdin(fd_set *rfds)
{
    struct timeval now;
    if (FD_ISSET(0, rfds)) {
        char buf[1024];
        int cnt = read(0, buf, sizeof(buf) - 1);
        if (cnt > 0) {
            buf[cnt - 1] = 0; // Kill the newline!
            printf("%s\n", buf);
            if (!strcmp(buf, "stop")) {
                printf("Got stop!\n");
                haveint = 1;
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
                hostname.c_str(), runtime, record_cnt, ((double) record_cnt) / runtime);
        fflush(stderr);
    } else {
        fprintf(stderr, "%sstopped, failed to initialize, no data recorded!\n",
                hostname.c_str());
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
        {"keepalive", 1, 0, 's'},
        {NULL, 0, NULL, 0}
    };

    while ((c = getopt_long(argc, argv, "hc:o:t:d:sk:", long_options, &idx)) != -1) {
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
