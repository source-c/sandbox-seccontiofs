#ifndef _SECCONTIOFS_H_
#define _SECCONTIOFS_H_

#include <linux/dcache.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/seq_file.h>
#include <linux/statfs.h>
#include <linux/fs_stack.h>
#include <linux/magic.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/xattr.h>
#include <linux/exportfs.h>

#ifndef SECCONTIOFS_SUPER_MAGIC
#define SECCONTIOFS_SUPER_MAGIC   0x55FFABBA
#endif

extern int vfs_path_lookup(struct dentry *, struct vfsmount *,
	       const char *, unsigned int, struct path *);

/* the file system name */
#define SECCONTIOFS_NAME "seccontiofs"

/* seccontiofs root inode number */
#define SECCONTIOFS_ROOT_INO     1

/* useful for tracking code reachability */
#define TRACE_DBG printk(KERN_DEFAULT "-> DBG:%s:%s:%d\n", __FILE__, __func__, __LINE__)

/* some basic definitions */

#define PRIV_LBL "P1"
#define UNPRIV_LBL "U1"
#define PRIV_CG_NAME "/lxc/cont-priv"
#define PRIV_CG_NAME_LEN 14

/* operations vectors defined in specific files */
extern const struct file_operations seccontiofs_main_fops;
extern const struct file_operations seccontiofs_dir_fops;
extern const struct inode_operations seccontiofs_main_iops;
extern const struct inode_operations seccontiofs_dir_iops;
extern const struct inode_operations seccontiofs_symlink_iops;
extern const struct super_operations seccontiofs_sops;
extern const struct dentry_operations seccontiofs_dops;
extern const struct address_space_operations seccontiofs_aops, seccontiofs_dummy_aops;
extern const struct vm_operations_struct seccontiofs_vm_ops;
extern const struct export_operations seccontiofs_export_ops;
extern const struct xattr_handler *seccontiofs_xattr_handlers[];

extern int seccontiofs_init_inode_cache(void);
extern void seccontiofs_destroy_inode_cache(void);
extern int seccontiofs_init_dentry_cache(void);
extern void seccontiofs_destroy_dentry_cache(void);
extern int new_dentry_private_data(struct dentry *dentry);
extern void free_dentry_private_data(struct dentry *dentry);
extern struct dentry *seccontiofs_lookup(struct inode *dir, struct dentry *dentry,
				    unsigned int flags);
extern struct inode *seccontiofs_iget(struct super_block *sb,
				 struct inode *lower_inode);
extern int seccontiofs_interpose(struct dentry *dentry, struct super_block *sb,
			    struct path *lower_path);

/* file private data */
struct seccontiofs_file_info {
	struct file *lower_file;
	const struct vm_operations_struct *lower_vm_ops;
    const char *lbl;
};

/* seccontiofs inode data in memory */
struct seccontiofs_inode_info {
	struct inode *lower_inode;
	struct inode vfs_inode;
};

/* seccontiofs dentry data in memory */
struct seccontiofs_dentry_info {
	spinlock_t lock;	/* protects lower_path */
	struct path lower_path;
};

/* seccontiofs super-block data in memory */
struct seccontiofs_sb_info {
	struct super_block *lower_sb;
};

/*
 * inode to private data
 *
 * Since we use containers and the struct inode is _inside_ the
 * seccontiofs_inode_info structure, seccontiofs_I will always (given a non-NULL
 * inode pointer), return a valid non-NULL pointer.
 */
static inline struct seccontiofs_inode_info *seccontiofs_I(const struct inode *inode)
{
	return container_of(inode, struct seccontiofs_inode_info, vfs_inode);
}

/* dentry to private data */
#define seccontiofs_D(dent) ((struct seccontiofs_dentry_info *)(dent)->d_fsdata)

/* superblock to private data */
#define seccontiofs_SB(super) ((struct seccontiofs_sb_info *)(super)->s_fs_info)

/* file to private Data */
#define seccontiofs_F(file) ((struct seccontiofs_file_info *)((file)->private_data))

/* file to lower file */
static inline struct file *seccontiofs_lower_file(const struct file *f)
{
	return seccontiofs_F(f)->lower_file;
}

static inline void seccontiofs_set_lower_file(struct file *f, struct file *val)
{
	seccontiofs_F(f)->lower_file = val;
}

/* inode to lower inode. */
static inline struct inode *seccontiofs_lower_inode(const struct inode *i)
{
	return seccontiofs_I(i)->lower_inode;
}

static inline void seccontiofs_set_lower_inode(struct inode *i, struct inode *val)
{
	seccontiofs_I(i)->lower_inode = val;
}

/* superblock to lower superblock */
static inline struct super_block *seccontiofs_lower_super(
	const struct super_block *sb)
{
	return seccontiofs_SB(sb)->lower_sb;
}

static inline void seccontiofs_set_lower_super(struct super_block *sb,
					  struct super_block *val)
{
	seccontiofs_SB(sb)->lower_sb = val;
}

/* path based (dentry/mnt) macros */
static inline void pathcpy(struct path *dst, const struct path *src)
{
	dst->dentry = src->dentry;
	dst->mnt = src->mnt;
}
/* Returns struct path.  Caller must path_put it. */
static inline void seccontiofs_get_lower_path(const struct dentry *dent,
					 struct path *lower_path)
{
	spin_lock(&seccontiofs_D(dent)->lock);
	pathcpy(lower_path, &seccontiofs_D(dent)->lower_path);
	path_get(lower_path);
	spin_unlock(&seccontiofs_D(dent)->lock);
	return;
}
static inline void seccontiofs_put_lower_path(const struct dentry *dent,
					 struct path *lower_path)
{
	path_put(lower_path);
	return;
}
static inline void seccontiofs_set_lower_path(const struct dentry *dent,
					 struct path *lower_path)
{
	spin_lock(&seccontiofs_D(dent)->lock);
	pathcpy(&seccontiofs_D(dent)->lower_path, lower_path);
	spin_unlock(&seccontiofs_D(dent)->lock);
	return;
}
static inline void seccontiofs_reset_lower_path(const struct dentry *dent)
{
	spin_lock(&seccontiofs_D(dent)->lock);
	seccontiofs_D(dent)->lower_path.dentry = NULL;
	seccontiofs_D(dent)->lower_path.mnt = NULL;
	spin_unlock(&seccontiofs_D(dent)->lock);
	return;
}
static inline void seccontiofs_put_reset_lower_path(const struct dentry *dent)
{
	struct path lower_path;
	spin_lock(&seccontiofs_D(dent)->lock);
	pathcpy(&lower_path, &seccontiofs_D(dent)->lower_path);
	seccontiofs_D(dent)->lower_path.dentry = NULL;
	seccontiofs_D(dent)->lower_path.mnt = NULL;
	spin_unlock(&seccontiofs_D(dent)->lock);
	path_put(&lower_path);
	return;
}

/* locking helpers */
static inline struct dentry *lock_parent(struct dentry *dentry)
{
	struct dentry *dir = dget_parent(dentry);
	inode_lock_nested(d_inode(dir), I_MUTEX_PARENT);
	return dir;
}

static inline void unlock_dir(struct dentry *dir)
{
	inode_unlock(d_inode(dir));
	dput(dir);
}
#endif	/* not _SECCONTIOFS_H_ */
