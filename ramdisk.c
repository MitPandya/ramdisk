#define FUSE_USE_VERSION 30
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fuse.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

long int MEM_SIZE;

//Define structures
typedef struct nodeContents
{
    char node_name[NAME_MAX];
    struct nodeContents *next;
    struct nodeContents *prev;

}nContents;

typedef struct fs_node 
{
    struct stat *node_details;
    struct fs_node *prev;
    char type[1];
    char node_name[NAME_MAX];
    char full_path[PATH_MAX];
    char *load;
    struct fs_node *next;
    nContents *node_contents;
    
}fs_node;
//_____________________________//


fs_node *node_list;

fs_node* get_node(const char *path);
void add_contents_to_node(fs_node *node, char* node_name);
static int fusefs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info* fi);
static int fusefs_getattr(const char* path, struct stat* st);
static int fusefs_create(const char* path, mode_t mode, struct fuse_file_info* fi);
static int fusefs_mkdir(const char* path, mode_t mode);
static int fusefs_open(const char *path, struct fuse_file_info *fi);
static int fusefs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fusefs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int fusefs_unlink(const char* path);
static int fusefs_rmdir(const char* path);


static int fusefs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info* fi)
{
    if(strlen(path)  > PATH_MAX)
        return -ENAMETOOLONG;
    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    nContents *temp;
    fs_node *node = get_node(path);
    
    if (node == NULL) 
        return -ENOENT;
    if(node->node_contents == NULL)
        return 0;
    else
    {
        temp = node->node_contents->next;
        time_t curr_time;
        time(&curr_time);
        node->node_details->st_atime = curr_time;
        while(temp != NULL) 
        {
            filler(buf, temp->node_name, NULL, 0);
            temp = temp -> next;
        }
        return 0;
    }
}

static int fusefs_getattr(const char* path, struct stat* st)
{
    if(strlen(path) > PATH_MAX)
        return -ENAMETOOLONG;
    else
    {
        fs_node *node = get_node(path);
        if (node == NULL) 
            return -ENOENT;
        else
        {
            st->st_ctime = node ->node_details->st_ctime;
            st->st_atime = node ->node_details->st_atime;
            st->st_mtime = node ->node_details->st_mtime;
            st->st_mode = node ->node_details->st_mode;
            st->st_nlink = node ->node_details->st_nlink;
            st->st_uid = node ->node_details->st_uid;
            st->st_gid = node ->node_details->st_gid;
            st->st_size = node ->node_details->st_size;
        }
        return 0;
    }
}


static int fusefs_mkdir(const char* path, mode_t mode) 
{
    if(strlen(path)  > PATH_MAX)
        return -ENAMETOOLONG;
    if(MEM_SIZE - sizeof(fs_node) - sizeof(struct stat) - ( sizeof(char) * PATH_MAX) - (2* sizeof(nContents)) < 0)
        return -ENOMEM;
    time_t now;
    time(&now);
    MEM_SIZE -= MEM_SIZE - sizeof(fs_node) - sizeof(struct stat) - ( sizeof(char) * PATH_MAX) - ( 2* sizeof(nContents));
    char *dir_name, *base_name;
    char temp_path1[PATH_MAX], temp_path2[PATH_MAX];
    strcpy(temp_path1, path);
    strcpy(temp_path2, path);
    dir_name = dirname(temp_path1);
    base_name = basename(temp_path2);

    fs_node *newNode = (fs_node *) malloc(sizeof(fs_node));
    newNode->node_details = (struct stat *)malloc(sizeof(struct stat));
    
    fs_node *curr_node = get_node(dir_name);

     //add new node to node_list
    fs_node *temp_node = node_list;
    while(temp_node -> next != NULL)
        temp_node = temp_node->next;
    temp_node -> next = newNode;
    newNode -> next = NULL;
    //____________________________

    newNode->node_details->st_mode = 0775 | S_IFDIR;
    newNode->node_details->st_nlink = 2;
    newNode->node_details->st_gid = getgid();
    newNode->node_details->st_uid = getuid();
    newNode->node_details->st_size = 4096;
    newNode->node_details->st_atime = now;
    newNode->node_details->st_ctime = now;
    newNode->node_details->st_mtime = now;
    newNode->prev = temp_node;

    strcpy(newNode->type,"d");
    newNode->load = NULL;
    strcpy(newNode->node_name, base_name);
    strcpy(newNode-> full_path, path);
    newNode -> node_contents = (nContents*) malloc(sizeof(nContents));
    newNode -> node_contents -> next = NULL;
    nContents *new_node = (nContents *) malloc(sizeof(nContents));
    strcpy(new_node->node_name, newNode->node_name);
    new_node -> next = NULL;
    nContents *temp = curr_node -> node_contents;

    while(temp->next != NULL){
        temp = temp -> next;
    }
    new_node->prev = temp;
    temp->next = new_node;

    return 0;
}

