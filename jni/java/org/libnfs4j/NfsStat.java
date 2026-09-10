package org.libnfs4j;

/**
 * File metadata returned by {@link NfsClient#stat} / {@link NfsClient#lstat}
 * / {@link NfsFile#stat}. Mirrors the POSIX {@code struct stat}.
 * Timestamps are seconds since the Unix epoch.
 */
public final class NfsStat {

    public final long dev;
    public final long ino;
    public final long mode;
    public final long nlink;
    public final long uid;
    public final long gid;
    public final long rdev;
    public final long size;
    public final long blksize;
    public final long blocks;
    public final long atime;
    public final long mtime;
    public final long ctime;

    /** The native side returns the fields in this exact order. */
    NfsStat(long[] v) {
        if (v == null || v.length != 13) {
            throw new IllegalArgumentException("invalid native stat buffer");
        }
        dev = v[0];
        ino = v[1];
        mode = v[2];
        nlink = v[3];
        uid = v[4];
        gid = v[5];
        rdev = v[6];
        size = v[7];
        blksize = v[8];
        blocks = v[9];
        atime = v[10];
        mtime = v[11];
        ctime = v[12];
    }

    @Override
    public String toString() {
        return "NfsStat{" +
                "mode=" + Long.toOctalString(mode) +
                ", uid=" + uid +
                ", gid=" + gid +
                ", size=" + size +
                ", mtime=" + mtime +
                '}';
    }
}
