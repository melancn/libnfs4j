/*
 * JNI binding for libnfs, wrapping the synchronous libnfs API for the
 * Java classes in jni/java/org/libnfs4j/.
 *
 * Copyright (c) 2026 - libnfs4j contributors
 * Licenced under LGPL 2.1, the same terms as libnfs itself.
 */

#include <jni.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nfsc/libnfs.h>

/* ------------------------------------------------------------------ */
/* helpers                                                             */
/* ------------------------------------------------------------------ */

static struct nfs_context *to_nfs(jlong ctx)
{
	return (struct nfs_context *)(intptr_t)ctx;
}

static struct nfsfh *to_fh(jlong fh)
{
	return (struct nfsfh *)(intptr_t)fh;
}

/*
 * Throw org.libnfs4j.NfsException with the libnfs error string and the
 * (negative errno) return value. Caller must return immediately after.
 */
static void throw_nfs(JNIEnv *env, struct nfs_context *nfs, int ret,
                      const char *what)
{
	jclass cls;
	jmethodID ctor;
	jstring jmsg;
	char buf[1024];
	const char *err;

	err = (nfs != NULL) ? nfs_get_error(nfs) : NULL;
	if (err == NULL) {
		err = "unknown error";
	}
	snprintf(buf, sizeof(buf), "%s: %s (errno %d)", what, err, ret);
	buf[sizeof(buf) - 1] = '\0';

	cls = (*env)->FindClass(env, "org/libnfs4j/NfsException");
	if (cls == NULL) {
		return; /* FindClass already threw */
	}
	ctor = (*env)->GetMethodID(env, cls, "<init>",
	                           "(Ljava/lang/String;I)V");
	jmsg = (*env)->NewStringUTF(env, buf);
	{
		jthrowable ex = (jthrowable)(*env)->NewObject(env, cls, ctor,
		                                              jmsg, (jint)ret);
		(*env)->Throw(env, ex);
	}
}

/* Fill a long[13] in the order expected by org.libnfs4j.NfsStat. */
static jlongArray stat_to_java(JNIEnv *env, const struct nfs_stat_64 *st)
{
	jlongArray arr;
	jlong v[13];

	v[0]  = (jlong)st->nfs_dev;
	v[1]  = (jlong)st->nfs_ino;
	v[2]  = (jlong)st->nfs_mode;
	v[3]  = (jlong)st->nfs_nlink;
	v[4]  = (jlong)st->nfs_uid;
	v[5]  = (jlong)st->nfs_gid;
	v[6]  = (jlong)st->nfs_rdev;
	v[7]  = (jlong)st->nfs_size;
	v[8]  = (jlong)st->nfs_blksize;
	v[9]  = (jlong)st->nfs_blocks;
	v[10] = (jlong)st->nfs_atime;
	v[11] = (jlong)st->nfs_mtime;
	v[12] = (jlong)st->nfs_ctime;

	arr = (*env)->NewLongArray(env, 13);
	if (arr == NULL) {
		return NULL;
	}
	(*env)->SetLongArrayRegion(env, arr, 0, 13, v);
	return arr;
}

/* ------------------------------------------------------------------ */
/* org.libnfs4j.NfsClient                                              */
/* ------------------------------------------------------------------ */

JNIEXPORT jlong JNICALL
Java_org_libnfs4j_NfsClient_nativeInitContext(JNIEnv *env, jclass clazz)
{
	struct nfs_context *nfs = nfs_init_context();
	if (nfs == NULL) {
		throw_nfs(env, NULL, -1, "nfs_init_context failed");
		return 0;
	}
	return (jlong)(intptr_t)nfs;
}

JNIEXPORT void JNICALL
Java_org_libnfs4j_NfsClient_nativeDestroyContext(JNIEnv *env, jclass clazz,
                                                 jlong jctx)
{
	struct nfs_context *nfs = to_nfs(jctx);
	if (nfs != NULL) {
		nfs_destroy_context(nfs);
	}
}

