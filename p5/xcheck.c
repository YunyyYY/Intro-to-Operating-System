#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <string.h>

#define stat xv6_stat  // avoid clash with host struct stat
#define dirent xv6_dirent  // avoid clash with host struct
#include "types.h"
#include "fs.h"
#include "stat.h"
#undef stat
#undef dirent

#define SINODE sizeof(struct dinode)
#define NDIRENT BSIZE/sizeof(struct xv6_dirent)
#define BITADDR(base, sb, db) base + sb*BSIZE + db/8
#define BITOFF(data_block) data_block%8

// global data
uint start_data, start_bitmap;
int *record, *inode_used, *inode_ref;
struct dinode *dip;       	// point to inode in inode blocks
uint a;                   	// used as the number of data block
char* byte_addr;          	// used as address of byte for a bit
uint* cat_ind;            	// used to check indirect address
int error_flag;			// used as flag to check error
int size, ninodes;
void *img_ptr;


void print_inode(void *img_ptr, struct dinode dip) {
	printf(" ---------------------------------\n");
	printf("file type:%d,", dip.type);
	printf("nlink:%d,", dip.nlink);
	printf("size:%d,", dip.size);
	printf("first_addr:%d\n", dip.addrs[0]);
	printf(" ---------------------------------\n");
	
  	uint a;
  	struct xv6_dirent *dir;
  	if (dip.type == 1) {
		for (int i = 0; i < NDIRECT; i++) {
	  		if ((a = dip.addrs[i]) == 0) continue;
	  		for (dir = (struct xv6_dirent *)(img_ptr + a*BSIZE);
		  		dir < (struct xv6_dirent *)(img_ptr + (a+1)*BSIZE); dir++) {
				if (dir == 0) continue;
				else printf("%d %s\n", dir->inum, dir->name);
	  		}
		}
  	}
}

void error(const char* s) {
    printf("%s\n", s);
    free(inode_used);
	free(inode_ref);
    exit(1);
}

void check_parent(int ind, int parent) {
	struct xv6_dirent * dir;

	for (int i = 0; i < NDIRECT; i++) {
      	if ((a = dip[parent].addrs[i]) == 0) continue;
      	if (a < start_data || a >= size)
      		error("ERROR: bad direct address in inode.");
      	
      	dir = (struct xv6_dirent *)(img_ptr + a*BSIZE);
		for (int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++)
			if (dir[n].inum == ind) return;
	}

	if ((a = dip[parent].addrs[NDIRECT]) != 0) {
		if (a < start_data || a >= size)
      		error("ERROR: bad direct address in inode.");

      	cat_ind = (uint*)(img_ptr + a * BSIZE); // address of indirect data block
        for (int off = 0; off < BSIZE/sizeof(uint); off++) {
        	if (cat_ind[off] == 0) continue;
        	dir = (struct xv6_dirent *)(img_ptr + cat_ind[off]*BSIZE);
			for (int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++)
				if (dir[n].inum == ind) return;
		}
	}
	error("ERROR: parent directory mismatch.");
}

void check_dir_block(void *addr, int ind) {
	struct xv6_dirent *dir = (struct xv6_dirent *)addr;
	for (int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++) {
		if (dir[n].inum == 0) continue;

		if (dip[dir[n].inum].type == 1) {
			if (strcmp(dir[n].name, ".") == 0) {
			if (dir[n].inum != ind)
				error("ERROR: directory not properly formatted.");
			error_flag = 0;
			} else if (strcmp(dir[n].name, "..") == 0 ) {
				check_parent(ind, dir[n].inum);
			} else if (inode_ref[dir[n].inum])
				error("ERROR: directory appears more than once in file system.");
		}	
		// check inode number?
		if (dir[n].inum < 0 || dir[n].inum > ninodes)
			error("[check_dir_block]: wrong inode number");
		inode_ref[dir[n].inum]++;
	}
}

// ------------------------------------
// 			main function
// ------------------------------------

