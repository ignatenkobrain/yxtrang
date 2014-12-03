#ifndef SCRIPTLET_H
#define SCRIPTLET_H

typedef struct _scriptlet* scriptlet;
typedef struct _runtime* runtime;

// Create a new scriptlet

extern scriptlet scriptlet_open(const char* text);

// Create a run-time environment for the scriptlet.

extern runtime scriptlet_prepare(scriptlet s);

extern int scriptlet_set_int(runtime r, const char* k, long long v);
extern int scriptlet_set_real(runtime r, const char* k, double v);
extern int scriptlet_set_string(runtime r, const char* k, const char* v);
extern int scriptlet_get_int(runtime r, const char* k, long long* v);
extern int scriptlet_get_real(runtime r, const char* k, double* v);
extern int scriptlet_get_string(runtime r, const char* k, const char** v);

// Run and cleanup.

extern int scriptlet_run(runtime r);
extern int scriptlet_done(runtime r);

// Free-up scriptlet.

extern int scriptlet_close(scriptlet s);

#endif
