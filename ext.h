/**********************************************
 * Disk Image Recovery Tool (DIRT)
 * -------------------------------------------
 * Created By        Marcus Kelly
 *    Contact        nexuslite@gmail.com
 * Created On        Feb 23, 2014
 * Updated On        Apr 03, 2014
 * -------------------------------------------
 * Copyright 2014 Marcus Kelly
 **********************************************/
#ifndef EXT
#define EXT

#include <fstream>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include "drive.h"

typedef struct ext_super_block {
	unsigned int s_inodes_count;
	unsigned int s_blocks_count_lo;
	unsigned int s_r_blocks_count_lo;
	unsigned int s_free_blocks_count_lo;
	unsigned int s_free_inodes_count;
	unsigned int s_first_data_block;
	unsigned int s_log_block_size;
	unsigned int s_log_cluster_size;
	unsigned int s_blocks_per_group;
	unsigned int s_obso_frags_per_group;
	unsigned int s_inodes_per_group;
	unsigned int s_mtime;
	unsigned int s_wtime;
	unsigned short int s_mnt_count;
	unsigned short int s_max_mnt_count;
	unsigned short int s_magic;
	unsigned short int s_state;
	unsigned short int s_errors;
	unsigned short int s_minor_rev_level;
	unsigned int s_lastcheck;
	unsigned int s_checkinterval;
	unsigned int s_creator_os;
	unsigned int s_rev_level;
	unsigned short int s_def_resuid;
	unsigned short int s_def_resgid;
	unsigned int s_first_ino;
	unsigned short int s_inode_size;
	unsigned short int s_block_group_nr;
	unsigned int s_feature_compat;
	unsigned int s_feature_incompat;
	unsigned int s_feature_ro_compat;
	unsigned char s_uuid[16];
	unsigned char s_volume_name[16];
	unsigned char s_last_mounted[64];
	unsigned int s_algorithm_usage_bitmap;
	unsigned char s_prealloc_blocks;
	unsigned char s_prealloc_dir_blocks;
	unsigned short int s_reserved_gdt_blocks;
	unsigned char s_journal_uuid[16];
	unsigned int s_journal_inum;
	unsigned int s_journal_dev;
	unsigned int s_last_orphan;
	unsigned int s_hash_seed[4];
	unsigned char s_def_hash_version;
	unsigned char s_jnl_backup_type;
	unsigned short int s_desc_size;
	unsigned int s_default_mount_opts;
	unsigned int s_first_meta_bg;
	unsigned int s_mkfs_time;
	unsigned int s_jnl_blocks[17];
	unsigned int s_blocks_count_hi;
	unsigned int s_r_blocks_count_hi;
	unsigned int s_free_blocks_count_hi;
	unsigned short int s_min_extra_isize;
	unsigned short int s_want_extra_isize;
	unsigned int s_flags;
	unsigned short int s_raid_stride;
	unsigned short int s_mmp_interval;
	unsigned long int s_mmp_block;
	unsigned int s_raid_stripe_width;
	unsigned char s_log_groups_per_flex;
	unsigned char s_checksum_type;
	unsigned short int s_reserved_pad;
	unsigned long int s_kbytes_written;
	unsigned int s_snapshot_inum;
	unsigned int s_snapshot_id;
	unsigned long int s_snapshot_r_blocks_count;
	unsigned int s_snapshot_list;
	unsigned int s_error_count;
	unsigned int s_first_error_time;
	unsigned int s_first_error_ino;
	unsigned long int s_first_error_block;
	unsigned char s_first_error_func[32];
	unsigned int s_first_error_line;
	unsigned int s_last_error_time;
	unsigned int s_last_error_ino;
	unsigned int s_last_error_line;
	unsigned long int s_last_error_block;
	unsigned char s_last_error_func[32];
	unsigned char s_mount_opts[64];
	unsigned int s_usr_quota_inum;
	unsigned int s_grp_quota_inum;
	unsigned int s_overhead_blocks;
	unsigned int s_reserved[108];
	unsigned int s_checksum;
} ext_super_block;

typedef struct ext_inode {
	unsigned short int i_mode;
	unsigned short int i_uid;
	unsigned int i_size_lo;
	unsigned int i_atime;
	unsigned int i_ctime;
	unsigned int i_mtime;
	unsigned int i_dtime;
	unsigned short int i_gid;
	unsigned short int i_links_count;
	unsigned int i_blocks_lo;
	unsigned int i_flags;
	unsigned char osd1[4];
	unsigned char i_block[60];
	unsigned int i_generation;
	unsigned int i_file_acl_lo;
	unsigned int i_size_high;
	unsigned int i_obso_faddr;
	unsigned char osd2[12];
	unsigned short int i_extra_isize;
	unsigned short int i_checksum_hi;
	unsigned int i_ctime_extra;
	unsigned int i_mtime_extra;
	unsigned int i_atime_extra;
	unsigned int i_crtime;
	unsigned int i_crtime_extra;
	unsigned int i_version_hi;
} ext_inode;

