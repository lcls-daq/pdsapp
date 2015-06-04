#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<ctype.h>
#include<signal.h>
#include<sys/select.h>
#include<sys/time.h>
#include<string.h>

#include"cadef.h"
#include"alarm.h"

char *pvlist[] = {
    "OTRS:DMP1:695:Image:ArrayData",
    "OTRS:DMP1:695:MinX_RBV",
    "OTRS:DMP1:695:MinY_RBV",
    "OTRS:DMP1:695:SizeX_RBV",
    "OTRS:DMP1:695:SizeY_RBV",
    "OTRS:DMP1:695:RESOLUTION",
    "OTRS:DMP1:695:ArrayRate_RBV",
    "OTRS:DMP1:695:AcquireTime_RBV",
    "OTRS:DMP1:695:TCAL_X",
    "OTRS:DMP1:695:BLEN",
    "KLYS:DMP1:1:MOD.RVAL",
    "REFS:DMP1:400:EDES",
    "SIOC:SYS0:ML01:AO212",
    "SIOC:SYS0:ML01:AO213",
    "SIOC:SYS0:ML01:AO214",
    "SIOC:SYS0:ML01:AO215",
    "SIOC:SYS0:ML01:AO216",
    "OTRS:DMP1:695:FLT1_PNEU.RVAL",
    "OTRS:DMP1:695:FLT2_PNEU.RVAL",
    "OTRS:DMP1:695:PNEUMATIC.RVAL",
    "FOIL:LI24:804:LVPOS",
    "FOIL:LI24:804:MOTR.VAL",
    NULL
};

typedef struct caconn {
    char  *name;
    chid   chan;
    int    nelem;
    chtype dbftype;
    long   dbrtype;
    int    size;
    evid   event;
    int    connected;
    int    is_cam;
    int    cnt;
    int    strict;
} caconn;

caconn caconns[30];
int caconncnt = 0;

static fd_set all_fds;
static int haveint = 0;
static int maxfds = -1;

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

static void fd_handler(void *parg, int fd, int opened)
{
    if (opened)
        add_socket(fd);
    else
        remove_socket(fd);
}


static void event_handler(struct event_handler_args args)
{
    caconn *c = (caconn *)args.usr;

    if (args.status != ECA_NORMAL) {
        printf("Bad status: %d\n", args.status);
    } else if (args.type == c->dbrtype && args.count == c->nelem) {
        c->cnt++;
    } else {
        printf("type = %ld, count = %ld -> expected type = %ld, count = %d\n",
                args.type, args.count, c->dbrtype, c->nelem);
    }
    fflush(stdout);
}

static void get_handler(struct event_handler_args args)
{
    caconn *c = (caconn *)args.usr;
    int status = ca_create_subscription(c->dbrtype, c->nelem, c->chan, DBE_VALUE | DBE_ALARM,
                                        event_handler, (void *) c, &c->event);
    if (status != ECA_NORMAL) {
        printf("Failed to create subscription! error %d!\n", status);
        exit(0);
    }
}

static void connection_handler(struct connection_handler_args args)
{
    caconn *c = (caconn *)ca_puser(args.chid);

    if (args.op == CA_OP_CONN_UP) {
        printf("%s is connected.\n", c->name);
        c->connected = 1;
        if (c->nelem == -1)
            c->nelem = ca_element_count(args.chid);
        c->dbftype = ca_field_type(args.chid);
        if (c->dbftype == DBF_LONG && c->is_cam)
            c->dbrtype = DBR_TIME_SHORT;             /* Force this, since we know the cameras are at most 16 bit!!! */
        else
            c->dbrtype = dbf_type_to_DBR_TIME(c->dbftype);
        c->size = dbr_size_n(c->dbrtype, c->nelem);
        if (c->is_cam) {
            int status = ca_create_subscription(c->dbrtype, c->nelem, args.chid, DBE_VALUE | DBE_ALARM,
                                                event_handler, (void *) c, &c->event);
            if (status != ECA_NORMAL) {
                printf("Failed to create subscription! error %d!\n", status);
                exit(0);
            }
        } else {
            int status = ca_array_get_callback(dbf_type_to_DBR_CTRL(c->dbftype), c->nelem, args.chid, get_handler, (void *) c);
            if (status != ECA_NORMAL) {
                printf("Get failed! error %d!\n", status);
                exit(0);
            }
        }
    } else {
        c->connected = 0;
    }
    fflush(stdout);
}

void initialize_ca(void)
{
    int i = 0;
    ca_add_fd_registration(fd_handler, NULL);
    ca_context_create(ca_disable_preemptive_callback);

    while (pvlist[i]) {
        chid chan;
        int result = ca_create_channel(pvlist[i],
                                       connection_handler,
                                       &caconns[i],
                                       50, /* priority?!? */
                                       &chan);
        caconns[i].name = pvlist[i];
        caconns[i].chan = chan;
        caconns[i].connected = 0;
        caconns[i].nelem = -1;
        caconns[i].cnt = 0;
        caconns[i].strict = 0;
        caconns[i].is_cam = (i == 0);
        if (result != ECA_NORMAL) {
            printf("CA error %s while creating channel to %s!\n",
                   ca_message(result), pvlist[i]);
            exit(0);
        }
        ca_poll();
        caconncnt = ++i;
    }
}

void begin_run_ca(void)
{
    int i;
    caconn *c;

    for (i = 0, c = &caconns[i]; i < caconncnt; c = &caconns[++i]) {
        if (!c->is_cam && !c->strict) {
            int status = ca_array_get_callback(c->dbrtype, c->nelem, c->chan, event_handler, (void *) c);
            if (status != ECA_NORMAL) {
                printf("Get failed! error %d (%s)!\n", status, ca_message(status));
                printf("i = %d, name = %s\n", i, c->name);
                exit(0);
            }
        }
    }
}

static void int_handler(int signal)
{
    haveint = 1;
    if (signal == SIGALRM) {
        fprintf(stderr, "alarm expired!\n");
        fflush(stderr);
    } else
        printf("^C\n");
    fflush(stdout);
}

static void handle_stdin(fd_set *rfds)
{
    if (FD_ISSET(0, rfds)) {
        char buf[1024];
        int cnt = read(0, buf, sizeof(buf) - 1);
        if (cnt > 0) {
            static int first = 1;
            int i;
            if (first) {
                first = 0;
                begin_run_ca();
            }
            for (i = 0; i < caconncnt; i++) {
                printf("%5d   %s\n", caconns[i].cnt, caconns[i].name);
            }
            printf("\n");
        } else {
            printf("Standard input is closed, terminating!\n");
            haveint = 1;
        }
    }
}

void handle_ca(fd_set *rfds)
{
    ca_poll();
}

static void do_poll(void)
{
    struct timeval timeout;
    fd_set rfds = all_fds;
    int nfds;

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; /* 1ms */
    if ((nfds = select(maxfds, &rfds, NULL, NULL, &timeout)) < 0) {  // If we got a signal, the masks are bad!
        printf("Select failure?!?\n");
        return;
    }
    handle_stdin(&rfds);
    handle_ca(&rfds);
}

int main(int argc, char **argv)
{
    FD_ZERO(&all_fds);
    add_socket(0);
    initialize_ca();

    signal(SIGINT, int_handler);
    signal(SIGALRM, int_handler);
    sigignore(SIGIO);

    while (!haveint) {
        do_poll();
    }

}
