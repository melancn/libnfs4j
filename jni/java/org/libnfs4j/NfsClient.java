package org.libnfs4j;

/**
 * Synchronous client for a single NFS server export, backed by libnfs.
 *
 * <pre>{@code
 * try (NfsClient nfs = new NfsClient()) {
 *     nfs.mount("192.168.1.10", "/export");
 *     for (String name : nfs.listDir("/")) { ... }
 *     try (NfsFile f = nfs.open("/hello.txt", NfsClient.O_RDONLY)) {
 *         byte[] data = f.pread(0, 4096);
 *     }
 * }
 * }</pre>
 *
 * The native library is loaded with {@code System.loadLibrary("nfsjni")},
 * which also transitively loads libnfs.
 */
public final class NfsClient implements AutoCloseable {

    static {
        System.loadLibrary("nfsjni");
    }

    /* Linux/Android open(2) flag values, passed straight through to the server. */
    public static final int O_RDONLY = 0x00000000;
    public static final int O_WRONLY = 0x00000001;
    public static final int O_RDWR = 0x00000002;
    public static final int O_CREAT = 0x00000040;
    public static final int O_EXCL = 0x00000080;
    public static final int O_TRUNC = 0x00000200;
    public static final int O_APPEND = 0x00000400;

    private long ctx; // opaque struct nfs_context *

    public NfsClient() throws NfsException {
        ctx = nativeInitContext();
    }

    private long checkedContext() {
        if (ctx == 0) {
            throw new IllegalStateException("NfsClient is closed");
        }
        return ctx;
    }

    /** Mount the export {@code export} of NFS {@code server} (host name or IP). */
    public void mount(String server, String export) throws NfsException {
        nativeMount(checkedContext(), server, export);
    }

    /** Unmount the export. Called automatically by {@link #close()}. */
    public void umount() throws NfsException {
        if (ctx != 0) {
            nativeUmount(ctx);
        }
    }

    /** Metadata for {@code path} on the exported filesystem. Follows symlinks. */
    public NfsStat stat(String path) throws NfsException {
        return new NfsStat(nativeStat(checkedContext(), path, false));
    }

    /** Like {@link #stat} but does not follow symlinks. */
    public NfsStat lstat(String path) throws NfsException {
        return new NfsStat(nativeStat(checkedContext(), path, true));
    }

    /** Open {@code path} with {@code O_*} flags. */
    public NfsFile open(String path, int flags) throws NfsException {
        long fh = nativeOpen(checkedContext(), path, flags);
        return new NfsFile(this, fh);
    }

    /** Perform one sequential read of up to {@code count} bytes at the open offset. */
    public byte[] read(NfsFile f, int count) throws NfsException {
        return nativeRead(checkedContext(), f.checkedHandle(), count);
    }

    /** Names of the entries in directory {@code path}. */
    public String[] listDir(String path) throws NfsException {
        return nativeListDir(checkedContext(), path);
    }

    public void mkdir(String path) throws NfsException {
        nativeMkdir(checkedContext(), path);
    }

    public void mkdir(String path, int mode) throws NfsException {
        nativeMkdir2(checkedContext(), path, mode);
    }

    public void rmdir(String path) throws NfsException {
        nativeRmdir(checkedContext(), path);
    }

    public void unlink(String path) throws NfsException {
        nativeUnlink(checkedContext(), path);
    }

    public void rename(String oldPath, String newPath) throws NfsException {
        nativeRename(checkedContext(), oldPath, newPath);
    }

    /** Unmounts (if mounted) and releases the native context. Idempotent. */
    @Override
    public void close() {
        if (ctx != 0) {
            nativeDestroyContext(ctx);
            ctx = 0;
        }
    }

    /* The following are package-private bridges used by NfsFile. */
    byte[] nativePread(long fh, long offset, int count) throws NfsException {
        return nativePread0(ctx, fh, offset, count);
    }

    int nativePwrite(long fh, long offset, byte[] data, int off, int len) throws NfsException {
        return nativePwrite0(ctx, fh, offset, data, off, len);
    }

    long[] nativeFstat(long fh) throws NfsException {
        return nativeFstat0(ctx, fh);
    }

    void nativeClose(long fh) {
        nativeClose0(ctx, fh);
    }

    private static native long nativeInitContext() throws NfsException;
    private static native void nativeDestroyContext(long ctx);
    private static native void nativeMount(long ctx, String server, String export) throws NfsException;
    private static native void nativeUmount(long ctx) throws NfsException;
    private static native long[] nativeStat(long ctx, String path, boolean lstat) throws NfsException;
    private static native long nativeOpen(long ctx, String path, int flags) throws NfsException;
    private static native byte[] nativeRead(long ctx, long fh, int count) throws NfsException;
    private static native byte[] nativePread0(long ctx, long fh, long offset, int count) throws NfsException;
    private static native int nativePwrite0(long ctx, long fh, long offset, byte[] data, int off, int len) throws NfsException;
    private static native long[] nativeFstat0(long ctx, long fh) throws NfsException;
    private static native void nativeClose0(long ctx, long fh);
    private static native String[] nativeListDir(long ctx, String path) throws NfsException;
    private static native void nativeMkdir(long ctx, String path) throws NfsException;
    private static native void nativeMkdir2(long ctx, String path, int mode) throws NfsException;
    private static native void nativeRmdir(long ctx, String path) throws NfsException;
    private static native void nativeUnlink(long ctx, String path) throws NfsException;
    private static native void nativeRename(long ctx, String oldPath, String newPath) throws NfsException;
}
