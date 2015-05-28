/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class ncsa_hdf_hdf5lib_H5_H5E */

#ifndef _Included_hdf_hdf5lib_H5_H5E
#define _Included_hdf_hdf5lib_H5_H5E
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eauto_is_v2
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_hdf_hdf5lib_H5_H5Eauto_1is_1v2
  (JNIEnv *, jclass, jlong);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eregister_class
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_hdf_hdf5lib_H5_H5Eregister_1class
  (JNIEnv *, jclass, jstring, jstring, jstring);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eunregister_class
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_hdf_hdf5lib_H5_H5Eunregister_1class
  (JNIEnv *, jclass, jlong);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eclose_msg
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_hdf_hdf5lib_H5_H5Eclose_1msg
  (JNIEnv *, jclass, jlong);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Ecreate_msg
 * Signature: (JILjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_hdf_hdf5lib_H5_H5Ecreate_1msg
  (JNIEnv *, jclass, jlong, jint, jstring);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Ecreate_stack
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_hdf_hdf5lib_H5_H5Ecreate_1stack
  (JNIEnv *, jclass);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eget_current_stack
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_hdf_hdf5lib_H5_H5Eget_1current_1stack
  (JNIEnv *, jclass);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eclose_stack
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_hdf_hdf5lib_H5_H5Eclose_1stack
  (JNIEnv *, jclass, jlong);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eprint1
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_hdf_hdf5lib_H5_H5Eprint1
  (JNIEnv *, jclass, jobject);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eprint2
 * Signature: (JLjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_hdf_hdf5lib_H5_H5Eprint2
  (JNIEnv *, jclass, jlong, jobject);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eget_class_name
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_hdf_hdf5lib_H5_H5Eget_1class_1name
  (JNIEnv *, jclass, jlong);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eset_current_stack
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_hdf_hdf5lib_H5_H5Eset_1current_1stack
  (JNIEnv *, jclass, jlong);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Epop
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_hdf_hdf5lib_H5_H5Epop
  (JNIEnv *, jclass, jlong, jlong);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eclear2
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_hdf_hdf5lib_H5_H5Eclear2
  (JNIEnv *, jclass, jlong);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eget_msg
 * Signature: (J[I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_hdf_hdf5lib_H5_H5Eget_1msg
  (JNIEnv *, jclass, jlong, jintArray);

/*
 * Class:     ncsa_hdf_hdf5lib_H5
 * Method:    H5Eget_num
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_hdf_hdf5lib_H5_H5Eget_1num
  (JNIEnv *, jclass, jlong);

#ifdef __cplusplus
}
#endif
#endif
