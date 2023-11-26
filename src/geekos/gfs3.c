/*
 * GeekOS file system
 * Copyright (c) 2008, David H. Hovemeyer <daveho@cs.umd.edu>, 
 * Neil Spring <nspring@cs.umd.edu>, Aaron Schulman <schulman@cs.umd.edu>
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 * Any student solutions using any of this code base constitute derviced work and may
 * not be redistributed in any form.  This includes (but is not limited to) posting on
 * public forums or web sites, providing copies to (past, present, or future) students
 * enrolled in similar operating systems courses the University of Maryland's CMSC412 course.
 */


#include <limits.h>
#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/bitset.h>
#include <geekos/synch.h>
#include <geekos/bufcache.h>
#include <geekos/gfs3.h>
#include <geekos/pfat.h>
#include <geekos/projects.h>

/* ----------------------------------------------------------------------
 * Private data and functions
 * ---------------------------------------------------------------------- */

struct GFS_Instance {
    struct gfs3_superblock superblock;
    gfs3_inodenum rootInode;
};

struct GFS3_File {
    struct gfs3_inode *inode;
    gfs3_inodenum inode_num;
};

struct gfs3_inode getInode(struct GFS_Instance *instance, struct Block_Device *dev, gfs3_inodenum inum) {
    struct gfs3_inode blockBuffer[16];
    gfs3_blocknum blockNumber = (inum / (SECTOR_SIZE / sizeof(struct gfs3_inode))) + (instance->superblock.block_with_inode_zero);
    int blockIndex = inum % (SECTOR_SIZE / sizeof(struct gfs3_inode));
    Block_Read(dev, blockNumber, blockBuffer);
    return blockBuffer[blockIndex];
}

/* ----------------------------------------------------------------------
 * Implementation of VFS operations
 * ---------------------------------------------------------------------- */

/*
 * Get metadata for given file.
 */
static int GFS3_FStat(struct File *file, struct VFS_File_Stat *stat) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem FStat operation");
    return 0;
}

/*
 * Read data from current position in file.
 */
static int GFS3_Read(struct File *file, void *buf, ulong_t numBytes) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem read operation");

    return EUNSUPPORTED;
}

/*
 * Write data to current position in file.
 */
static int GFS3_Write(struct File *file, void *buf, ulong_t numBytes) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem write operation");
    return EUNSUPPORTED;
}


/*
 * Seek to a position in file; returns 0 on success.
 */
static int GFS3_Seek(struct File *file, ulong_t pos) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem seek operation");
    return EUNSUPPORTED;
}

/*
 * Close a file.
 */
static int GFS3_Close(struct File *file) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem close operation");
    return EUNSUPPORTED;
}

/*static*/ struct File_Ops s_gfs3FileOps = {
    &GFS3_FStat,
    &GFS3_Read,
    &GFS3_Write,
    &GFS3_Seek,
    &GFS3_Close,
    0,                          /* Read_Entry */
};

/*
 * Stat operation for an already open directory.
 */
static int GFS3_FStat_Directory(struct File *dir,
                                struct VFS_File_Stat *stat) {
    /* may be unused. */
    TODO_P(PROJECT_GFS3, "GeekOS filesystem FStat directory operation");
    return 0;
}

/*
 * Directory Close operation.
 */
static int GFS3_Close_Directory(struct File *dir) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem Close directory operation");
    return EUNSUPPORTED;
}

/*
 * Read a directory entry from an open directory.
 */
static int GFS3_Read_Entry(struct File *dir, struct VFS_Dir_Entry *entry) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem Read_Entry operation");
    return EUNSUPPORTED;
}

/*static*/ struct File_Ops s_gfs3DirOps = {
    &GFS3_FStat_Directory,
    0,                          /* Read */
    0,                          /* Write */
    0,                          /* Seek */
    &GFS3_Close_Directory,
    &GFS3_Read_Entry,
};



/*
 * Open a file named by given path.
 */
