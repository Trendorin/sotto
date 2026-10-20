/* Only one uid exists inside the rootless build root, so ownership changes made by
 * dpkg can't succeed. Pretend they did. Used for the apt step only. */
int chown(const char *path, unsigned uid, unsigned gid) { (void)path; (void)uid; (void)gid; return 0; }
int lchown(const char *path, unsigned uid, unsigned gid) { (void)path; (void)uid; (void)gid; return 0; }
int fchown(int fd, unsigned uid, unsigned gid) { (void)fd; (void)uid; (void)gid; return 0; }
int fchownat(int dir, const char *path, unsigned uid, unsigned gid, int flags) { (void)dir; (void)path; (void)uid; (void)gid; (void)flags; return 0; }
