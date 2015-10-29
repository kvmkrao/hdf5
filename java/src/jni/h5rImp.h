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

#include <jni.h>
/* Header for class hdf_hdf5lib_H5_H5R */

#ifndef _Included_hdf_hdf5lib_H5_H5R
#define _Included_hdf_hdf5lib_H5_H5R
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     hdf_hdf5lib_H5
 * Method:    H5Rcreate
 * Signature: ([BJLjava/lang/String;IJ)I
 */
JNIEXPORT jint JNICALL Java_hdf_hdf5lib_H5_H5Rcreate
  (JNIEnv *, jclass, jbyteArray, jlong, jstring, jint, jlong);

/*
 * Class:     hdf_hdf5lib_H5
 * Method:    _H5Rdereference
 * Signature: (JJI[B)J
 */
JNIEXPORT jlong JNICALL Java_hdf_hdf5lib_H5__1H5Rdereference
  (JNIEnv *, jclass, jlong, jlong, jint, jbyteArray);

/*
 * Class:     hdf_hdf5lib_H5
 * Method:    H5Rget_region
 * Signature: (JI[B)J
 */
JNIEXPORT jlong JNICALL Java_hdf_hdf5lib_H5__1H5Rget_1region
  (JNIEnv *, jclass, jlong, jint, jbyteArray);

/*
 * Class:     hdf_hdf5lib_H5
 * Method:    H5G_obj_t H5Rget_obj_type
 * Signature: (JI[B)I
 */
JNIEXPORT jint JNICALL Java_hdf_hdf5lib_H5_H5Rget_1obj_1type
  (JNIEnv *, jclass, jlong, jint, jbyteArray);

/*
 * Class:     hdf_hdf5lib_H5
 * Method:    int H5Rget_obj_type2
 * Signature: (JI[B[I)I
 */
JNIEXPORT jint JNICALL Java_hdf_hdf5lib_H5_H5Rget_1obj_1type2
  (JNIEnv *, jclass, jlong, jint, jbyteArray, jintArray);

/*
 * Class:     hdf_hdf5lib_H5
 * Method:    H5Rget_name
 * Signature: (JI[B[Ljava/lang/String;J)J
 */
JNIEXPORT jlong JNICALL Java_hdf_hdf5lib_H5_H5Rget_1name
  (JNIEnv *, jclass, jlong, jint, jbyteArray, jobjectArray, jlong);

#ifdef __cplusplus
}
#endif
#endif
