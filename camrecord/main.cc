#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<ctype.h>
#include<signal.h>
#include<sys/select.h>
#include<sys/time.h>

#include<fstream>
#include<sstream>  //for std::istringstream
#include<iterator> //for std::istream_iterator
#include<vector>   //for std::vector

#include"yagxtc.hh"

using namespace std;

class symbol {
 public:
    symbol(string _name, string _det, string _camtype, string _pvname, int _binned)
        : name(_name), detector(_det), camtype(_camtype), pvname(_pvname), address(-1), is_bld(0), binned(_binned) {
        syms.push_back(this);
    };
    symbol(string _name, int addr) : name(_name), address(addr), is_bld(1) {
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
    int is_bld;
    int binned;
};

vector<symbol *> symbol::syms;
static int haveint = 0;
static fd_set all_fds;
static int maxfds = -1;
static int delay = 0;
static struct timeval start, stop;
int record_cnt = 0;
int *data_cnt = NULL;
static int nrec = 0;

static void int_handler(int signal)
{
    haveint = 1;
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
    data_cnt = (int *) calloc(nrec, sizeof(int));
    if (delay)
        alarm(delay);
    gettimeofday(&start, NULL);
}

static ifstream *open_config_file(char *name)
{
    char buf[512];

    if (!name)
        name = DEFAULT_CFG;
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
    if (s->is_bld) {
        printf("Found %s -> BLD at 239.255.24.%d\n", name.c_str(), s->address);
        create_bld(s->name, s->address, (string)(arg ? arg : "eth0"));
    } else {
        printf("Found %s -> CA to (%s,%s) at %s.\n", name.c_str(), 
               s->detector.c_str(), s->camtype.c_str(), s->pvname.c_str());
        create_ca(s->name, s->detector, s->camtype, s->pvname, s->binned);
    }
    nrec++;
}

static void read_config_file(const char *name)
{
    string line;
    ifstream *in = open_config_file(const_cast<char *>(name));
    int lineno = 0;

    if (!in)
        return;
    while(getline(*in, line)) {
        istringstream ss(line);
        istream_iterator<std::string> begin(ss), end;
        vector<string> arrayTokens(begin, end); 
        lineno++;

        if (arrayTokens.size() == 0 || arrayTokens[0] == "#") {
            continue;
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
        } else if (arrayTokens[0] == "bld") {
            if (arrayTokens.size() >= 3 && isdigit(arrayTokens[2][0]))
                new symbol(arrayTokens[1], atoi(arrayTokens[2].c_str()));
        } else if (arrayTokens[0] == "record") {
            if (arrayTokens.size() == 2)
                record(arrayTokens[1], NULL);
            else
                record(arrayTokens[1], arrayTokens[2].c_str());
        } else {
            record(arrayTokens[0], NULL);
        }
    }
    delete in;
}

static void initialize(char *config, char *outfile)
{
    FD_ZERO(&all_fds);
    initialize_bld();
    initialize_ca();
    read_config_file(config);
    initialize_xtc(outfile);
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
    handle_bld(&rfds);
    handle_ca(&rfds);
}

void cleanup(void)
{
    double runtime;
    int i;

    cleanup_bld();
    cleanup_ca();
    cleanup_xtc();

    gettimeofday(&stop, NULL);
    runtime = (1000000LL * stop.tv_sec + stop.tv_usec) - 
              (1000000LL * start.tv_sec + start.tv_usec);
    runtime /= 1000000.;
    fprintf(stderr, "Runtime: %.4lf seconds, Records: %d, Rate: %.2lf Hz\n",
            runtime, record_cnt, ((double) record_cnt) / runtime);
    fprintf(stderr, "Data counts:");
    for (i = 0; i < nrec; i++) {
        fprintf(stderr, " %d", data_cnt[i]);
    }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
    int c;
    char *config = NULL;
    char *outfile = NULL;
    int idx = 0;
    static struct option long_options[] = {
        {"help",     0, 0, 'h'},
        {"config",   1, 0, 'c'},
        {"output",   1, 0, 'o'},
        {"timeout",  1, 0, 't'},
        {NULL, 0, NULL, 0}
    };

    while ((c = getopt_long(argc, argv, "hc:o:t:", long_options, &idx)) != -1) {
        switch (c) {
        case 'h':
            fprintf(stderr, "Usage: camrecord [ OPTION ]...\n");
            fprintf(stderr, "Record camera PVs and BLDs into a file.  Options are:\n");
            fprintf(stderr, "    -h, --help                       = Print this help text.\n");
            fprintf(stderr, "    -c FILE, --config FILE           = Specify the configuration file.\n");
            fprintf(stderr, "    -o FILE, --output FILE           = The name of the XTC file to be saved.\n");
            fprintf(stderr, "    -t SECS, --timeout SECS          = Seconds to record after connecting.\n");
            fprintf(stderr, "If no timeout is specified, recording will continue until interrupted with ^C.\n");
            exit(0);
        case 'c':
            config = optarg;
            break;
        case 'o':
            outfile = optarg;
            break;
        case 't':
            delay = atoi(optarg);
            break;
        }
    }

    initialize(config, outfile);

    signal(SIGINT, int_handler);
    signal(SIGALRM, int_handler);

    while (!haveint) {
        do_poll();
    }

    cleanup();
}
