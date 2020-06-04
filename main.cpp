#include "mainwindow.h"
#include <QApplication>
#include <QDesktopWidget>

#include <stdio.h>
#include <unistd.h>
#include <execinfo.h>
#include <signal.h>


static char g_bin[256] = {0,}; 
#define MAX_DEPTH_BACKTRACE 100
void dump_stack(void)
{
	static void *array[MAX_DEPTH_BACKTRACE];
	char dump_fname[256] = {0,};
	sprintf(dump_fname, "%s.dump", g_bin);
	FILE *dump_fd = fopen(dump_fname, "w");
	if(!dump_fd) {
		printf("failed to create dump file[%s]!\n", dump_fname);
		return;
	}
	
	// get void*'s for all entries on the stack
	size_t size = backtrace(array, MAX_DEPTH_BACKTRACE);	// print out all the frames
	backtrace_symbols_fd(array, size, fileno(dump_fd));
	fclose(dump_fd);
}

static void exception_handler(int sig)
{
	printf("%s got signal[%d]!\n", g_bin, sig);
	
	dump_stack();

	signal(sig, SIG_DFL);
	raise(sig);
}

static void reg_sig_handler(const char *path)
{
	strcpy(g_bin, path);
	signal(SIGFPE, exception_handler);
	signal(SIGILL, exception_handler);
	signal(SIGSEGV, exception_handler);
	signal(SIGTERM, exception_handler);
	signal(SIGQUIT, exception_handler);
	signal(SIGKILL, exception_handler);
}

int main(int argc, char *argv[])
{
/*
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
*/

    reg_sig_handler(argv[0]);

    QApplication a(argc, argv);
    MainWindow w;

    if(argc > 1 && !strcmp("-full", argv[1]))
        w.showFullScreen();
    else
        w.show();

    QRect r = w.geometry();
    r.moveCenter(QApplication::desktop()->availableGeometry().center());
    w.setGeometry(r);


    int result = a.exec();

    return result;

}
