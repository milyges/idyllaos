/*
 * Idylla Operating System
 * Copyright (C) 2009-2010 Idylla Operating System Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/
#ifndef __MODULES_FS_EXT2_H
#define __MODULES_FS_EXT2_H

#include <kernel/types.h>
#include <kernel/vfs.h>

// s_errors values
#define EXT2_ERRORS_CONTINUE          1
#define EXT2_ERRORS_RO                2
#define EXT2_ERRORS_PANIC             3
#define EXT2_ERRORS_DEFAULT           EXT2_ERRORS_CONTINUE

//  s_rev_level values
#define EXT2_GOOD_OLD_REV             0
#define EXT2_DYNAMIC_REV              1

#define EXT2_BAD_INO                  0x01 /* bad blocks inode */
#define EXT2_ROOT_INO                 0x02 /* root directory inode */
#define EXT2_ACL_IDX_INO              0x03 /* ACL index inode (deprecated?) */
#define EXT2_ACL_DATA_INO             0x04 /* ACL data inode (deprecated?) */
#define EXT2_BOOT_LOADER_INO          0x05 /* boot loader inode */
#define EXT2_UNDEL_DIR_INO            0x06 /* undelete directory inode */

#define EXT2_FILE_NAME                255

#define EXT2_FT_UNKNOWN               0
#define EXT2_FT_REG_FILE              1
#define EXT2_FT_DIR                   2
#define EXT2_FT_CHRDEV                3
#define EXT2_FT_BLKDEV                4
#define EXT2_FT_FIFO                  5
#define EXT2_FT_SOCK                  6
#define EXT2_FT_SYMLINK               7
#define EXT2_FT_MAX                   8

#define EXT2_MAGIC                    0xEF53

#define EXT2_VALID_FS                 0x0001
#define EXT2_ERROR_FS                 0x0002

#define EXT2_SUPERBLOCK_SECTOR        2

#define EXT2_BLOCK_SIZE(sb)           (1024 << ((struct ext2_superblock *)sb)->s_log_block_size)
#define EXT2_GET_SECTOR_NUM(d,b)      ((b) * ((struct ext2_data *)d)->spb)
#define EXT2_INODE_BLOCKS(d,b)        (((b) * 512) / ((struct ext2_data *)d)->blk_size)
#define EXT2_GET_INODE_GROUP(d,i)     (((i) - 1) / ((struct ext2_data *)d)->sb.s_inodes_per_group)
#define EXT2_GET_BLOCK_GROUP(d,b)     (((b) - 1) / ((struct ext2_data *)d)->sb.s_blocks_per_group)

#define EXT2_SB_LOCK(d)               mutex_lock(&((struct ext2_data *)d)->sb_mutex)
#define EXT2_SP_UNLOCK(d)             mutex_unlock(&((struct ext2_data *)d)->sb_mutex)
#define EXT2_GROUP_LOCK(d,g)          mutex_lock(&((struct ext2_data *)d)->groups_mutex[(g)])
#define EXT2_GROUP_UNLOCK(d,g)        mutex_unlock(&((struct ext2_data *)d)->groups_mutex[(g)])

struct ext2_superblock                 // offset  size
{
	uint32_t s_inodes_count;       //   0      4
	uint32_t s_blocks_count;       //   4      4
	uint32_t s_r_blocks_count;     //   8      4
	uint32_t s_free_blocks_count;  //   12     4
	uint32_t s_free_inodes_count;  //   16     4
	uint32_t s_first_data_block;   //   20     4
	uint32_t s_log_block_size;     //   24     4
	uint32_t s_log_frag_size;      //   28     4
	uint32_t s_blocks_per_group;   //   32     4
	uint32_t s_frags_per_group;    //   36     4
	uint32_t s_inodes_per_group;   //   40     4
	uint32_t s_mtime;              //   44     4
	uint32_t s_wtime;              //   48     4
	uint16_t s_mnt_count;          //   52     2
	uint16_t s_max_mnt_count;      //   54     2
	uint16_t s_magic;              //   56     2
	uint16_t s_state;              //   58     2
	uint16_t s_errors;             //   60     2
	uint16_t s_minor_rev_level;    //   62     2
	uint32_t s_lastcheck;          //   64     4
	uint32_t s_checkinterval;      //   68     4
	uint32_t s_creator_os;         //   72     4
	uint32_t s_rev_level;          //   76     4
	uint16_t s_def_resuid;         //   80     2
	uint16_t s_def_resgid;         //   82     2
	// -- EXT2_DYNAMIC_REV Specific --
	uint32_t s_first_ino;          //   84     4
	uint16_t s_inode_size;         //   88     2
	uint16_t s_block_group_nr;     //   90     2
	uint32_t s_feature_compat;     //   92     4
	uint32_t s_feature_incompat;   //   96     4
	uint32_t s_feature_ro_compat;  //   100    4
	uint8_t  s_uuid[16];           //   104    16
	uint8_t  s_volume_name[16];    //   120    16
	uint8_t  s_last_mounted[64];   //   136    64
	uint32_t s_algo_bitmap;        //   200    4
	// -- Performance Hints --
	uint8_t  s_prealloc_blocks;    //   204    1
	uint8_t  s_prealloc_dir_blocks;//   205    1
	uint16_t reserved1;            //   206    2
	// -- Journaling Support --
	uint8_t  s_journal_uuid[16];   //   208    16
	uint32_t s_journal_inum;       //   224    4
	uint32_t s_journal_dev;        //   228    4
	uint32_t s_last_orphan;        //   232    4
	// -- Unused --
	uint8_t  reserved2[788];       //   236    788
} PACKED;

