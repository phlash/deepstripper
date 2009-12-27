/* DeepStripper - A utiltity to extract audio from Akai DPS12/16 backup disks
 * or image files. This version uses GTK-2.0 for the GUI, with the intention
 * of being portable to Mac/Linux/Win32 platforms without carrying lots of
 * platform baggage (eg: JRE/Tk/CLR etc).
 *
 * Author: Phil Ashby
 * Date: October 2009
 */

#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#ifdef WIN32
#define GETCWD	_getcwd
#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <fcntl.h>
#include <share.h>
#else
#include <unistd.h>
#include <fcntl.h>
#define GETCWD	getcwd
#endif
#include "deepstripper.h"
#include "akaiosdisk.h"
#include "akaiosproject.h"

// Debug text to stdout
int g_dbg = 0;

// Text for about box
#define DS_VER_MAJOR	0
#define DS_VER_MINOR	1
#define ABOUT_TEXT "DeepStripper/GTK version %d.%d\nAuthor: Phil Ashby, phil.yahoo@ashbysoft.com\nEnjoy!"

// Default text for info box
#define DEFAULT_INFO	"Open a project.."

// Prototype for menu defs
static void menu(gpointer d, guint action, GtkWidget *w);

// Menu IDs
#define MID_OPEN12	100
#define MID_OPEN16	101
#define MID_CLOSE	102
#define MID_PROPS	103
#define MID_EXIT	104

#define MID_SELASS	200
#define MID_SELNOE	201
#define MID_SELALL	202
#define MID_SELNONE	203
#define MID_SELEXP	204

#define MID_MULTI	300

#define MID_HELP	999

// Menu definition
static GtkItemFactoryEntry root_menu[] = {
	{ "/_File/_Open DPS12...", "<CTRL>o", menu, MID_OPEN12, NULL },
	{ "/_File/Open DPS1_6...", "<CTRL>6", menu, MID_OPEN16, NULL },
	{ "/_File/_Close", "<CTRL>x", menu, MID_CLOSE, NULL },
	{ "/_File/sep", "", NULL, 0, "<Separator>" },
	{ "/_File/_Properties", "<CTRL>p", menu, MID_PROPS, NULL },
	{ "/_File/sep", "", NULL, 0, "<Separator>" },
	{ "/_File/E_xit", "<ALT>x", menu, MID_EXIT, NULL },
	{ "/_Multi", "", NULL, 0, "<Branch>" },
	{ "/_Track/Select A_ssigned", "<CTRL>g", menu, MID_SELASS, NULL },
	{ "/_Track/Select Non-_Empty", "<CTRL>n", menu, MID_SELNOE, NULL },
	{ "/_Track/Select _All", "<CTRL>a", menu, MID_SELALL, NULL },
	{ "/_Track/Select _None", "<CTRL>z", menu, MID_SELNONE, NULL },
	{ "/_Track/E_xport...", "<CTRL>s", menu, MID_SELEXP, NULL },
	{ "/_Help", "", NULL, 0, "<LastBranch>" },
	{ "/_Help/_About", "<CTRL>h", menu, MID_HELP, NULL }
};
#define MENU_SIZE	(sizeof(root_menu)/sizeof(GtkItemFactoryEntry))

// Globals
static char g_src[80];
static char g_dst[80];
static char g_prj[80];
static int g_fd = -1;				// current open device/file/source handle
static AkaiOsDisk g_disk;			// current open disk (if any)
static AkaiOsProject g_proj;		// current open project (if any)
static GtkWidget *g_main;			// main window, useful for dialogs etc.
static GtkWidget *g_info;			// information window, updated everywhere

// Notebook tabs
#define TAB_TRACKS	0
#define TAB_MIXES	1
#define TAB_MEMORY	2
#define N_TABS		3
static struct {
	char *tab;
	char *data;
} g_tabs[N_TABS] = {
	{ "Tracks", "Virtual Tracks:" },
	{ "Mix", "Mix Scenes:" },
	{ "Mem", "Memory Locations:" }
};
static GtkListStore *g_data[N_TABS];// Tracks, Mixes, Memory (see above)

