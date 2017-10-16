#include "seccontiofs.h"
#include <linux/module.h>

/*
 * There is no need to lock the seccontiofs_super_info's rwsem as there is no
 * way anyone can have a reference to the superblock at this point in time.
 */
static int seccontiofs_read_super(struct super_block *sb, void *raw_data, int silent)
{
	int err = 0;
	struct super_block *lower_sb;
	struct path lower_path;
	char *dev_name = (char *) raw_data;
	struct inode *inode;

	if (!dev_name) {
		printk(KERN_ERR
		       "seccontiofs: read_super: missing dev_name argument\n");
		err = -EINVAL;
		goto out;
	}

	/* parse lower path */
	err = kern_path(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY,
			&lower_path);
	if (err) {
		printk(KERN_ERR	"seccontiofs: error accessing "
		       "lower directory '%s'\n", dev_name);
		goto out;
	}

	/* allocate superblock private data */
	sb->s_fs_info = kzalloc(sizeof(struct seccontiofs_sb_info), GFP_KERNEL);
	if (!seccontiofs_SB(sb)) {
		printk(KERN_CRIT "seccontiofs: read_super: out of memory\n");
		err = -ENOMEM;
		goto out_free;
	}

	/* set the lower superblock field of upper superblock */
	lower_sb = lower_path.dentry->d_sb;
	atomic_inc(&lower_sb->s_active);
	seccontiofs_set_lower_super(sb, lower_sb);

	/* inherit maxbytes from lower file system */
	sb->s_maxbytes = lower_sb->s_maxbytes;

	/*
	 * Our c/m/atime granularity is 1 ns because we may stack on file
	 * systems whose granularity is as good.
	 */
	sb->s_time_gran = 1;

	sb->s_op = &seccontiofs_sops;
	sb->s_xattr = seccontiofs_xattr_handlers;

	sb->s_export_op = &seccontiofs_export_ops; /* adding NFS support */

	/* get a new inode and allocate our root dentry */
	inode = seccontiofs_iget(sb, d_inode(lower_path.dentry));
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_sput;
	}
	sb->s_root = d_make_root(inode);
	if (!sb->s_root) {
		err = -ENOMEM;
		goto out_iput;
	}
	d_set_d_op(sb->s_root, &seccontiofs_dops);

	/* link the upper and lower dentries */
	sb->s_root->d_fsdata = NULL;
	err = new_dentry_private_data(sb->s_root);
	if (err)
		goto out_freeroot;

	/* if get here: cannot have error */

	/* set the lower dentries for s_root */
	seccontiofs_set_lower_path(sb->s_root, &lower_path);

	/*
	 * No need to call interpose because we already have a positive
	 * dentry, which was instantiated by d_make_root.  Just need to
	 * d_rehash it.
	 */
	d_rehash(sb->s_root);
	if (!silent)
		printk(KERN_INFO
		       "seccontiofs: mounted on top of %s type %s\n",
		       dev_name, lower_sb->s_type->name);

	seccontiofs_SB(sb)->__mode = WRITABLE_MODE;
	goto out; /* all is well */

	/* no longer needed: free_dentry_private_data(sb->s_root); */
out_freeroot:
	dput(sb->s_root);
out_iput:
	iput(inode);
out_sput:
	/* drop refs we took earlier */
	atomic_dec(&lower_sb->s_active);
	kfree(seccontiofs_SB(sb));
	sb->s_fs_info = NULL;
out_free:
	path_put(&lower_path);

out:
	return err;
}

struct dentry *seccontiofs_mount(struct file_system_type *fs_type, int flags,
			    const char *dev_name, void *raw_data)
{
	void *lower_path_name = (void *) dev_name;

	return mount_nodev(fs_type, flags, lower_path_name,
			   seccontiofs_read_super);
}

static struct file_system_type seccontiofs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= SECCONTIOFS_NAME,
	.mount		= seccontiofs_mount,
	.kill_sb	= generic_shutdown_super,
	.fs_flags	= 0,
};
MODULE_ALIAS_FS(SECCONTIOFS_NAME);

static int __init init_seccontiofs_fs(void)
{
	int err;

	pr_info("Registering seccontiofs " SECCONTIOFS_VERSION "\n");

	err = seccontiofs_init_inode_cache();
	if (err)
		goto out;
	err = seccontiofs_init_dentry_cache();
	if (err)
		goto out;
	err = register_filesystem(&seccontiofs_fs_type);
out:
	if (err) {
		seccontiofs_destroy_inode_cache();
		seccontiofs_destroy_dentry_cache();
	}
	return err;
}

static void __exit exit_seccontiofs_fs(void)
{
	seccontiofs_destroy_inode_cache();
	seccontiofs_destroy_dentry_cache();
	unregister_filesystem(&seccontiofs_fs_type);
	pr_info("Completed seccontiofs module unload\n");
}

MODULE_AUTHOR("AI");
MODULE_DESCRIPTION("seccontiofs " SECCONTIOFS_VERSION
		   " no-www-yet");
MODULE_LICENSE("GPL");

module_init(init_seccontiofs_fs);
module_exit(exit_seccontiofs_fs);
