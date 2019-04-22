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

struct inodes_t {
	int used;
	int ref;
	int parent;
	int self;
};

// global data
uint start_data, start_bitmap;
int *record;
//int *record, *inode_used, *inode_ref;
struct inodes_t *inodes = 0;
struct dinode *dip;       	// point to inode in inode blocks
uint a;                   	// used as the number of data block
char* byte_addr;          	// used as address of byte for a bit
uint* cat_ind;            	// used to check indirect address
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
				else printf("60: %d %s\n", dir->inum, dir->name);
	  		}
		}
  	}
}

void error(const char* s) {
	fprintf(stderr,"%s\n", s);
    free(inodes);
    exit(1);
}

void check_parent(int ind, int parent) {
	struct xv6_dirent * dir;

	// direct data blocks
	for (int i = 0; i < NDIRECT; i++) {
      	if ((a = dip[parent].addrs[i]) == 0) continue;
      	if (a < start_data || a >= size)
      		error("ERROR: bad direct address in inode.");
      	
      	dir = (struct xv6_dirent *)(img_ptr + a*BSIZE);
		for (int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++)
			if (dir[n].inum == ind) return;
	}

	// indirect data blocks
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
	// printf("104: block: %d\n", ind);
	for (int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++) {
		if (dir[n].inum == 0) continue;

		if (dip[dir[n].inum].type == 1) {
			if (strcmp(dir[n].name, ".") == 0) {
				if (dir[n].inum != ind)
					error("ERROR: directory not properly formatted.");
				inodes[ind].self = ind;
			} else if (strcmp(dir[n].name, "..") == 0 ) {
				if (inodes[ind].parent != 0 && inodes[ind].parent != dir[n].inum)
					error("ERROR: parent directory mismatch.");
				check_parent(ind, dir[n].inum);
				inodes[ind].parent = dir[n].inum; //printf("121: %d\n", dir[n].inum);
			} else if (inodes[dir[n].inum].ref)
				error("ERROR: directory appears more than once in file system.");
			else
				inodes[dir[n].inum].parent = ind;
		} else
			inodes[dir[n].inum].parent = ind;
			// check inode number?
		if (dir[n].inum < 0 || dir[n].inum > ninodes)
			error("126 [check_dir_block]: wrong inode number");
		inodes[dir[n].inum].ref++;
	}
}

int main(int argc, char *argv[]) {
	int fd;
	if (argc == 2) {
		fd = open(argv[1], O_RDONLY);
  	} else {
		printf("Usage: xcheck <file_system_image>\n");
		exit(1);
	}
  	if (fd < 0)
  		error("image not found.");


 	struct stat sbuf;  // fstat() return information about a file.
  	fstat(fd, &sbuf);  // printf("Image is %ld in size\n", sbuf.st_size);

  	// use mmap to map the file into virtual memory
  	img_ptr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  	if (img_ptr == (void *)-1)
  		error("img_ptr == -1");

  	// ------------------------------------
	//  finish check file, begin to set up
	// ------------------------------------

  	struct superblock *sb = (struct superblock *) (img_ptr + BSIZE);
	// printf("size %d, blocks %d, inodes %d\n", sb->size, sb->nblocks, sb->ninodes);

  	start_data = sb->ninodes/IPB + 4; 					// start of data block
  	start_bitmap = BBLOCK(start_data, sb->ninodes); 	// start of bitmap
  	ninodes = sb->ninodes;								// number of inodes
  	size = sb->size;									// size of file system
  	record = calloc(sb->size, sizeof(int));				// for comparison with bitmap
	for (int i = 0; i < start_data; i++)
		record[i] = 1;
	inodes = calloc(sb->ninodes, sizeof(struct inodes_t));
	inodes[1].used = inodes[1].ref = 1;
  	struct xv6_dirent *dir;   	// point to directory entry in data blocks

	// ---------------------------------
	//  set up complete, begin to check
	// ---------------------------------

  	// check root directory
	dip = (struct dinode *) (img_ptr + 2*BSIZE); // print_inode(img_ptr, dip[1]);
	if (dip[ROOTINO].type == 1) {
		for (int i = 0; i < NDIRECT; i++) { // check every direct address
			if ((a = dip[ROOTINO].addrs[i]) == 0) continue;
			dir = (struct xv6_dirent *)(img_ptr + a*BSIZE);
			for (int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++) {
				if (dir[n].inum == 0) continue;
				if (strcmp(dir[n].name, "..") == 0) {
					if (dir[n].inum != ROOTINO)
						error("ERROR: root directory does not exist.");
					inodes[ROOTINO].parent = ROOTINO;
				}
			}
		}

	}
	if (inodes[ROOTINO].parent > ROOTINO || dip[ROOTINO].type != 1)
		error("ERROR: root directory does not exist.");


	// check inodes and data blocks, from staring point of inode
	for (int ind = 1; ind < sb->ninodes; ind++) {
    	if (dip[ind].type == 0) continue;
		// check if inode is valid
    	if (dip[ind].type < 0 || dip[ind].type > 3) 
        	error("ERROR: bad inode.");

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
      	if (dip[ind].type == 1 && (inodes[ind].self != ind || !inodes[ind].parent))
      		// printf("255: parent %d, self %d, ind %d\n", inodes[ind].parent, inodes[ind].self, ind);
      		error("ERROR: directory not properly formatted.");

      	inodes[ind].used = 1;		// mark inode as used
      	//printf("ind %d \n", ind);
	}

	// check bitmap consistency with data blocks
	for (int i = start_data; i < sb->nblocks; i++) {
		byte_addr = (char *)BITADDR(img_ptr, start_bitmap, i);
		if ((*byte_addr & 1<<BITOFF(i)) != 0 && record[i] == 0)
			error("ERROR: bitmap marks block in use but it is not in use.");
	}

	for (int id = 0; id < sb->ninodes; id++) {
		if (inodes[id].used == 0 && inodes[id].ref == 0) continue;
		if (inodes[id].ref == 0){
			error("ERROR: inode marked use but not found in a directory.");
		}
		if (inodes[id].used == 0)
			error("ERROR: inode referred to in directory but marked free.");
		if (dip[id].type == 2) {
			if (dip[id].nlink != inodes[id].ref)
				error("ERROR: bad reference count for file.");
		}
	}

	// check if exists inaccessible directories
	int fast, slow;
	for (int i = 1; i < sb->ninodes; i++) {
		if (!inodes[i].used) continue;
		fast = slow = inodes[i].parent;
		while (fast != 1) {
			fast = inodes[fast].parent;
			if (!fast || !inodes[fast].used)
				//printf("291: %d parent: %d\n", i, inodes[fast].parent);
				error("ERROR: inaccessible directory exists.");
			if (fast == 1)
				break;
			fast = inodes[fast].parent;
			if (!fast || !inodes[fast].used){
				error("ERROR: inaccessible directory exists.");
			}
			if (fast == 1)
				break;
			slow = inodes[slow].parent;
			if ((fast == slow) != 1)
				error("ERROR: inaccessible directory exists.");
		}
	}

	free(inodes);
	return 0;
}