// Bail out..
static gboolean quit(GtkWidget *w, GdkEvent *e, gpointer d) {
	gtk_main_quit();
	_DBG(_DBG_GUI,"Quitting\n");
	return FALSE;
}

// change main window title (and save values)
void set_title(char *src, char *dst, char *prj) {
	static char buf[80];
	snprintf(buf, sizeof(buf), "DeepStripper (%s->%s) - %s",
		src?src:g_src, dst?dst:g_dst, prj?prj:g_prj);
	if (src) strcpy(g_src, src);
	if (dst) strcpy(g_dst, dst);
	if (prj) strcpy(g_prj, prj);
	gtk_window_set_title(GTK_WINDOW(g_main), buf);
}

// Create a scrollable list box
static GtkWidget *make_list(char *data, gint idx) {
	GtkWidget *scroll, *tree, *list;
	GtkListStore *model;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scroll);

	g_data[idx] = gtk_list_store_new(1, G_TYPE_STRING);

	tree = gtk_tree_view_new();
	gtk_tree_selection_set_mode(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)),
		(GTK_SELECTION_MULTIPLE));
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(g_data[idx]));
	gtk_container_add(GTK_CONTAINER(scroll), tree);
	cell = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(data, cell, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	gtk_widget_show(tree);

	return scroll;
}

// Show an about box (or error message)
static void about_box(char *msg) {
	GtkWidget *about;
	if (msg) {
		about = gtk_message_dialog_new(GTK_WINDOW(g_main),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
        msg);
	} else {
		about = gtk_message_dialog_new(GTK_WINDOW(g_main),
		GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		ABOUT_TEXT, DS_VER_MAJOR, DS_VER_MINOR);
	}
	gtk_dialog_run(GTK_DIALOG(about));
	gtk_widget_destroy(about);
}

// Fill in a listbox with text items
static void populate(gint idx, gchar **data) {
	GtkTreeIter iter;

	while(data && *data) {
		gtk_list_store_append(GTK_LIST_STORE(g_data[idx]), &iter);
		gtk_list_store_set(GTK_LIST_STORE(g_data[idx]), &iter, 0, *data, -1);
		++data;
	}
}

// Manipulate multi menu
static void clear_multi() {
	// TODO:XXX
}

static void add_multi(char *name) {
	// TODO:XXX
}

// Set info text
static void set_info(char *info) {
	gtk_label_set_label(GTK_LABEL(g_info), info);
}

// Show project info
static void show_project_info() {
	char msg[80];
	if (g_proj.offset) {
		sprintf(msg, "Name: %s\nSample rate: %d\nSample size: %d\nDef Scene: %s",
			g_proj.name, g_proj.splrate, g_proj.splsize, g_proj.defscene.name);
		set_info(msg);
	}
}

// Close all data sources
static void close_backup() {
	// Empty all data
	int i;
	for (i=0; i<N_TABS; i++) {
		gtk_list_store_clear(g_data[i]);
	}
	set_info(DEFAULT_INFO);
	akaiosproject_clear(&g_proj);
	akaiosdisk_clear(&g_disk);
	if (g_fd>=0) {
		close(g_fd);
		g_fd = -1;
	}
	set_title("?", NULL, "");
}

// Open a specific project or entire backup (into g_proj)
static void open_project(int type, char *name) {
	if (g_fd<0) {
		about_box("Attempting to open a project without opening a backup");
		return;
	}
	if (name) {
		off64_t off = akaiosdisk_project(&g_disk, name);
		if (!off) {
			about_box("Specified project does not exist");
			return;
		}
		if (lseek64(g_fd, off, SEEK_SET)==(off64_t)-1) {
			about_box("Unable to seek to specified project");
			return;
		}
	}
	if (akaiosproject_read(g_fd, type, &g_proj)) {
		about_box("Unable to read project");
	}
	show_project_info();
}