struct ext2_group_desc                 // offset  size
{
	uint32_t bg_block_bitmap;      //   0      4
	uint32_t bg_inode_bitmap;      //   4      4
	uint32_t bg_inode_table;       //   8      4
	uint16_t bg_free_blocks_count; //   12     2
	uint16_t bg_free_inodes_count; //   14     2
	uint16_t bg_used_dirs_count;   //   16     2
	uint16_t bg_pad;               //   18     2
	uint8_t  bg_reserved[12];      //   20     12
} PACKED;

struct ext2_inode                      // offset  size
{
	uint16_t i_mode;               //   0      2
	uint16_t i_uid;                //   2      2
	uint32_t i_size;               //   4      4
	uint32_t i_atime;              //   8      4
	uint32_t i_ctime;              //   12     4
	uint32_t i_mtime;              //   16     4
	uint32_t i_dtime;              //   20     4
	uint16_t i_gid;                //   24     2
	uint16_t i_links_count;        //   26     2
	uint32_t i_blocks;             //   28     4
	uint32_t i_flags;              //   32     4
	uint32_t i_osd1;               //   36     4
	uint32_t i_block[15];          //   40     4x15
	uint32_t i_generation;         //   100    4
	uint32_t i_file_acl;           //   104    4
	uint32_t i_dir_acl;            //   108    4
	uint32_t i_faddr;              //   112    4
	uint8_t  i_osd2[12];           //   116    12
} PACKED;

struct ext2_dir_entry                  // offset  size
{
	uint32_t inode;                //   0      4
	uint16_t rec_len;              //   4      2
	uint8_t  name_len;             //   6      1
	uint8_t  file_type;            //   7      1
	uint8_t  name[EXT2_FILE_NAME]; //   8      255
} PACKED;

struct ext2_data
{
	struct ext2_superblock sb;   /* Super block */
	struct mutex sb_mutex;       /* Mutex dla superblocka */
	uint16_t spb;                /* Sectors Per Block */
	uint16_t bps;                /* Bytes Per Sector */
	uint16_t blk_size;           /* Rozmiar bloku w bajtach */
	int groups_count;            /* Ilość grup */
	int groups_blocks;           /* Ilość bloków z informacjami o grupach */
	struct ext2_group_desc * groups;
	struct mutex * groups_mutex; /* Blogady poszczegolnych grup */
	struct mountpoint * mp;      /* Wskaznik na punkt montowania */
};

/* Z pliku super.c */
int super_read(struct ext2_data * data);
int super_write(struct ext2_data * data);
int super_read_groups(struct ext2_data * data);
int super_write_groups(struct ext2_data * data);

/* Z pliku inode.c */
int inode_do_read(struct ext2_data * data, struct ext2_inode * inode, ino_t num);
int inode_do_write(struct ext2_data * data, struct ext2_inode * inode, ino_t num);
int inode_read(struct vnode * vnode);
int inode_write(struct vnode * vnode);
void inode_free(struct vnode * vnode);
int inode_link(struct vnode * parent, char * name, ino_t ino_num, int type);
int inode_unlink(struct vnode * parent, char * name);
int32_t inode_read_content(struct vnode * vnode, void * buf, uint32_t blocks, uint32_t start);
int32_t inode_write_content(struct vnode * vnode, void * buf, uint32_t blocks, uint32_t start);

int inode_alloc(struct ext2_data * data, ino_t * ino, int group);

#endif /* __MODULES_FS_EXT2_H */
