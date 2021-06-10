#pragma once
#include <stddef.h>
int exec(const char *name, char *const argv[]);

// Create and bind a user thread to current running task.
// Exec as user code
void exec_user(const char *name, char *const argv[]);