#ifndef	_SYS_STAT_H
#define	_SYS_STAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <_ansi.h>
#include <time.h>
#include <sys/types.h>

/* dj's stat defines _STAT_H_ */
#ifndef _STAT_H_

/* It is intended that the layout of this structure not change when the
   sizes of any of the basic types change (short, int, long) [via a compile
   time option].  */

struct stat
{
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off64_t st_size;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
};

#define stat64 stat

/* mode_t flags */
#define S_IFMT             0170000 /* bitmask for the file type bitfields */
#define S_IFSOCK           0140000 /* socket */
#define S_IFLNK            0120000 /* symbolic link */
#define S_IFREG            0100000 /* regular file */
#define S_IFBLK            0060000 /* block device */
#define S_IFDIR            0040000 /* directory */
#define S_IFCHR            0020000 /* character device */
#define S_IFIFO            0010000 /* FIFO */
#define S_ISUID            0004000 /* set UID bit */
#define S_ISGID            0002000 /* set-group-ID bit */
#define S_ISVTX            0001000 /* sticky bit (see below) */
#define S_IRWXU            00700   /* mask for file owner permissions */
#define S_IRUSR            00400   /* owner has read permission */
#define S_IWUSR            00200   /* owner has write permission */
#define S_IXUSR            00100   /* owner has execute permission */
#define S_IRWXG            00070   /* mask for group permissions */
#define S_IRGRP            00040   /* group has read permission */
#define S_IWGRP            00020   /* group has write permission */
#define S_IXGRP            00010   /* group has execute permission */
#define S_IRWXO            00007   /* mask for permissions for others (not in group) */
#define S_IROTH            00004   /* others have read permission */
#define S_IWOTH            00002   /* others have write permission */
#define S_IXOTH            00001   /* others have execute permission */

/* Mode macros */
#define S_ISREG(m)         (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)         (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)         (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)         (((m) & S_IFMT) == S_IFBLK)
#define S_ISLNK(m)         (((m) & S_IFMT) == S_IFLNK)
#define S_ISFIFO(m)        (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)        (((m) & S_IFMT) == S_IFSOCK)

int	_EXFUN(chmod,( const char *__path, mode_t __mode ));
int     _EXFUN(fchmod,(int __fd, mode_t __mode));
int	_EXFUN(fstat,( int __fd, struct stat *__sbuf ));
int	_EXFUN(mkdir,( const char *_path, mode_t __mode ));
int	_EXFUN(mkfifo,( const char *__path, mode_t __mode ));
int	_EXFUN(stat,( const char *__path, struct stat *__sbuf ));
mode_t	_EXFUN(umask,( mode_t __mask ));
int	_EXFUN(lstat,( const char *__path, struct stat *__buf ));
int	_EXFUN(mknod,( const char *__path, mode_t __mode, dev_t __dev ));

/* Provide prototypes for most of the _<systemcall> names that are
   provided in newlib for some compilers.  */
#ifdef _COMPILING_NEWLIB
int	_EXFUN(_fstat,( int __fd, struct stat *__sbuf ));
int	_EXFUN(_stat,( const char *__path, struct stat *__sbuf ));
int	_EXFUN(_fstat64,( int __fd, struct stat64 *__sbuf ));
#endif

#endif /* !_STAT_H_ */
#ifdef __cplusplus
}
#endif
#endif /* _SYS_STAT_H */