static int GFS3_Open(struct Mount_Point *mountPoint, const char *path,
                     int mode, struct File **pFile) {
    Print("GFS3_Open: %s\n", path);  
    ++path;
    int i = 0;
    int numSegments = 1;
    char **directories = Malloc(sizeof(char *) * 10);
    char *fileName;
    char *pos = (char *)path;
    Print("path: %s\n", pos);

    char *delimed = strchr(path, '/');
    if (delimed == NULL){
        directories = NULL;
        fileName = pos;
    }
    else{
        while (delimed != NULL) {
            *delimed = '\0';
            directories[i++] = pos;
            numSegments++;
            pos = delimed + 1;
            delimed = strchr(pos, '/');
        }
        fileName = pos;
    }

    Print("fileName: %s\n", fileName);
    for (i = 0; i < 10; ++i){
        if (directories[i] != NULL){
            Print("directories[%d]: %s\n", i, directories[i]);
        }
    }

    struct gfs3_inode inode = getInode((struct GFS_Instance *)mountPoint->fsData, mountPoint->dev, ((struct GFS_Instance *)(mountPoint->fsData))->rootInode);
    struct gfs3_dirent *dirents[16];
    int read = 0;
    bool found = false;
    bool fileFound = false;
    char *target = 0;

    int k = 0;

    for (i = 0; i < numSegments; i++){
        if (i == numSegments - 1){
            //last segment = file
            target = fileName;
        }
        else{
            target = directories[i];
        }
        if(inode.type == GFS3_DIRECTORY){
            int j = 0;
            for (j = 0; j < GFS3_EXTENTS; j++){
                gfs3_blocknum blockNumber = inode.extents[j].start_block;
                Block_Read(mountPoint->dev, blockNumber, dirents);
                while(read < (int)inode.size){
                    dirents[k]->name[dirents[k]->name_length] = '\0';
                    if (strcmp(dirents[k]->name, target) == 0){
                        inode = getInode((struct GFS_Instance *)mountPoint->fsData, mountPoint->dev, dirents[k]->inum);
                        found = true;
                        if (i == numSegments - 1){
                            fileFound = true;
                        }
                        break;
                    }
                    read += sizeof(gfs3_inodenum) + sizeof(dirents[k]->entry_length) + sizeof(dirents[k]->name_length) + sizeof(dirents[k]->name);
                    k++;
                }
                read = 0;
                if(found){
                    found = false;
                    break;
                }
            }
        }
        if (fileFound){
            break;
        }
    }
    if (!fileFound){
        return ENOTFOUND;
    }

    struct GFS3_File gfsFile;
    gfsFile.inode = &inode;
    gfsFile.inode_num = dirents[k]->inum;
    struct File *file = Allocate_File(&s_gfs3FileOps, 0, inode.size, &gfsFile, mode, mountPoint);
    if (!file){
        return ENOMEM;
    }

    *pFile = file;
    return 0;
}

/*
 * Create a directory named by given path.
 */
static int GFS3_Create_Directory(struct Mount_Point *mountPoint,
                                 const char *path) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem create directory operation");
    return EUNSUPPORTED;
}

/*
 * Open a directory named by given path.
 */
static int GFS3_Open_Directory(struct Mount_Point *mountPoint,
                               const char *path, struct File **pDir) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem open directory operation");
    return EUNSUPPORTED;
}

/*
 * Open a directory named by given path.
 */
static int GFS3_Delete(struct Mount_Point *mountPoint, const char *path,
                       bool recursive) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem delete operation");
    return EUNSUPPORTED;
}

/*
 * Get metadata (size, permissions, etc.) of file named by given path.
 */
static int GFS3_Stat(struct Mount_Point *mountPoint, const char *path,
                     struct VFS_File_Stat *stat) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem stat operation");
    return EUNSUPPORTED;
}

/*
 * Synchronize the filesystem data with the disk
 * (i.e., flush out all buffered filesystem data).
 */
static int GFS3_Sync(struct Mount_Point *mountPoint) {
    TODO_P(PROJECT_GFS3, "GeekOS filesystem sync operation");
    return EUNSUPPORTED;
}

static int GFS3_Disk_Properties(struct Mount_Point *mountPoint,
                                unsigned int *block_size,
                                unsigned int *blocks_in_disk) {
    TODO_P(PROJECT_GFS3,
           "GeekOS filesystem infomation operation; set variables.");
    return EUNSUPPORTED;
}

/*static*/ struct Mount_Point_Ops s_gfs3MountPointOps = {
    &GFS3_Open,
    &GFS3_Create_Directory,
    &GFS3_Open_Directory,
    &GFS3_Stat,
    &GFS3_Sync,
    &GFS3_Delete,
    0,                          /* Rename  */
    0,                          /* Link  */
    0,                          /* SymLink  */
    0,                          /* setuid  */
    0,                          /* acl  */
    &GFS3_Disk_Properties,
};

static int GFS3_Format(struct Block_Device *blockDev
                       __attribute__ ((unused))) {
    TODO_P(PROJECT_GFS3,
           "DO NOT IMPLEMENT: There is no format operation for GFS3");
    return EUNSUPPORTED;
}

static int GFS3_Mount(struct Mount_Point *mountPoint) {
    struct GFS_Instance *instance = 0;

    /* Allocate instance. */
    instance = (struct GFS_Instance *)Malloc(sizeof(*instance));
    if(instance == 0)
        return ENOMEM;
    memset(instance, '\0', sizeof(*instance));

    /* Read superblock. */
    void *bootBlock = Malloc(SECTOR_SIZE);
    Block_Read(mountPoint->dev, 0, bootBlock);
    memcpy(&instance->superblock, bootBlock+PFAT_BOOT_RECORD_OFFSET, sizeof(struct gfs3_superblock));

    /* Check magic number. */
    if(instance->superblock.gfs3_magic != GFS3_MAGIC) {
        Print("GFS3_Mount: bad magic number %x\n", instance->superblock.gfs3_magic);
        return -1;
    }

    mountPoint->fsData = instance;
    mountPoint->ops = &s_gfs3MountPointOps;
    Free(bootBlock);

    return 0;
}


static struct Filesystem_Ops s_gfs3FilesystemOps = {
    &GFS3_Format,
    &GFS3_Mount,
};

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

void Init_GFS3(void) {
    Register_Filesystem("gfs3", &s_gfs3FilesystemOps);
}
