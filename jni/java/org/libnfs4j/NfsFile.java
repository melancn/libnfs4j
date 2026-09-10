package org.libnfs4j;

/**
 * An open file on an NFS export. Instances are created by
 * {@link NfsClient#open} and must be closed with {@link #close()};
 * closing the owning {@link NfsClient} invalidates all its open files.
 */
public final class NfsFile implements AutoCloseable {

    private final NfsClient client;
    private long fh; // opaque struct nfsfh *

    NfsFile(NfsClient client, long fh) {
        this.client = client;
        this.fh = fh;
    }

    // package-private: also used by NfsClient for file-scoped operations
    long checkedHandle() {
        if (fh == 0) {
            throw new IllegalStateException("NfsFile is closed");
        }
        return fh;
    }

    /** Read up to {@code count} bytes starting at {@code offset} (POSIX pread). */
    public byte[] pread(long offset, int count) throws NfsException {
        return client.nativePread(checkedHandle(), offset, count);
    }

    /** Write all of {@code data} at {@code offset} (POSIX pwrite). Returns bytes written. */
    public int pwrite(long offset, byte[] data) throws NfsException {
        return pwrite(offset, data, 0, data.length);
    }

    /** Write {@code data[off..off+len)} at {@code offset}. Returns bytes written. */
    public int pwrite(long offset, byte[] data, int off, int len) throws NfsException {
        return client.nativePwrite(checkedHandle(), offset, data, off, len);
    }

    /** Metadata for this open file handle. */
    public NfsStat stat() throws NfsException {
        return new NfsStat(client.nativeFstat(checkedHandle()));
    }

    @Override
    public void close() {
        if (fh != 0) {
            client.nativeClose(fh);
            fh = 0;
        }
    }
}