typedef struct ext_block_group { // half table
	unsigned int bg_block_bitmap_lo;
	unsigned int bg_inode_bitmap_lo;
	unsigned int bg_inode_table_lo;
	unsigned short int bg_free_blocks_count_lo;
	unsigned short int bg_free_inodes_count_lo;
	unsigned short int bg_used_dirs_count_lo;
	unsigned short int bg_flags;
	unsigned int bg_exclude_bitmap_lo;
	unsigned short int bg_block_bitmap_csum_lo;
	unsigned short int bg_inode_bitmap_csum_lo;
	unsigned short int bg_itable_unused_lo;
	unsigned short int bg_checksum;
} ext_block_group;


typedef struct ext_extent_header {
	unsigned short int eh_magic;
	unsigned short int eh_entries;
	unsigned short int eh_max;
	unsigned short int eh_depth;
	unsigned int eh_generation;
} ext_extent_header;


typedef struct ext_extent_idx {
	unsigned int ei_block;
	unsigned int ei_leaf_lo;
	unsigned short int ei_leaf_hi;
	unsigned short int ei_unused;
} ext_extent_idx;

typedef struct ext_extent_leaf {
	unsigned int ee_block;
	unsigned short int ee_len;
	unsigned short int ee_start_hi;
	unsigned int ee_start_lo;
} ext_extent_leaf;

typedef struct ext_dir_entry {
	unsigned int inode;
	unsigned short int rec_len;
	char name_len;
	char file_type;
	char name[256];
} ext_dir_entry;