// Open a new data source (into g_disk), and possibly the only project
static void open_backup(char *type, char *path) {
	int dps;
	if (strcmp(type, "-dps12")==0) {
		_DBG(_DBG_PRJ, "Opening DPS12 backup: %s\n", path);
		dps = AOSP_DPS12;
	} else if (strcmp(type, "-dps16")==0) {
		_DBG(_DBG_PRJ, "Opening DPS16 backup: %s\n", path);
		dps = AOSP_DPS16;
	} else {
		about_box("Invalid backup type specified");
		return;
	}
	if (g_fd>=0)
		close(g_fd);
#ifdef WIN32
	g_fd = _sopen(path, _O_RDONLY | _O_BINARY, _SH_DENYNO);
#else
	g_fd = open(path, O_RDONLY | O_LARGEFILE);
#endif
	if (g_fd<0) {
		about_box("Unable to open specified path");
		return;
	}
	_DBG(_DBG_PRJ, "trying multi-project reader..\n");
	if (akaiosdisk_read(g_fd, &g_disk)==0) {
		// It's a valid multi-project backup, populate menu
		// and open first project
		if (g_disk.dir) {
			_DBG(_DBG_PRJ, "populating multi-menu\n");
			AkaiOsDisk_Dirent *e = g_disk.dir;
			clear_multi();
			while(e) {
				add_multi(e->name);
				e = e->next;
			}
			open_project(dps, g_disk.dir->name);
		} else {
			about_box("Empty multi-project backup");
		}
	} else {
		// It might be a single project backup..
		_DBG(_DBG_PRJ, "trying single-project reader..\n");
		lseek64(g_fd, (off64_t)0, SEEK_SET);
		open_project(dps, NULL);
	}
}

// Select non-empty tracks
static void select_non_empty() {
	// XXX:TODO
}

// Extract selected tracks
static void extract_tracks() {
	// XXX:TODO
}

// File selection dialog..
static char *get_file(char *title, gboolean save) {
	static char *name = NULL;
	if (name)		// From previous call
		g_free(name);
	name = NULL;
	GtkWidget *file = gtk_file_chooser_dialog_new(title, GTK_WINDOW(g_main),
		save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	if (gtk_dialog_run(GTK_DIALOG(file))==GTK_RESPONSE_ACCEPT) {
		name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file));
	}
	gtk_widget_destroy(file);
	return name;
}

// Menu handler
static void menu(gpointer d, guint action, GtkWidget *w) {
	char *name;

	switch(action) {
	case MID_OPEN12:
		_DBG(_DBG_GUI,"open DPS12\n");
		name = get_file("Open DPS12 backup", FALSE);
		if (name)
			open_backup("-dps12", name);
		break;
	case MID_OPEN16:
		_DBG(_DBG_GUI,"open DPS16\n");
		name = get_file("Open DPS16 backup", FALSE);
		if (name)
			open_backup("-dps16", name);
		break;
	case MID_CLOSE:
		_DBG(_DBG_GUI,"close\n");
		close_backup();
		break;
	case MID_PROPS:
		_DBG(_DBG_GUI,"properties\n");
		show_project_info();
		break;
	case MID_EXIT:
		quit(w, NULL, d);
		break;

	case MID_SELASS:
		_DBG(_DBG_GUI,"select assigned\n");
		break;
	case MID_SELNOE:
		_DBG(_DBG_GUI,"select non-empty\n");
		break;
	case MID_SELALL:
		_DBG(_DBG_GUI,"select all\n");
		break;
	case MID_SELNONE:
		_DBG(_DBG_GUI,"select none\n");
		break;
	case MID_SELEXP:
		_DBG(_DBG_GUI,"export selected\n");
		break;

	case MID_HELP:
		_DBG(_DBG_GUI,"help about\n");
		about_box(NULL);
		break;

	default:
		if(0 <= action-MID_MULTI < g_disk.dirsize) {
			_DBG(_DBG_GUI,"change project: %d\n", action-MID_MULTI);
		} else {
			_DBG(_DBG_GUI,"unknown action: %d\n", action);
		}
		break;
	}
}

