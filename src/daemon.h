extern volatile int g_stopping;

#ifdef _WIN32
int daemonize(int argc, char *argv[], int watchdog);
#else
int daemonize(int watchdog);
#endif
