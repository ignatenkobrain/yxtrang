#ifndef SCRIPTLET_H
#define SCRIPTLET_H

typedef struct scriptlet_* scriptlet;
typedef struct hscriptlet_* hscriptlet;

// Create a new scriptlet

extern scriptlet scriptlet_open(const char* text);

// Create a run-time environment for the scriptlet.

extern hscriptlet scriptlet_prepare(scriptlet s);

extern int scriptlet_set_int(hscriptlet r, const char* k, long long v);
extern int scriptlet_set_real(hscriptlet r, const char* k, double v);
extern int scriptlet_set_string(hscriptlet r, const char* k, const char* v);
extern int scriptlet_get_int(hscriptlet r, const char* k, long long* v);
extern int scriptlet_get_real(hscriptlet r, const char* k, double* v);
extern int scriptlet_get_string(hscriptlet r, const char* k, const char** v);

// Run and cleanup.

extern int scriptlet_run(hscriptlet r);
extern int scriptlet_done(hscriptlet r);

// Free-up scriptlet.

extern int scriptlet_close(scriptlet s);

#endif