static int fusefs_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
    fprintf(stderr,"in create, path: %s\n", path);
    if(strlen(path)  > PATH_MAX)
        return -ENAMETOOLONG;
    if(MEM_SIZE - sizeof(fs_node) - sizeof(struct stat) - ( sizeof(char) * PATH_MAX) - sizeof(nContents) < 0)
        return -ENOMEM;
    time_t now;
    time(&now);
    MEM_SIZE -= MEM_SIZE - sizeof(fs_node) - sizeof(struct stat) - ( sizeof(char) * PATH_MAX) - sizeof(nContents);
    char *dir_name, *base_name;
    char temp_path1[PATH_MAX], temp_path2[PATH_MAX];
    strcpy(temp_path1, path);
    strcpy(temp_path2, path);
    dir_name = dirname(temp_path1);
    base_name = basename(temp_path2);
    //Make the node structures
    fs_node *newNode = (fs_node *) malloc(sizeof(fs_node));
    newNode->node_details = (struct stat *)malloc(sizeof(struct stat));
    
    fs_node *curr_node = get_node(dir_name);

    //add new node to node_list
    fs_node *temp_node = node_list;
    while(temp_node -> next != NULL)
        temp_node = temp_node->next;
    temp_node -> next = newNode;
    newNode -> next = NULL;

    //fill stat data structure
    newNode->node_details->st_mode = S_IFREG|mode;
    newNode->node_details->st_nlink = 1;
    newNode->node_details->st_gid = getgid();
    newNode->node_details->st_uid = getuid();
    newNode->node_details->st_size = 0;
    newNode->node_details->st_atime = now;
    newNode->node_details->st_ctime = now;
    newNode->node_details->st_mtime = now;
    newNode->prev = temp_node;

    strcpy(newNode->type,"f");
    strcpy(newNode->node_name, base_name);
    strcpy(newNode-> full_path, path);
    newNode -> node_contents = NULL;

    nContents *new_node = (nContents *) malloc(sizeof(nContents));
    strcpy(new_node->node_name, newNode->node_name);
    new_node -> next = NULL;
    nContents *temp = curr_node -> node_contents;

    while(temp->next != NULL){
        temp = temp -> next;
    }
    new_node->prev = temp;
    temp->next = new_node;
    return 0;
}


static int fusefs_open(const char *path, struct fuse_file_info *fi) 
{
   if(strlen(path)  > PATH_MAX)
        return -ENAMETOOLONG;

    fs_node *node = get_node(path);
    if (node != NULL)
        return 0;
    return -ENOENT;
}



static int fusefs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    if(strlen(path)  > PATH_MAX)
        return -ENAMETOOLONG;
    fs_node *node = get_node(path);
    if ( node == NULL) 
        return -ENOENT;
    if (strcmp(node->type, "d") == 0) 
        return -EISDIR;
    time_t now;
    time(&now);
    if (offset < node->node_details->st_size)
    {
        if(node->node_details->st_size - offset < size)
            size = node->node_details->st_size - offset;
        memcpy(buf, (node->load) + offset, size);
    }
    else
        size = 0;
    
    if(size > 0)
        node->node_details->st_atime = now;
    return size;    
}



