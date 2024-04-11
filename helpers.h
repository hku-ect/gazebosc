#ifndef HELPERS_H
#define HELPERS_H
#include "Python.h"
#include "czmq.h"

#ifdef __cplusplus
extern "C" {
#endif

PyObject *python_call_file_func(const char *file, const char *func, const char *fmt, ...);
void python_add_path(const char *path);
void python_remove_path(const char *path);
zosc_t *s_py_zosc(PyObject *pAddress, PyObject *pData);
char *s_remove_ext(const char* myStr);
char *s_basename(char const *path);
bool s_dir_exists(const wchar_t *pypath);
void s_replace_char(char *str, char from, char to);
char *convert_to_relative_to_wd(const char *path);
char *itoa(int i);
char *ftoa(float f);
#ifdef __cplusplus
}
#endif

#endif // HELPERS_H