JNIEXPORT void JNICALL
Java_org_libnfs4j_NfsClient_nativeMount(JNIEnv *env, jclass clazz,
                                        jlong jctx, jstring jserver,
                                        jstring jexport)
{
	struct nfs_context *nfs = to_nfs(jctx);
	const char *server = (*env)->GetStringUTFChars(env, jserver, NULL);
	const char *export = (*env)->GetStringUTFChars(env, jexport, NULL);
	int ret;

	ret = nfs_mount(nfs, server, export);
	(*env)->ReleaseStringUTFChars(env, jserver, server);
	(*env)->ReleaseStringUTFChars(env, jexport, export);

	if (ret != 0) {
		throw_nfs(env, nfs, ret, "mount failed");
	}
}

JNIEXPORT void JNICALL
Java_org_libnfs4j_NfsClient_nativeUmount(JNIEnv *env, jclass clazz, jlong jctx)
{
	struct nfs_context *nfs = to_nfs(jctx);
	int ret = nfs_umount(nfs);
	if (ret != 0) {
		throw_nfs(env, nfs, ret, "umount failed");
	}
}

JNIEXPORT jlongArray JNICALL
Java_org_libnfs4j_NfsClient_nativeStat(JNIEnv *env, jclass clazz, jlong jctx,
                                       jstring jpath, jboolean jlstat)
{
	struct nfs_context *nfs = to_nfs(jctx);
	struct nfs_stat_64 st;
	const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
	int ret;

	if (jlstat) {
		ret = nfs_lstat64(nfs, path, &st);
	} else {
		ret = nfs_stat64(nfs, path, &st);
	}
	(*env)->ReleaseStringUTFChars(env, jpath, path);

	if (ret != 0) {
		throw_nfs(env, nfs, ret, "stat failed");
		return NULL;
	}
	return stat_to_java(env, &st);
}

JNIEXPORT jlongArray JNICALL
Java_org_libnfs4j_NfsClient_nativeFstat0(JNIEnv *env, jclass clazz,
                                         jlong jctx, jlong jfh)
{
	struct nfs_context *nfs = to_nfs(jctx);
	struct nfs_stat_64 st;
	int ret = nfs_fstat64(nfs, to_fh(jfh), &st);

	if (ret != 0) {
		throw_nfs(env, nfs, ret, "fstat failed");
		return NULL;
	}
	return stat_to_java(env, &st);
}

JNIEXPORT jlong JNICALL
Java_org_libnfs4j_NfsClient_nativeOpen(JNIEnv *env, jclass clazz, jlong jctx,
                                       jstring jpath, jint jflags)
{
	struct nfs_context *nfs = to_nfs(jctx);
	struct nfsfh *fh = NULL;
	const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
	int ret;

	ret = nfs_open(nfs, path, (int)jflags, &fh);
	(*env)->ReleaseStringUTFChars(env, jpath, path);

	if (ret != 0) {
		throw_nfs(env, nfs, ret, "open failed");
		return 0;
	}
	return (jlong)(intptr_t)fh;
}

JNIEXPORT void JNICALL
Java_org_libnfs4j_NfsClient_nativeClose0(JNIEnv *env, jclass clazz,
                                         jlong jctx, jlong jfh)
{
	struct nfs_context *nfs = to_nfs(jctx);
	if (nfs != NULL && jfh != 0) {
		nfs_close(nfs, to_fh(jfh));
	}
}

