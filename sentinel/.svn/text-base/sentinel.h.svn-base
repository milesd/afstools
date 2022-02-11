#include <sys/types.h>
#include <X11/Intrinsic.h>

#define _NO_PROTO		/* No motif function prototypes */

#include <Xm/Xm.h>

#define DEF_CONFIG	"/afs/andrew.cmu.edu/data/db/sentinel/sentinel.config"
#define DEF_LOGFILE	"/tmp/sentinel.log"
#define DEF_FONT	"6x10"
#define BUFLEN		512
#define MAX_NAME_LEN	64

/* Server warning flags */
#define SW_DOWN		0x1

/* Disk warning flags */
#define DW_PERCENT_FULL	0x1
#define DW_BLOCKS_AVAIL	0x2

#define DW (DW_PERCENT_FULL | DW_BLOCKS_AVAIL)

/* Display modes */
#define MODE_CLEAR	0
#define MODE_WARN	1

/* Table entries */
#define FORMAT_PERCENT	0
#define FORMAT_BLOCKS	1

/* Label indices */
#define SERVER_NAME	0
#define BOOT_TIME	1
#define START_TIME	2
#define DISK_1		3
#define DISK_2		4
#define DISK_3		5
#define DISK_4		6
#define DISK_5		7
#define DISK_6		8
#define DISK_7		9
#define DISK_8		10
#define DISK_9		11
#define DISK_10		12

#define NUM_LABELS	13

#define NULL_LABEL	"     ---     "
#define SUPER_LABEL	"Supervisor"
#define TROUBLE_LABEL	"Trouble"
#define NITPICK_LABEL	"Nitpicker"
#define LOG_LABEL	"Log"
#define BORDER_WIDTH	2

#define XmNconfigFile "configFile"
#define XmCConfigFile "ConfigFile"
#define XmNlogFile "logFile"
#define XmCLogFile "LogFile"
#define XmNtroubleForeground "troubleForeground"
#define XmCTroubleForeground "TroubleForeground"
#define XmNtroubleBackground "troubleBackground"
#define XmCTroubleBackground "TroubleBackground"
#define XmNinitialWindow "initialWindow"
#define XmCInitialWindow "InitialWindow"
#define XmNlogVisible "logVisible"
#define XmCLogVisible "LogVisible"
#define XmNtroubleGeometry "troubleGeometry"
#define XmCTroubleGeometry "TroubleGeometry"
#define XmNnitpickerGeometry "nitpickerGeometry"
#define XmCNitpickerGeometry "NitpickerGeometry"
#define XmNsupervisorGeometry "supervisorGeometry"
#define XmCSupervisorGeometry "SupervisorGeometry"
#define XmNlogGeometry "logGeometry"
#define XmCLogGeometry "LogGeometry"
#define XmNprobeDelay "probeDelay"
#define XmCProbeDelay "ProbeDelay"

/* this is *supposed* to be a string */
#define PROBE_DELAY "30"

#define MALLOC_ERROR() \
{ \
    fprintf(stderr, "Malloc failure\n"); \
    exit(1); \
}

struct thresholds
{
    int percent_full;
    int blocks_avail;
};

struct disk {
    long warning;
    char name[MAX_NAME_LEN];
    long last_blocks_avail;
    long last_percent_full;
    struct thresholds thresholds;
};

struct server {
    char name[MAX_NAME_LEN];
    long warning;
    int disk_count;
    u_long boot_time;
    u_long start_time;
    struct disk *disks;
    struct server *next;

/* These should not be hard-coded.  Blame Transarc.
   See <afs/afsint.h> for more details */
    struct disk Disk1;
    struct disk Disk2;
    struct disk Disk3;
    struct disk Disk4;
    struct disk Disk5;
    struct disk Disk6;
    struct disk Disk7;
    struct disk Disk8;
    struct disk Disk9;
    struct disk Disk10;
};

typedef struct {
    Boolean log_visible;
    Dimension border_width;
    Pixel border_color;
    Pixel bg;
    Pixel fg;
    Pixel t_bg;
    Pixel t_fg;
    String config_file;
    String log_file;
    String initial_window;
    String probe_delay;
    String log_geometry;
    String nitpicker_geometry;
    String supervisor_geometry;
    String trouble_geometry;
    XFontStruct *font_struct;
    XmFontList font_list;
} AppData;

/* button callbacks */
extern void log_button_cb();
extern void switch_window_cb();
extern void quit_cb();
extern void probe_cb();
extern void format_cb();

extern void receive_data();
extern void initialize_data();
extern void process_data();

extern void parse_config();
extern void open_log_file();
extern void close_log_file();

extern void create_windows();
extern void change_label();
extern void switch_window();
extern void switch_format();
extern void toggle_log();
extern void append_log();
extern void flip_label();
extern void update_supervisor();

extern struct server **servers;
extern int num_servers;
extern AppData app_data;