int main(int argc, char **argv) {
	char buf[80], *usage =
"usage: deepstripper-gtk [-d[ebug]] [-h[elp]] [-o[utput] <path>]\n"
"       [-dps12|-dps16 <path>] [-e[xtract] <project>]\n";
	GtkWidget *vbox, *men, *paned, *note, *frame, *scroll;
	GtkItemFactory *fac;
	GtkAccelGroup *acc;
	int i, dps = -1;

	for (i=1; i<argc; i++) {
		if (strncmp(argv[i], "-d", 2)==0) {
			g_dbg = g_dbg<<1 | 1;
		} else if (strncmp(argv[i], "-h", 2)==0) {
			g_print(usage);
			return 0;
		}
	}
	gtk_init(&argc, &argv);
	g_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(g_main), 600, 400);
	set_title("?", GETCWD(buf, sizeof(buf)), "");
	g_signal_connect(G_OBJECT(g_main), "delete_event", G_CALLBACK(quit), NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(g_main), vbox);
	gtk_widget_show(vbox);

	acc = gtk_accel_group_new();
	fac = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<deepstripper-main>", acc);
	gtk_item_factory_create_items(fac, MENU_SIZE, root_menu, 0);
	men = gtk_item_factory_get_widget(fac, "<deepstripper-main>");
	gtk_box_pack_start(GTK_BOX(vbox), men, FALSE, FALSE, 0);
	gtk_window_add_accel_group(GTK_WINDOW(g_main), acc);
	gtk_widget_show(men);

	paned = gtk_hpaned_new();
	gtk_container_set_border_width(GTK_CONTAINER(paned), 5);
	gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
	gtk_widget_show(paned);

	note = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(note), GTK_POS_BOTTOM);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(note), TRUE);
	gtk_notebook_set_homogeneous_tabs(GTK_NOTEBOOK(note), TRUE);
	gtk_widget_set_size_request(note, 150, 200);
	gtk_paned_pack1(GTK_PANED(paned), note, TRUE, FALSE);
	gtk_widget_show(note);

	for (i=0; i<N_TABS; i++) {
		GtkWidget *l = gtk_label_new(g_tabs[i].tab);
		gtk_widget_show(l);
		gtk_notebook_append_page(GTK_NOTEBOOK(note),
			make_list(g_tabs[i].data, i), l);
	}

	frame = gtk_frame_new("Item properties:");
	gtk_widget_set_size_request(frame, 150, 200);
	gtk_paned_pack2(GTK_PANED(paned), frame, TRUE, FALSE);
	gtk_widget_show(frame);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), scroll);
	gtk_widget_show(scroll);
	
	g_info = gtk_label_new(DEFAULT_INFO);
	gtk_label_set_justify(GTK_LABEL(g_info), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap(GTK_LABEL(g_info), FALSE);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), g_info);
	gtk_widget_show(g_info);

// Display everything
	gtk_widget_show(g_main);

// Process all other command line switches
	for (i=1; i<argc; i++) {
		if (strncmp(argv[i], "-o", 2)==0) {
			if (chdir(argv[++i]))
				about_box("unable to change directory");
			else
				set_title("?", GETCWD(buf, sizeof(buf)), "");
		} else if (strncmp(argv[i], "-dps", 4)==0) {
			open_backup(argv[i], argv[i+1]);
			if (strcmp(argv[i], "-dps12")==0)
				dps = AOSP_DPS12;
			else
				dps = AOSP_DPS16;
			++i;
		} else if (strncmp(argv[i], "-e", 2)==0) {
			if (dps<0)
				about_box("No backup specified to extract from");
			else {
				open_project(dps, argv[++i]);
				select_non_empty();
				extract_tracks();
			}
		} else if (strncmp(argv[i], "-d", 2)==0 ||
			strncmp(argv[i], "-h", 2)==0) {
			; // Skip earlier opts
		} else {
			about_box(usage);
		}
	}

// Run message pump..
	gtk_main();
	return 0;
}