JNIEXPORT jbyteArray JNICALL
Java_org_libnfs4j_NfsClient_nativeRead(JNIEnv *env, jclass clazz, jlong jctx,
                                       jlong jfh, jint jcount)
{
	struct nfs_context *nfs = to_nfs(jctx);
	jbyteArray arr;
	char *buf;
	int ret;

	buf = malloc((size_t)jcount);
	if (buf == NULL) {
		throw_nfs(env, NULL, -12 /* ENOMEM */, "read failed");
		return NULL;
	}
	ret = nfs_read(nfs, to_fh(jfh), buf, (size_t)jcount);
	if (ret < 0) {
		free(buf);
		throw_nfs(env, nfs, ret, "read failed");
		return NULL;
	}
	arr = (*env)->NewByteArray(env, ret);
	if (arr != NULL && ret > 0) {
		(*env)->SetByteArrayRegion(env, arr, 0, ret, (jbyte *)buf);
	}
	free(buf);
	return arr;
}

JNIEXPORT jbyteArray JNICALL
Java_org_libnfs4j_NfsClient_nativePread0(JNIEnv *env, jclass clazz, jlong jctx,
                                         jlong jfh, jlong joffset,
                                         jint jcount)
{
	struct nfs_context *nfs = to_nfs(jctx);
	jbyteArray arr;
	char *buf;
	int ret;

	buf = malloc((size_t)jcount);
	if (buf == NULL) {
		throw_nfs(env, NULL, -12 /* ENOMEM */, "pread failed");
		return NULL;
	}
	ret = nfs_pread(nfs, to_fh(jfh), buf, (size_t)jcount,
	                (uint64_t)joffset);
	if (ret < 0) {
		free(buf);
		throw_nfs(env, nfs, ret, "pread failed");
		return NULL;
	}
	arr = (*env)->NewByteArray(env, ret);
	if (arr != NULL && ret > 0) {
		(*env)->SetByteArrayRegion(env, arr, 0, ret, (jbyte *)buf);
	}
	free(buf);
	return arr;
}

JNIEXPORT jint JNICALL
Java_org_libnfs4j_NfsClient_nativePwrite0(JNIEnv *env, jclass clazz,
                                          jlong jctx, jlong jfh, jlong joffset,
                                          jbyteArray jdata, jint joff,
                                          jint jlen)
{
	struct nfs_context *nfs = to_nfs(jctx);
	char *buf;
	int ret;

	if (jlen == 0) {
		return 0;
	}
	buf = malloc((size_t)jlen);
	if (buf == NULL) {
		throw_nfs(env, NULL, -12 /* ENOMEM */, "pwrite failed");
		return -1;
	}
	(*env)->GetByteArrayRegion(env, jdata, joff, jlen, (jbyte *)buf);
	ret = nfs_pwrite(nfs, to_fh(jfh), buf, (size_t)jlen, (uint64_t)joffset);
	free(buf);

	if (ret < 0) {
		throw_nfs(env, nfs, ret, "pwrite failed");
		return -1;
	}
	return ret;
}

JNIEXPORT jobjectArray JNICALL
Java_org_libnfs4j_NfsClient_nativeListDir(JNIEnv *env, jclass clazz,
                                          jlong jctx, jstring jpath)
{
	struct nfs_context *nfs = to_nfs(jctx);
	struct nfsdir *dir = NULL;
	struct nfsdirent *ent;
	jobjectArray result;
	jclass string_cls;
	const char *path;
	jstring *names = NULL;
	size_t count = 0, capacity = 0, i;
	int ret;

	path = (*env)->GetStringUTFChars(env, jpath, NULL);
	ret = nfs_opendir(nfs, path, &dir);
	(*env)->ReleaseStringUTFChars(env, jpath, path);
	if (ret != 0) {
		throw_nfs(env, nfs, ret, "opendir failed");
		return NULL;
	}

	/* Collect names; jstring creation can fail OOM, so build a plain
	   array of local refs first. */
	while ((ent = nfs_readdir(nfs, dir)) != NULL) {
		if (count == capacity) {
			size_t new_cap = capacity ? capacity * 2 : 32;
			jstring *tmp = realloc(names, new_cap * sizeof(*tmp));
			if (tmp == NULL) {
				nfs_closedir(nfs, dir);
				for (i = 0; i < count; i++) {
					(*env)->DeleteLocalRef(env, names[i]);
				}
				free(names);
				throw_nfs(env, NULL, -12 /* ENOMEM */,
				          "listDir failed");
				return NULL;
			}
			names = tmp;
			capacity = new_cap;
		}
		names[count] = (*env)->NewStringUTF(env, ent->name);
		if (names[count] == NULL) {
			nfs_closedir(nfs, dir);
			for (i = 0; i < count; i++) {
				(*env)->DeleteLocalRef(env, names[i]);
			}
			free(names);
			return NULL;
		}
		count++;
	}
	nfs_closedir(nfs, dir);

	string_cls = (*env)->FindClass(env, "java/lang/String");
	result = (*env)->NewObjectArray(env, (jsize)count, string_cls, NULL);
	if (result != NULL) {
		for (i = 0; i < count; i++) {
			(*env)->SetObjectArrayElement(env, result, (jsize)i,
			                              names[i]);
			(*env)->DeleteLocalRef(env, names[i]);
		}
	} else {
		for (i = 0; i < count; i++) {
			(*env)->DeleteLocalRef(env, names[i]);
		}
	}
	free(names);
	return result;
}