int main(int argc, char *argv[]) {
	int fd;
	if (argc == 2) {
		fd = open(argv[1], O_RDONLY);
  	} else {
		printf("Usage: xcheck <file_system_image>\n");
		exit(1);
	}
  	if (fd < 0) {
		printf("%s image not found\n", argv[1]);
		exit(1);
  	}

 	struct stat sbuf;  // fstat() return information about a file.
  	fstat(fd, &sbuf);  // printf("Image is %ld in size\n", sbuf.st_size);

  	// use mmap to map the file into virtual memory
  	img_ptr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  	if (img_ptr == (void *)-1)
  		error("img_ptr == -1");

  	// ------------------------------------
  	//   size 1024 nblocks 995 ninodes 200
	//  finish check file, begin to set up
	// ------------------------------------

  	struct superblock *sb = (struct superblock *) (img_ptr + BSIZE);
  	printf("size %d, nblocks %d, ninodes %d\n", sb->size, sb->nblocks, sb->ninodes);

  	start_data = sb->ninodes/IPB + 4; 					// start of data block
  	start_bitmap = BBLOCK(start_data, sb->ninodes); 	// start of bitmap
  	ninodes = sb->ninodes;								// number of inodes
  	size = sb->size;									// size of file system
  	record = calloc(sb->size, sizeof(int));				// for comparison with bitmap
	for (int i = 0; i < start_data; i++)
		record[i] = 1;
	inode_used = calloc(sb->ninodes, sizeof(int));
	inode_ref = calloc(sb->ninodes, sizeof(int));
	inode_used[1] = inode_ref[1] = 1;
  	error_flag = 1;
  	struct xv6_dirent *dir;   	// point to directory entry in data blocks

	// ---------------------------------
	//  set up complete, begin to check
	// ---------------------------------

  	// check root directory
	dip = (struct dinode *) (img_ptr + 2*BSIZE);
	if (dip[ROOTINO].type == 1) {
		for (int i = 0; i < NDIRECT; i++) { // check every direct address
			if ((a = dip[ROOTINO].addrs[i]) == 0) continue;
			dir = (struct xv6_dirent *)(img_ptr + a*BSIZE);
			for (int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++) {
				if (dir[n].inum == 0) continue;
				if (dir[n].inum == ROOTINO) {
					if (strcmp(dir[n].name, "..") == 0) error_flag = 0;
					else if (strcmp(dir[n].name, ".") == 0) continue;
					else { error_flag = 1; break; }
				}
			}
		}
	}
	if (error_flag)
		error("ERROR: root directory does not exist.");

	// check inodes and data blocks, from staring point of inode
	for (int ind = 1; ind < sb->ninodes; ind++) {
    	if (dip[ind].type == 0) continue;

		// check if inode is valid
    	if (dip[ind].type < 0 || dip[ind].type > 3) 
        	error("ERROR: bad inode.");

      	// check direct data block addresses within inode
      	error_flag = 1;
      	for (int i = 0; i < NDIRECT; i++) {
      		if ((a = dip[ind].addrs[i]) == 0) continue;

      		// check if data block is valid
      		if (a < start_data || a >= sb->size)
      			error("ERROR: bad direct address in inode.");
      		else if (record[a] == 1)
      			error("ERROR: direct address used more than once.");
      		byte_addr = (char *)BITADDR(img_ptr, start_bitmap, a);
      		if ((*byte_addr & 1<<BITOFF(a)) == 0)
      			error("ERROR: address used by inode but marked free in bitmap.");
      		record[a] = 1;

      		// assume that . and .. are both direct
      		if (dip[ind].type == 1) 	// data block stores directory entries
      			check_dir_block((img_ptr + a*BSIZE), ind);
      	}
      	if (dip[ind].type == 1 && error_flag)
      		error("ERROR: directory not properly formatted.");

      	// check indirect addresses
      	if((a=dip[ind].addrs[NDIRECT]) != 0) {
      		if (a < start_data || a >= sb->size)
        		error("ERROR: bad indirect address in inode.");
      		else if (record[a] == 1)
        		error("ERROR: indirect address used more than once.");

        	cat_ind = (uint*)(img_ptr + a * BSIZE); // address of indirect address
        	for (int off = 0; off < BSIZE/sizeof(uint); off++) {
        		if (cat_ind[off] == 0) continue;
        		if (cat_ind[off] < start_data || cat_ind[off] >= sb->size)
        			error("ERROR: bad indirect address in inode.");
        		else if (record[cat_ind[off]] == 1)
        			error("ERROR: indirect address used more than once.");
        		byte_addr = (char *)BITADDR(img_ptr, start_bitmap, cat_ind[off]);
        		if ((*byte_addr & 1<<BITOFF(cat_ind[off])) == 0)
        			error("ERROR: address used by inode but marked free in bitmap.");

        		// cat_ind[off] is a valid data block;
        		record[cat_ind[off]] = 1;
        		if (dip[ind].type == 1) // if inode is directory, need to check dir entries
        			check_dir_block((img_ptr + record[cat_ind[off]]*BSIZE), ind);
      		}
        	record[a] = 1;

      	}
      	inode_used[ind] = 1;		// mark inode as used
      	//printf("ind %d \n", ind);
	}

	// check bitmap consistency with data blocks
	for (int i = start_data; i < sb->nblocks; i++) {
		byte_addr = (char *)BITADDR(img_ptr, start_bitmap, i);
		if ((*byte_addr & 1<<BITOFF(i)) != 0 && record[i] == 0)
			error("ERROR: bitmap marks block in use but it is not in use.");
	}

	for (int id = 0; id < sb->ninodes; id++) {
		if (inode_used[id] == 0 && inode_ref[id] == 0) continue;
		if (inode_ref[id] == 0){
			printf("%d ", id);
			error("ERROR: inode marked use but not found in a directory.");
		}
		if (inode_used[id] == 0)
			error("ERROR: inode referred to in directory but marked free.");
		if (dip[id].type == 2) {
			if (dip[id].nlink != inode_ref[id])
				error("ERROR: bad reference count for file.");
		}
	}

	free(inode_used);
	free(inode_ref);
	return 0;
}