package org.libnfs4j;

import java.io.IOException;

/**
 * Exception thrown when a libnfs operation fails.
 *
 * {@link #getNfsCode()} returns the negative errno value reported by the
 * NFS server (e.g. -2 for ENOENT).
 */
public class NfsException extends IOException {

    private final int nfsCode;

    public NfsException(String message, int nfsCode) {
        super(message);
        this.nfsCode = nfsCode;
    }

    /** Negative errno value returned by libnfs/NFS server. */
    public int getNfsCode() {
        return nfsCode;
    }
}
