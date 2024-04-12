#include "helpers.h"

PyObject *python_call_file_func(const char *file, const char *func, const char *fmt, ...)
{
    PyObject *pName, *pModule, *pFunc, *pValue;
    pValue = NULL;
    pName = PyUnicode_DecodeFSDefault(file);
    /* Error checking of pName left out */
    assert(pName);
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL)
    {
        pFunc = PyObject_GetAttrString(pModule, func);
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {

            va_list argsptr;
            va_start(argsptr, fmt);
            PyObject *pArgs = Py_VaBuildValue(fmt, argsptr);
            assert(pArgs);
            pValue = PyObject_CallObject(pFunc, pArgs);
            //vprintf(fmt, argsptr);
            va_end(argsptr);
            if (pValue != NULL) {
                printf("Result of call: %ld\n", PyLong_AsLong(pValue));
                Py_DECREF(pValue);
            }
            else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
                fprintf(stderr,"Call failed\n");
            }
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", func);
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else
    {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", file);
    }
    return pValue;
}

void python_add_path(const char *path)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    char *script = (char*)malloc(512 * sizeof(char));
    // Prints "Hello world!" on hello_world
    snprintf(script, 512 * sizeof(char),
             "import sys\n"
             "if \"%s\" not in sys.path:\n"
             "\tsys.path.append(r\"%s/site-packages\")\n"
             "print(\"added path %s/site-packages to sys.path\")\n",
             path, path, path);
    int rc = PyRun_SimpleString(script);
    assert(rc == 0);
    PyGILState_Release(gstate);
    free(script);
}

void python_remove_path(const char *path)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    char *script = (char*)malloc(512 * sizeof(char));
    // Prints "Hello world!" on hello_world
    snprintf(script, 512 * sizeof(char),
             "import sys\n"
             "try:\n"
             "\tsys.path.remove(r\"%s/site-packages\")\n"
             "except Exception:\n"
             "\tpass\n"
             "print(\"removed path %s/site-packages from sys.path\")\n",
             path, path);
    int rc = PyRun_SimpleString(script);
    assert(rc == 0);
    PyGILState_Release(gstate);
    free(script);
}

zosc_t *
s_py_zosc(PyObject *pAddress, PyObject *pData)
{
    assert( PyUnicode_Check(pAddress) );
    assert( PyList_Check(pData) );
    PyObject *stringbytes = PyUnicode_AsASCIIString(pAddress);
    zosc_t *ret = zosc_new( PyBytes_AsString( stringbytes) );
    Py_DECREF(stringbytes);

    // iterate
    for ( Py_ssize_t i=0;i<PyList_Size(pData);++i )
    {
        PyObject *item = PyList_GetItem(pData, i);
        assert(item);
        // determine type, first check if boolean otherwise it will be an int
        if (PyBool_Check(item))
        {
            long b = PyLong_AsLong(item);
            if (b)
                zosc_append(ret, "T");
            else
                zosc_append(ret, "F");
        }
        else if ( PyLong_Check(item) )
        {
            long v = PyLong_AsLong(item);
            zosc_append(ret, "h", v);
        }
        else if (PyFloat_Check(item))
        {
            double v = PyFloat_AsDouble(item);
            zosc_append(ret, "d", v);
        }
        else if (PyUnicode_Check(item))
        {
            PyObject* ascii = PyUnicode_AsASCIIString(item);
            assert(ascii);
            char* s = PyBytes_AsString(ascii);
            zosc_append(ret, "s", s);
            Py_DECREF(ascii);
        }
        else if (PyBytes_Check(item)) // this can be used to force 32bit int, ie: struct.pack("I", 32)
        {
            Py_ssize_t bytesize = PyBytes_Size(item);
            if ( bytesize == 4 ) // 32bit int
            {
                char * buffer = PyBytes_AsString(item);
                uint32_t* vp = (uint32_t *)(buffer);
                zosc_append(ret, "i", *vp);
            }
            else
                zsys_warning("I don't know what to do with these bytes, please help: https://github.com/hku-ect/gazebosc");
        }
        else
        {
            // not a supported native python type try ctypes
            PyTypeObject* type = Py_TYPE(item);
            const char * pytypename = _PyType_Name(type);
            if (streq(pytypename, "c_int"))
            {
                zsys_warning("Unsupported ctypes.c_int until we find a way to access the value :S");
            }
            else
                zsys_warning("unsupported python type");
        }

    }

    return ret;
}

// https://stackoverflow.com/questions/2736753/how-to-remove-extension-from-file-name
char *
s_remove_ext(const char* myStr) {
    char *retStr;
    char *lastExt;
    if (myStr == NULL) return NULL;
    if ((retStr = (char *)malloc (strlen (myStr) + 1)) == NULL) return NULL;
    strcpy (retStr, myStr);
    lastExt = strrchr (retStr, '.');
    if (lastExt != NULL)
        *lastExt = '\0';
    return retStr;
}

char *
s_basename(char const *path)
{
    const char *s = strrchr(path, '/');
    if (!s)
        return strdup(path);
    else
        return strdup(s + 1);
}

bool
s_dir_exists(const wchar_t *pypath)
{
    // bleeh we need to convert to char as czmq only supports char paths
    char path[PATH_MAX];
    wcstombs(path, pypath, PATH_MAX);
    zfile_t *dir = zfile_new(NULL, path);
    assert(dir);
    bool ret = zfile_is_directory(dir);
    zfile_destroy(&dir);
    return ret;
}

void
s_replace_char(char *str, char from, char to)
{
    ssize_t length = strlen(str);
    for (int i = 0; i<length;i++)
    {
        if (str[i] == from )
        {
            str[i] = to;
        }
    }
}

char *
convert_to_relative_to_wd(const char *path)
{
    char wdpath[PATH_MAX];
    getcwd(wdpath, PATH_MAX);

#if WIN32
    for (int i = 0; wdpath[i] != '\0'; i++) {
        if (wdpath[i] == '\\') {
            wdpath[i] = '/';
        }
    }
    //std::string pathStr = wdpath;
    //std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
    //strcpy(wdpath, pathStr.c_str());
#endif

    const char *ret = strstr(path, wdpath);
    if (ret == NULL)
        return strdup(path);
    // working dir is found in path so only return the relative path
    assert(ret == path);
    ssize_t wdpathlen = strlen( wdpath ) + 1;// working dir path + first dir delimiter
    return strdup( &path[ wdpathlen ]);
}

char *gzb_itoa(int i)
{
    char *str = (char *)malloc(10);
    sprintf(str, "%d", i);
    return str;
}

char *ftoa(float f)
{
    char *str = (char *)malloc(30);
    sprintf(str, "%f", f);
    return str;
}