static int fusefs_unlink(const char* path) {
        if(strlen(path)  > PATH_MAX)
        return -ENAMETOOLONG;
    fs_node *node = get_node(path);
    fs_node *parent_node;
    char *dir_name, *base_name;
    char temp_path1[PATH_MAX], temp_path2[PATH_MAX];
    strcpy(temp_path1, path);
    strcpy(temp_path2, path);
    dir_name = dirname(temp_path1);
    base_name = basename(temp_path2);

    if(node!=NULL)
    {
        parent_node = get_node(dir_name);
    }
    else 
        return -ENOENT;

    if (node -> next == NULL) 
        node -> prev -> next = NULL;
        
    else 
    {
        node->prev->next = node->next; 
        node->next->prev = node->prev;
    }

    
    if (parent_node -> node_contents != NULL) {
        nContents *parent_list = parent_node -> node_contents -> next;
        while(parent_list != NULL){
            int found = strcmp(base_name, parent_list -> node_name);
            if ( found == 0) 
            {
                if (parent_list -> next == NULL) 
                {
                    parent_list -> prev -> next = NULL;
                } 
                else 
                {
                    parent_list -> next -> prev = parent_list -> prev;
                    parent_list -> prev -> next = parent_list -> next;
                    
                }
                free(parent_list);
                break;
            }
            parent_list = parent_list -> next;
        }
    }

    free(node);
   
    MEM_SIZE += sizeof(fs_node) + sizeof(struct stat) + ( 2* sizeof(nContents)) + node->node_details->st_size;
    return 0;
}

static int fusefs_rmdir(const char* path) {
    if(strlen(path)  > PATH_MAX)
        return -ENAMETOOLONG;
    fs_node *node = get_node(path);
    fs_node *parent_node;
    nContents *node_list;
    char *dir_name, *base_name;
    char temp_path1[PATH_MAX], temp_path2[PATH_MAX];
    strcpy(temp_path1, path);
    strcpy(temp_path2, path);
    dir_name = dirname(temp_path1);
    base_name = basename(temp_path2);

    if(node!=NULL)
    {
        parent_node = get_node(dir_name);
        node_list = node -> node_contents;
    }
    else 
        return -ENOENT;

    if (node -> next == NULL) 
        node -> prev -> next = NULL;
        
    else 
    {
        node->prev->next = node->next; 
        node->next->prev = node->prev;
    }

    
    if (parent_node -> node_contents != NULL) {
        nContents *parent_list = parent_node -> node_contents -> next;
        while(parent_list != NULL){
            int found = strcmp(base_name, parent_list -> node_name);
            if ( found == 0) 
            {
                if (parent_list -> next == NULL) 
                {
                    parent_list -> prev -> next = NULL;
                } 
                else 
                {
                    parent_list -> next -> prev = parent_list -> prev;
                    parent_list -> prev -> next = parent_list -> next;
                    
                }
                free(parent_list);
                break;
            }
            parent_list = parent_list -> next;
        }
    }

    while(node_list != NULL)
    {
        free(node_list);
        node_list = node_list->next;
    }

    free(node);
   
    MEM_SIZE += sizeof(fs_node) + sizeof(struct stat) + sizeof(nContents);
    return 0;
}

static int fusefs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if(MEM_SIZE-size < 0)
        return -ENOMEM;
    if(strlen(path)  > PATH_MAX)
        return -ENAMETOOLONG;
    fs_node *node = get_node(path);
    size_t curr_size = node->node_details->st_size;
    time_t curr_time;
    time(&curr_time);

    if(node == NULL)
        return -ENOENT;
    else
    {

        if(strcmp(node->type, "d") == 0)
            return -EISDIR;
        if(size == 0)
            return size;
        if(size > 0)
        {

            if(curr_size > 0)
            {
                if(offset > curr_size)
                    offset = curr_size;
                char *new_file_data = (char *)realloc(node->load, sizeof(char) * (offset+size));
                if(new_file_data == NULL)
                    return -ENOSPC;
                else
                {
                    node->load = new_file_data;
                    MEM_SIZE += curr_size - (offset + size);
                }
            }

            else if(curr_size == 0)
            {
                node->load = (char *)malloc(sizeof(char) * size);
                offset = 0;
                MEM_SIZE-= size;
                   
            }
            memcpy(node->load + offset, buf, size);
            node->node_details->st_size = offset + size;
            node->node_details->st_ctime = curr_time;
            node->node_details->st_mtime = curr_time; 
            return size;   
        }
        return size;  
    } 
}