JNIEXPORT void JNICALL
Java_org_libnfs4j_NfsClient_nativeMkdir(JNIEnv *env, jclass clazz, jlong jctx,
                                        jstring jpath)
{
	struct nfs_context *nfs = to_nfs(jctx);
	const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
	int ret = nfs_mkdir(nfs, path);
	(*env)->ReleaseStringUTFChars(env, jpath, path);
	if (ret != 0) {
		throw_nfs(env, nfs, ret, "mkdir failed");
	}
}

JNIEXPORT void JNICALL
Java_org_libnfs4j_NfsClient_nativeMkdir2(JNIEnv *env, jclass clazz,
                                         jlong jctx, jstring jpath, jint mode)
{
	struct nfs_context *nfs = to_nfs(jctx);
	const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
	int ret = nfs_mkdir2(nfs, path, (int)mode);
	(*env)->ReleaseStringUTFChars(env, jpath, path);
	if (ret != 0) {
		throw_nfs(env, nfs, ret, "mkdir failed");
	}
}

JNIEXPORT void JNICALL
Java_org_libnfs4j_NfsClient_nativeRmdir(JNIEnv *env, jclass clazz, jlong jctx,
                                        jstring jpath)
{
	struct nfs_context *nfs = to_nfs(jctx);
	const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
	int ret = nfs_rmdir(nfs, path);
	(*env)->ReleaseStringUTFChars(env, jpath, path);
	if (ret != 0) {
		throw_nfs(env, nfs, ret, "rmdir failed");
	}
}

JNIEXPORT void JNICALL
Java_org_libnfs4j_NfsClient_nativeUnlink(JNIEnv *env, jclass clazz, jlong jctx,
                                         jstring jpath)
{
	struct nfs_context *nfs = to_nfs(jctx);
	const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
	int ret = nfs_unlink(nfs, path);
	(*env)->ReleaseStringUTFChars(env, jpath, path);
	if (ret != 0) {
		throw_nfs(env, nfs, ret, "unlink failed");
	}
}

JNIEXPORT void JNICALL
Java_org_libnfs4j_NfsClient_nativeRename(JNIEnv *env, jclass clazz, jlong jctx,
                                         jstring joldpath, jstring jnewpath)
{
	struct nfs_context *nfs = to_nfs(jctx);
	const char *oldpath = (*env)->GetStringUTFChars(env, joldpath, NULL);
	const char *newpath = (*env)->GetStringUTFChars(env, jnewpath, NULL);
	int ret = nfs_rename(nfs, oldpath, newpath);
	(*env)->ReleaseStringUTFChars(env, joldpath, oldpath);
	(*env)->ReleaseStringUTFChars(env, jnewpath, newpath);
	if (ret != 0) {
		throw_nfs(env, nfs, ret, "rename failed");
	}
}