class Ext {
	public:
		Ext(Drive* InDrive) {
			drive = InDrive;
		}
		~Ext() {}
		void ReadSuperBlock() {
		}
		ext_dir_entry GetFirstDirEntry() {
			DirectoryIndex = 0;
			DirectorySectorIndex = 0;
			TotalBytes = 0;
			return GetNextDirEntry();
		}
		ext_dir_entry GetNextDirEntry() {
			unsigned int rec_len;
			int adj = 0;
			char EntryData[264];
			//memcpy(&DirEntry, &data[DirectoryIndex], 4);
			//if (DirEntry.inode == 0) {
			//	return DirEntry;
			//}
			if (TotalBytes >= 4096-8) {
				DirEntry.inode = 0;
				DirEntry.rec_len = 0;
				DirEntry.name_len = 0;
				DirEntry.file_type = 0;
				DirEntry.name[0] = '\0';
				return DirEntry;
			}

			char * data = drive->Read(Directory+DirectorySectorIndex);
			if (sizeof(ext_dir_entry) < drive->GetBufferSize()-DirectoryIndex) {
				memcpy(&EntryData, &data[DirectoryIndex], sizeof(ext_dir_entry));
			}
			else {
				memcpy(&EntryData, &data[DirectoryIndex], drive->GetBufferSize()-DirectoryIndex);
			}
			memcpy(&DirEntry, &EntryData, 264);
			if (drive->GetBufferSize()-DirectoryIndex < 8) {
				adj = drive->GetBufferSize()-DirectoryIndex;
				DirectorySectorIndex++;
				data = drive->Read(Directory+DirectorySectorIndex);
				char* end = EntryData+adj;
				memcpy(end, &data[0], sizeof(EntryData)-adj);
				DirectoryIndex = 0;
			}
			memcpy(&DirEntry, &EntryData, 264);
			if ((unsigned int) drive->GetBufferSize()-DirectoryIndex-8 < DirEntry.name_len) {
				adj = drive->GetBufferSize()-DirectoryIndex;
				DirectorySectorIndex++;
				data = drive->Read(Directory+DirectorySectorIndex);
				char* end = EntryData+adj;
				memcpy(end, &data[0], sizeof(EntryData)-adj);
				DirectoryIndex = 0;
			}
			memcpy(&DirEntry, &EntryData, 264);

			DirEntry.name[DirEntry.name_len] = '\0';

			rec_len = DirEntry.rec_len;
			while (drive->GetBufferSize()-DirectoryIndex < rec_len) {
				DirectorySectorIndex++;
				data = drive->Read(Directory+DirectorySectorIndex);
				rec_len -= drive->GetBufferSize();
				TotalBytes += drive->GetBufferSize();
				DirectoryIndex = 0;
				adj = 0;
			}
			DirectoryIndex += rec_len-adj;
			TotalBytes += rec_len;
			return DirEntry;
		}
		void ShowDirectory() {
			ext_dir_entry thisDirEntry = GetFirstDirEntry();
			while (strlen(thisDirEntry.name) != 0) {
				if (thisDirEntry.file_type == 2) {
					cout << thisDirEntry.name << "/             " << thisDirEntry.inode << " " << (int) thisDirEntry.file_type << " " << (int) thisDirEntry.name_len << " " << thisDirEntry.rec_len << "\n";
				}
				else {
					cout << thisDirEntry.name << "              " << thisDirEntry.inode << " " << (int) thisDirEntry.file_type << " " << (int) thisDirEntry.name_len << " " << thisDirEntry.rec_len << "\n";
				}
				thisDirEntry = GetNextDirEntry();
			}
			cout << "Total Bytes: " << TotalBytes << "\n";
		}
		ext_dir_entry NameToInode (string Name) {
			int i;
			for (i = 0; i < Name.length(); i++) {
				if (Name[i] == '^') {
					Name[i] = ' ';
				}
			}
			ext_dir_entry thisDirEntry = GetFirstDirEntry();
			while (strlen(thisDirEntry.name) != 0) {
				if (strlen(thisDirEntry.name) != Name.length()) {
					thisDirEntry = GetNextDirEntry();
					continue;
				}
				for (i = 0; i < strlen(thisDirEntry.name); i++) {
					if (thisDirEntry.name[i] == Name[i]) {
						continue;
					}
					else {
						break;
					}
				}
				if (i == Name.length()) {
					break;
				}
				thisDirEntry = GetNextDirEntry();
			}
			return thisDirEntry;
		}
		void ChangeDirectory(string Name) {
			ext_dir_entry thisDirEntry = NameToInode(Name);
			if (thisDirEntry.inode != 0 && thisDirEntry.file_type == 2) {
				ext_inode DirInode = ReadInode(thisDirEntry.inode);
				ext_extent_header ExtentHeader;
				ext_extent_leaf ExtentLeaf;
				int ExtentIndex = 0;
				memcpy(&ExtentHeader, &DirInode.i_block[ExtentIndex], sizeof(ext_extent_header));
				ExtentIndex += sizeof(ext_extent_header);
				if (ExtentHeader.eh_magic == 0xF30A) {
					if (ExtentHeader.eh_depth == 0) {
						memcpy(&ExtentLeaf, &DirInode.i_block[ExtentIndex], sizeof(ext_extent_leaf));
						Directory = ExtentLeaf.ee_start_lo*8;
					}
					else {
						cout << "Error Changing Directory\n";
					}
				}
				else {
					cout << "Error Changing Directory\n";
				}
			}
		}
		void SetCurrentDirectory(unsigned int InDirectory) {
			Directory = InDirectory;
		}
		int GetCurrentDirectory() {
			return Directory;
		}
		void FileDump(string Name) {
			if (Name.compare("*") == 0) {
				ext_dir_entry thisDirEntry = GetFirstDirEntry();
				while (strlen(thisDirEntry.name) != 0) {
					cout << thisDirEntry.name << "\n";
					if (thisDirEntry.file_type == 1) {
						PullInode(thisDirEntry.inode, thisDirEntry.name);
					}
					thisDirEntry = GetNextDirEntry();
				}
			}
			else {
				ext_dir_entry thisDirEntry = NameToInode(Name);
				if (thisDirEntry.inode != 0 && thisDirEntry.file_type == 1) {
					PullInode(thisDirEntry.inode, Name);
				}
			}
		}
		void PullInode(unsigned int InInode, string filename) {
			ofstream file(filename.c_str(), ios_base::binary);
			if(!file)
				throw(runtime_error(strerror(errno)));
			char * data;
			//cout << InInode << "\n";
			ext_inode thisInode = ReadInode(InInode);
			ext_extent_header ExtentHeader;
			ext_extent_idx ExtentIDX;
			ext_extent_leaf ExtentLeaf;
			bool ininode = true;
			bool idx = false;
			char Extent[12];
			int index = 0;
			int extcount = 1;
			int remaning, left;
			unsigned int LastSector;
			unsigned int datablocks, datasector;
			int extsector;
			unsigned int filesize = thisInode.i_size_lo;
			unsigned int origfilesize = filesize;
			while (extcount > 0) {
				if (ininode == true) {
			 		memcpy(&Extent, &thisInode.i_block[index], 12);
					index += 12;
				}
				else {
					data = drive->Read(LastSector);
					remaning = drive->GetBufferSize()-index;
					if (remaning < 12) {
						memcpy(&Extent, &data[index], remaning);
						LastSector++;
						data = drive->Read(LastSector);
						left = 12-remaning;
						memcpy(&Extent[remaning], &data[0], left);
						index = left;
					}
					else {
						memcpy(&Extent, &data[index], 12);
						index += 12;
					}
				}
				if (idx == 0) {
			 		memcpy(&ExtentHeader, &Extent, 12);
				}
				if (ExtentHeader.eh_magic == 0xF30A) {
					cout << "  eh_magic: " << ExtentHeader.eh_magic << " eh_entries: " << ExtentHeader.eh_entries << " eh_max: " << ExtentHeader.eh_max << " eh_depth: " << ExtentHeader.eh_depth << "\n";
					extcount = ExtentHeader.eh_entries;
					datablocks = 0;
					ExtentHeader.eh_magic=0;
					if (ExtentHeader.eh_entries > 1 && ExtentHeader.eh_depth > 0) {
						cout << "Can't pull file multiple extent idx.";
						extcount = 0;
						break;
					}
					idx=1;
					continue;
				}
				else if (ExtentHeader.eh_depth == 1) { // extent idx
			 		memcpy(&ExtentIDX, &Extent, 12);
					index = 0;
					cout << "  ei_block: " << ExtentIDX.ei_block << " ei_leaf_lo: " << ExtentIDX.ei_leaf_lo << " ei_leaf_hi: " << ExtentIDX.ei_leaf_hi << " ei_unused: " << ExtentIDX.ei_unused << "\n";
					cout << "  Address: " << (ExtentIDX.ei_leaf_lo*8) << "\n";
					extsector = ExtentIDX.ei_leaf_lo*8;
					datablocks = 0;
					ininode = false;
					LastSector = extsector;
					data = drive->Read(extsector);
					idx=0;
					continue;
				}
				else if (ExtentHeader.eh_depth == 0) { // extent leaf
			 		memcpy(&ExtentLeaf, &Extent, 12);
					cout << "  ee_block: " << ExtentLeaf.ee_block << " ee_len: " << ExtentLeaf.ee_len << " ee_start_hi: " << ExtentLeaf.ee_start_hi << " ee_start_lo: " << ExtentLeaf.ee_start_lo << "\n";
					cout << "  Address: " << (ExtentLeaf.ee_start_lo*8) << "\n";
					datasector = ExtentLeaf.ee_start_lo*8;
					if (datasector == 0) {
						cout << "Bad Data Sector.\n";
						break;
					}
					datablocks = ExtentLeaf.ee_len*8;
					extcount--;
				}
				else {
					cout << "Nothing to do.\n";
					cout << "  eh_magic: " << ExtentHeader.eh_magic << " eh_entries: " << ExtentHeader.eh_entries << " eh_max: " << ExtentHeader.eh_max << " eh_depth: " << ExtentHeader.eh_depth << "\n";
					extcount = 0;
					datablocks = 0;
					continue;
				}

				while (datablocks > 0 && filesize > 0) {
					cout << "Remaining Size: " << filesize << "/" << origfilesize  << " DataBlocks: " << datablocks << " Leafs: " << extcount << "              \r" << std::flush;
					data = drive->Read(datasector);
					if (filesize < 512) {
						file.write(&data[0], filesize);
						if (!file) {
							cout << "Write Error\n";
						}
						filesize = 0;
					}
					else {
						file.write(&data[0], 512);
						if (!file) {
							cout << "Write Error\n";
						}
						filesize -= 512;
					}
					datablocks--;
					datasector++;
				}

			}
			if (filesize > 0 || datablocks > 7 || extcount > 0) {
				cout << "Pull failed to complete file extraction.\n";
				cout << " filesize: " << filesize << " datablocks: " << datablocks <<  " extcount: " << extcount << "\n";
			}
			file.close();
		}
		ext_inode ReadInode(unsigned int InInode) {

			int s_inodes_per_group = 8192;				// this should come from superblock
			int s_inode_size = 256;					// this should come from superblock
			// Get Group Descriptor
			int bg = (InInode - 1) / s_inodes_per_group;		// Which Group Number
			int bgsector = bg / 16 + 8;					// = bg sector
			int bgoffset = bg % 16 * 32;
			char * data = drive->Read(bgsector);
			memcpy(&BlockGroup, &data[bgoffset], sizeof(ext_block_group));

			int index = (InInode - 1) % s_inodes_per_group;		// Which Item in that group
			int offset = index * s_inode_size;				// Offset of Item into INODE Table
			unsigned int InodeTableSector = BlockGroup.bg_inode_table_lo * 8;		// sector of inode table from block group
			unsigned int InodeEntrySector = offset / 512 + InodeTableSector;	// sector of inode entry
			int EntryOffset = offset % 512;
			data = drive->Read(InodeEntrySector);
			memcpy(&Inode, &data[EntryOffset], sizeof(ext_inode));
			return Inode;
		}
	private:
		int TotalBytes;
		unsigned int Directory;
		unsigned int DirectoryIndex;
		unsigned int DirectorySectorIndex;
		Drive* drive;
		ext_inode Inode;
		ext_dir_entry DirEntry;
		ext_block_group BlockGroup;
		
};

#endif