fs_node* get_node(const char *path)
{
    fs_node *head = node_list;
    
    if (strcmp(path, "/") == 0) 
        return node_list;
    else 
    {
        while(head != NULL)
        {
            char* full_path = head -> full_path;
            if ( strcmp(path, full_path) == 0) 
                return head;
            head = head -> next;
        }
        return NULL;
    }
}

static int fusefs_rename(const char *old_name, const char* new_name) 
{
     
    if(strlen(old_name)  > PATH_MAX || strlen(new_name) > PATH_MAX)
         return -ENAMETOOLONG;
    fs_node *node = get_node(old_name);
    if(node == NULL)
        return -ENOENT;
    else
    {
        time_t now;
        time(&now);
        node->node_details->st_atime = now;
        node->node_details->st_mtime = now;
        char new_path[PATH_MAX], new_path2[PATH_MAX];
        strcpy(new_path, new_name);
        char *new_parent_path = dirname(new_path);
        fs_node *parent_node = get_node(new_parent_path);
        strcpy(new_path2, new_name);
        strcpy(node->full_path, new_name);
        char *new_base = basename(new_path2);
        strcpy(node -> node_name, new_base);
        char old_path[PATH_MAX];
        strcpy(old_path, old_name);
        char *old_base = basename(old_path);
        if (parent_node -> node_contents != NULL)
        {
            nContents *content_list = parent_node->node_contents->next;
            while (content_list != NULL)
            {
                if (strcmp(old_base, content_list -> node_name) == 0) {
                    strcpy(content_list -> node_name, new_base);
                    break;
                }
                content_list = content_list -> next;
            }
        }
        return 0;
    }
}


void init_fs() 
{
    
    node_list = (fs_node *) malloc(sizeof(fs_node));
    
    //create the sublist for the node
    node_list -> node_contents = (nContents *) malloc(sizeof(nContents));
    node_list -> node_contents -> next = NULL;

    //fill the data structures for the node
    strcpy(node_list -> type, "d");
    strcpy(node_list ->node_name, "/");
    strcpy(node_list -> full_path, "/");

    node_list->node_details = (struct stat *)malloc(sizeof(struct stat));
    node_list -> node_details->st_mode = 0755 | S_IFDIR;
    node_list -> node_details->st_nlink = 2;
    node_list -> node_details->st_uid = 0;
    node_list -> node_details->st_gid = 0;
    
    //change the time
    time_t time_now;
    time(&time_now);
    node_list -> node_details->st_atime = time_now;
    node_list -> node_details->st_mtime = time_now;
    node_list -> node_details->st_ctime = time_now;
    
    //set the links
    node_list -> load = NULL;
    node_list -> next = NULL;
    
    //log the used memory
    MEM_SIZE = MEM_SIZE - sizeof(fs_node) - sizeof(struct stat);
    MEM_SIZE -= sizeof(nContents);
}


//operation mapping
static struct fuse_operations fuse_operations = {
    .readdir    =   fusefs_readdir,
    .getattr    =   fusefs_getattr,
    .create     =   fusefs_create,
    .mkdir      =   fusefs_mkdir,
    .open       =   fusefs_open,
    .read       =   fusefs_read,
    .rmdir      =   fusefs_rmdir,
    .unlink     =   fusefs_unlink,
    .write      =   fusefs_write,
    .rename     =   fusefs_rename,
};

int main(int argc, char* argv[])
{
  
    //Basic input valdation
    if (argc <3 || argc >6) 
    {
        printf("USAGE: ./ramdisk <mount point> <size of ramdisk(MB)>\n");
        exit(-1);
    } 
    
    MEM_SIZE = (long)atoi(argv[argc - 1]);
    if (MEM_SIZE == 0) 
    { 
        printf("Enter size more than zero\n");
        exit(-1);
    }

    MEM_SIZE = MEM_SIZE * pow(1024,2); 

    
    printf("File system memory allocated: %ld\n bytes", MEM_SIZE);
    init_fs();
    return fuse_main(argc-1, argv, &fuse_operations, NULL);
}