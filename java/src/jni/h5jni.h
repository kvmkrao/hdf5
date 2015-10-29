/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 *  For details of the HDF libraries, see the HDF Documentation at:
 *    http://hdfdfgroup.org/HDF5/doc/
 *
 */

#include "H5version.h"
#include <string.h>

#ifndef _Included_h5jni
#define _Included_h5jni

#ifdef __cplusplus
#define ENVPTR (env)
#define ENVPAR
#define ENVONLY
#else
#define ENVPTR (*env)
#define ENVPAR env,
#define ENVONLY env
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern jboolean h5JNIFatalError(JNIEnv *, char *);
extern jboolean h5nullArgument(JNIEnv *, char *);
extern jboolean h5badArgument (JNIEnv *, char *);
extern jboolean h5outOfMemory (JNIEnv *, char *);
extern jboolean h5libraryError(JNIEnv *env );
extern jboolean h5raiseException(JNIEnv *, char *, char *);
extern jboolean h5unimplemented( JNIEnv *env, char *functName);

/* implemented at H5.c */
extern jint get_enum_value(JNIEnv *env, jobject enum_obj);
extern jobject get_enum_object(JNIEnv *env, const char* enum_class_name,
    jint enum_val, const char* enum_field_desc);

/* implemented at H5G.c */
extern jobject create_H5G_info_t(JNIEnv *env, H5G_info_t group_info);

#ifdef __cplusplus
}
#endif

#endif
