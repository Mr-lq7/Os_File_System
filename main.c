/*
 * @Author: Deng Cai
 * @Date: 2019-09-09 16:09:14
 * @Last Modified by: Deng Cai
 * @Last Modified time: 2019-12-10 19:16:58
 */

//  in this code : \
    for the file system's space is so small, we don't need \
    group descriptors acctually, so i just put \
    some critical information in other structures and cut group \
    descriptors. what's more, the inode map and block map are \
    put into super block.
//  in some degrees, this file system can be seemed as a system \
    with only one block group, so we need just one super block \
    and one group descriptor. then why don't we put them together \
    and make them one single structure as "super_block"? that's \
    how i handle with it.


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define BLOCK_SIZE 1024
// 1KB.

typedef struct inode {
    // 32 bytes;
    uint32_t size;
    // the number of blocks it have.
    uint16_t file_type;
    // 1->dir; 0->file;
    uint16_t link;  // it doesn't matter if you \
        don't know what this variable means.
   // uint32_t block_point[6];
    int32_t block_point[6];
    // the blocks belonging to this inode.
} inode;

typedef struct super_block {
    // 656 bytes;
    int32_t system_mod;
    // use system_mod to check if it \
        is the first time to run the FS.
    int32_t free_block_count;
    // 2^12; 4096;
    int32_t free_inode_count;
    // 1024;
    int32_t dir_inode_count;
    uint32_t block_map[128];
    // 512 bytes;
    uint32_t inode_map[32];
    // 128 bytes;
} sp_block;
// 1 block;

typedef struct dir_item {
    // the content of folders.
    // 128 bytes;
    uint32_t inode_id;
    uint16_t item_count;
    // 1 means the last one;
    // it doesn't matter if you don't understand it.
    uint8_t type;
    // 1 represents dir;
    char name[121];
} dir_item;

FILE *fp;
sp_block spBlock;
inode inode_table[1024];
dir_item block_buffer[8];
char split[10][20], tmp[1] = "";//存目录
int p;
void print_information(int mode);
// do some pre-work when you run the FS.
void fs_init(sp_block *spBlock);
void ls(char *path);
void create_file(char *path,int size);
void create_dir(char *path);
void delete_file(char *path);
void delete_dir(char *path);
void move(char *from,char *to);
int find_inode_pos() ;
int find_block_pos();
void change_block_map();
void shutdown();
void Split(char *path);
int main()
{
    char str[512];
    fs_init(&spBlock);
    print_information(0);
    while(1) {
        printf("==> ");
        gets(str);
        char command[15], parameter[30], path[30] = {0};
        sscanf(str, "%s %s %s",command, parameter, path);
        if(!strcmp(command, "ls")) ls(parameter);
        else if(!strcmp(command, "create")) {
            if (!strcmp(path, "")) puts("Wrong command.");
            else if(!strcmp(parameter, "-d")) create_dir(path);
            else create_file(path, atoi(parameter));
        }
        else if(!strcmp(command, "delete")) {
            if (!strcmp(path, "")) puts("Wrong command.");
            else if(!strcmp(parameter, "-d")) delete_dir(path);
            else delete_file(path);
        }
        else if(!strcmp(command, "move")) move(parameter, path);
        else if(!strcmp(command, "shutdown")) {
            shutdown();
            break;
        }
    }
    return 0;
}

void print_information(int mode) {
    int o = 4063 - spBlock.free_block_count - spBlock.dir_inode_count;
    //if (mode == 0) {
        printf("In this FileSystem:\n");
        printf("The maximum size of a single file is 6KB;\n");
        printf("The maximum number of files and folders a single folder can contain is 46;\n");
        printf("The whole file system can contain mostly 1024 files and folders;\n");
        printf("**It has %d folders and %d files in this system now;\n", spBlock.dir_inode_count, o);
        printf("**It has %dKB free space now;\n", spBlock.free_block_count);
        printf("**And it can accept another %d new files or folders.\n", 1024 - o - spBlock.dir_inode_count);
        printf("----------------------------------------------------\n");
        printf("!!!!**The instruction should be shorter than 512 bytes**!!!!\n");
        printf("----------------------------------------------------\n");
    //}

}

void fs_init(sp_block* spBlock) {
    int i = 0;
    if ((fp = fopen("disk.os", "rb+")) == NULL) {
        puts("Fail to open file!");
        exit(0);
    }
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    if (size == 0) {
        fseek(fp, 0L, SEEK_SET);
        spBlock->system_mod = 0;
        spBlock->free_block_count = 4062;//剩余的数据块(可用的数据块为4093-32-1)
        spBlock->free_inode_count = 1023;//剩余的inode
        spBlock->dir_inode_count = 1;//已用文件夹数
        memset(spBlock->block_map, 0, sizeof(spBlock->block_map));
        memset(spBlock->inode_map, 0, sizeof(spBlock->inode_map));
        spBlock->block_map[0] = 0xffffffff;
        spBlock->block_map[1] = 3;
        spBlock->inode_map[0] = 1;
        inode_table[0].size = 1;//inode用了多少个数据块
        inode_table[0].file_type = 1;//dir
        memset(inode_table[0].block_point, -1, sizeof(inode_table[0].block_point));
        inode_table[0].block_point[0] = 33;
        memset(block_buffer, 0, sizeof(block_buffer));
        block_buffer[0].inode_id = 0;//在索引表中的位置
        block_buffer[0].item_count = 1;
        block_buffer[0].type = 1;//代表文件夹
        strcpy(block_buffer[0].name, ".");

        //fwrite(spBlock, BLOCK_SIZE, 1, fp);//656占一个数据块,原来的
        fseek(fp, 1024L, SEEK_SET);

        for (i = 0; i < 1024; ++i)
            fwrite(inode_table, sizeof(inode), 1, fp);//32占32个数据块
        fwrite(block_buffer, sizeof(block_buffer), 1, fp);//4063*1024
        for (i = 0; i < 4062; ++i) {
            fwrite(tmp,1024,1,fp);
        }
    }
    else {
        fseek(fp, 0L, SEEK_SET);
        fread(spBlock, BLOCK_SIZE, 1, fp);
        fseek(fp, 1024L, SEEK_SET);
        for (i = 0; i < 1024; ++i)
            fread(inode_table, sizeof(inode), 1, fp);
        spBlock->system_mod = 1;
    }
    return 0;
}

void Split(char *path) {
    p = 0;
    char tmp[10];
    int k = 0, i = 0;
    if (strlen(path) == 1 && path[0] == '/') {
        tmp[k] = path[0];
        tmp[k+1] = '\0';
        strcpy(split[p],tmp);
        ++p;
    } else {
        tmp[k] = path[0];
        tmp[k+1] = '\0';
        strcpy(split[p],tmp);
        ++p;
        for (i = 1; i < strlen(path); ++i) {
            if (path[i] != '/') {
                tmp[k++] = path[i];
            } else {
                tmp[k] = '\0';
                strcpy(split[p], tmp);
                k = 0;
                ++p;
            }
        }
        int l = strlen(path);
        if (path[l-1] != '/') {
            tmp[k] = '\0';
            strcpy(split[p], tmp);
            ++p;
        }
    }
}


void ls(char *path) {
    int i = 0, j = 0;
    //fseek(fp, 0L, SEEK_SET);//原来的
    fseek(fp, 1024L, SEEK_SET);

    //fread(&spBlock, BLOCK_SIZE, 1, fp);
    //fseek(fp, 1024L, SEEK_SET);
    for (i = 0; i < 1024; ++i)
        fread(inode_table, sizeof(inode), 1, fp);
    Split(path);
    int index = 0, id = 0;
    while(index < p - 1) {
        for (i = 0; i < 6; ++i) {
            if (inode_table[id].block_point[i] != -1) {
                fseek(fp, (inode_table[id].block_point[i])*1024L, SEEK_SET);
                fread(block_buffer, sizeof(block_buffer), 1, fp);
                for (j = 0; j < 8; ++j) {
                    if (!strcmp(block_buffer[j].name, split[index + 1])) {
                        id = block_buffer[j].inode_id;
                        ++index;
                        i = 7;
                        break;
                    }
                }
                if (i != 7) {
                    puts("Wrong path.");
                    return;
                }
            }
        }
    }
    for (i = 0; i < 6; ++i) {
        if (inode_table[id].block_point[i] != -1) {
            fseek(fp, (inode_table[id].block_point[i])*1024L, SEEK_SET);
            fread(block_buffer, sizeof(block_buffer), 1, fp);
            for (j = 0; j < 8; ++j) {
                if (block_buffer[j].item_count != 1) {
                    if (block_buffer[j].type == 1) printf("*");
                    printf("%s  ", block_buffer[j].name);
                }
                if (block_buffer[j].item_count == 1) {
                    if (block_buffer[j].type == 1) printf("*");
                    printf("%s  ", block_buffer[j].name);
                    break;
                }
            }
        }
    }
    printf("\n");

    return;
}

int find_inode_pos() {
    int i = 0, j = 0, cnt = 0;
    for (i = 0; i < 32; ++i) {
        int m = spBlock.inode_map[i];
        for(j = 0; j < 32; ++j) {
            if ((m & 1) == 0) {
                cnt = 32*i + j;
                i = 34;
                break;
            }
            m >>= 1;
        }
    }
    if (i == 35) return cnt;
    else return -1;
}

int find_block_pos() {
    int i = 0, j = 0, cnt = 0;
    for (i = 0; i < 128; ++i) {
        uint32_t m = spBlock.block_map[i];
        for(j = 0; j < 32; ++j) {
            if ((m & 1) == 0) {
                cnt = 32*i + j;
                i = 130;
                break;
            }
            m >>= 1;
        }
    }
    if (i == 131) return cnt;
    else return -1;
}


void create_dir(char *path) {
    int cnt = 0, i = 0, j = 0, k = 0, father_id = -1;
    Split(path);
    if (spBlock.free_block_count > 0){
        int id = 0, index = 0;
        if (p == 2) {
            for (i = 0; i < 6; ++i) {
                if (inode_table[id].block_point[i] != -1) {
                    fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                    fread(block_buffer, sizeof(block_buffer),1,  fp);
                    for (k = 0; k < 8; ++k) {
                        if (!strcmp(block_buffer[k].name, split[p-1]) && block_buffer[k].type == 1) {
                            puts("The folder has already existed.");
                            return;
                        }
                    }
                    for (j = 0; j < 8; ++j) {
                        if (block_buffer[j].item_count == 1) {
                            if (j == 7) {
                                if (i == 5) {
                                    puts("Not enough space.");
                                    i = 6;
                                    break;
                                } else {
                                    inode_table[id].block_point[i+1] = inode_table[id].block_point[i] + 1;
                                    break;
                                }
                            } else {
                                block_buffer[j].item_count = 0;
                                strcpy(block_buffer[j+1].name, split[p-1]);
                                block_buffer[j+1].item_count = 1;
                                block_buffer[j+1].type = 1;
                                cnt = find_inode_pos();
                                if (cnt == -1) {
                                    puts("Not enough space.");
                                    i = 6;
                                    break;
                                }
                                block_buffer[j+1].inode_id = cnt;
                                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                                fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                                i = 6;
                                break;
                            }
                        }

                    }
                }
            }
        } else {
            while (index < p - 2) {
                for (i = 0; i < 6; ++i) {
                    if (inode_table[id].block_point[i] != -1) {
                        fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                        fread(block_buffer, sizeof(block_buffer), 1, fp);
                        for (j = 0; j < 8; ++j) {
                            if (!strcmp(block_buffer[j].name, split[index+1])) {
                                id = block_buffer[j].inode_id;
                                ++index;
                                i = 8;
                                break;
                            }
                        }

                    }
                    if (i != 8) {
                        puts("Wrong path.");
                        return;
                    }
                }
            }
            father_id = id;
            for (i = 0; i < 6; ++i) {
                if (inode_table[id].block_point[i] != -1) {
                    fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                    fread(block_buffer, sizeof(block_buffer), 1, fp);
                    for (k = 0; k < 8; ++k) {
                        if (!strcmp(block_buffer[k].name, split[p-1]) && block_buffer[k].type == 1) {
                            puts("The folder has already existed.");
                            return;
                        }
                    }
                    for (j = 0; j < 8; ++j) {
                        if (block_buffer[j].item_count == 1) {
                            if (j == 7) {
                                if (i == 5) {
                                    puts("Not enough space.");
                                    i = 6;
                                    break;
                                } else {
                                    inode_table[id].block_point[i+1] = inode_table[id].block_point[i] + 1;
                                    break;
                                }
                            } else {
                                block_buffer[j].item_count = 0;
                                strcpy(block_buffer[j+1].name, split[p-1]);
                                block_buffer[j+1].item_count = 1;
                                block_buffer[j+1].type = 1;
                                cnt = find_inode_pos();
                                if (cnt == -1) {
                                    puts("Not enough space.");
                                    i = 6;
                                    break;
                                }
                                block_buffer[j+1].inode_id = cnt;
                                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                                fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                                i = 6;
                                break;
                            }
                        }
                    }
                }
            }
        }
        --spBlock.free_inode_count;
        --spBlock.free_block_count;
        ++spBlock.dir_inode_count;
        ++inode_table[cnt].size;
        inode_table[cnt].file_type = 1;
        //inode_table[cnt].block_point[0] = 33 + 6 * cnt;
        int pos = find_block_pos();
        inode_table[cnt].block_point[0] = pos + 1;
        fseek(fp, (inode_table[cnt].block_point[0]) * 1024, SEEK_SET);
        fread(block_buffer, sizeof(block_buffer), 1, fp);
        strcpy(block_buffer[0].name, ".");//代表当前目录
        block_buffer[0].item_count = 0;
        block_buffer[0].inode_id = cnt;
        block_buffer[0].type = 1;
        strcpy(block_buffer[1].name, "..");//代表上一级目录
        block_buffer[1].item_count = 1;
        if (p == 2) block_buffer[1].inode_id = 0;
        else block_buffer[1].inode_id = father_id;
        block_buffer[1].type = 1;
        //spBlock.dir_inode_count += 2;
        //spBlock.free_block_count -= 2;
        fseek(fp, (inode_table[cnt].block_point[0]) * 1024, SEEK_SET);
        fwrite(block_buffer, sizeof(block_buffer), 1, fp);
        uint32_t mm = 1;
        for (i = 0; i < cnt % 32; ++i) mm <<= 1;
        (spBlock.inode_map[cnt/32]) += mm;
        //cnt = find_block_pos();
        mm = 1;
        for (i = 0; i < pos % 32; ++i) mm <<= 1;
        (spBlock.block_map[pos/32]) += mm;
        //fseek(fp,0, SEEK_SET);
        //fwrite(&spBlock, BLOCK_SIZE, 1, fp);
    }
    else {
        puts("Not enough space!\n");
    }
}

void create_file(char *path, int size) {
   int cnt = 0, i = 0, j = 0, k = 0;
   if (size > 6144) {
        puts("Fail!The file is too large.");
        return;
   }
   int block = ceil((float)(size / 1024.0));
    Split(path);
    if (spBlock.free_block_count >= block){
        int id = 0, index = 0;
        if (p == 2) {
            for (i = 0; i < 6; ++i) {
                if (inode_table[id].block_point[i] != -1) {
                    fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                    fread(block_buffer, sizeof(block_buffer),1, fp);
                    for (k = 0; k < 8; ++k) {
                        if (!strcmp(block_buffer[k].name, split[p-1]) && block_buffer[k].type == 0) {
                            puts("The file has already existed.");
                            return;
                        }
                    }
                    for (j = 0; j < 8; ++j) {
                        if (block_buffer[j].item_count == 1) {
                            if (j == 7) {
                                if (i == 5) {
                                    puts("Not enough space.");
                                    i = 6;
                                    break;
                                } else {
                                    inode_table[id].block_point[i+1] = inode_table[id].block_point[i] + 1;
                                    break;
                                }
                            } else {
                                block_buffer[j].item_count = 0;
                                strcpy(block_buffer[j+1].name, split[p-1]);
                                block_buffer[j+1].item_count = 1;
                                block_buffer[j+1].type = 0;
                                cnt = find_inode_pos();
                                if (cnt == -1) {
                                    puts("Not enough space.");
                                    i = 6;
                                    break;
                                }
                                block_buffer[j+1].inode_id = cnt;
                                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                                fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                                i = 6;
                                break;
                            }
                        }

                    }
                }
            }
        } else {
            while (index < p - 2) {
                for (i = 0; i < 6; ++i) {
                    if (inode_table[id].block_point[i] != -1) {
                        fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                        fread(block_buffer, sizeof(block_buffer), 1, fp);
                        for (j = 0; j < 8; ++j) {
                            if (!strcmp(block_buffer[j].name, split[index+1])) {
                                id = block_buffer[j].inode_id;
                                ++index;
                                i = 8;
                                break;
                            }
                        }

                    }
                    if (i != 8) {
                        puts("Wrong path.");
                        return;
                    }
                }
            }
            for (i = 0; i < 6; ++i) {
                if (inode_table[id].block_point[i] != -1) {
                    fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                    fread(block_buffer, sizeof(block_buffer), 1, fp);
                    for (k = 0; k < 8; ++k) {
                        if (!strcmp(block_buffer[k].name, split[p-1]) && block_buffer[k].type == 0) {
                            puts("The folder has already existed.");
                            return;
                        }
                    }
                    for (j = 0; j < 8; ++j) {
                        if (block_buffer[j].item_count == 1) {
                            if (j == 7) {
                                if (i == 5) {
                                    puts("Not enough space.");
                                    i = 6;
                                    break;
                                } else {
                                    inode_table[id].block_point[i+1] = inode_table[id].block_point[i] + 1;
                                    break;
                                }
                            } else {
                                block_buffer[j].item_count = 0;
                                strcpy(block_buffer[j+1].name, split[p-1]);
                                block_buffer[j+1].item_count = 1;
                                block_buffer[j+1].type = 0;
                                cnt = find_inode_pos();
                                if (cnt == -1) {
                                    puts("Not enough space.");
                                    i = 6;
                                    break;
                                }
                                block_buffer[j+1].inode_id = cnt;
                                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                                fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                                i = 6;
                                break;
                            }
                        }
                    }
                }
            }
        }

        inode_table[cnt].size += block;
        inode_table[cnt].file_type = 0;
        spBlock.free_block_count -= block;
        spBlock.free_inode_count -= 1;
        //inode_table[cnt].block_point[0] = 33 + 6 * cnt;
        int pos = find_block_pos();
        for (i = 0; i < block; ++i) {
            inode_table[cnt].block_point[i] = pos + i + 1;
        }
        fseek(fp, (inode_table[cnt].block_point[0]) * 1024, SEEK_SET);
        fread(block_buffer, sizeof(block_buffer), 1, fp);

        fseek(fp, (inode_table[cnt].block_point[0]) * 1024, SEEK_SET);
        fwrite(block_buffer, sizeof(block_buffer), 1, fp);
        uint32_t mm = 1;
        for (i = 0; i < cnt % 32; ++i) mm <<= 1;
        (spBlock.inode_map[cnt/32]) += mm;
        //cnt = find_block_pos();
        mm = 1;
        for (i = 0; i < pos % 32; ++i) mm <<= 1;
        (spBlock.block_map[pos/32]) += mm;
        //fseek(fp,0, SEEK_SET);
        //fwrite(&spBlock, BLOCK_SIZE, 1, fp);
    }
    else {
        puts("Not enough space!\n");
    }
}


//递归删除文件夹
void dele(int id, char *name) {
    int i = 0, j = 0, block_size, father_id = -1;
    block_size = inode_table[id].size;
    for (i = 0; i < 6; ++i) {
        if (inode_table[id].block_point[i] != -1) {
            fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
            fread(block_buffer, sizeof(block_buffer), 1, fp);
            for (j = 0; j < 8; ++j) {
                if (!strcmp(block_buffer[j].name, ".") || !strcmp(block_buffer[j].name, "..")) continue;
                else if (strcmp(block_buffer[j].name, "") && block_buffer[j].type == 1) {
                    //strcpy(name, block_buffer[j].name);
                    id = block_buffer[j].inode_id;//文件
                    dele(id, block_buffer[j].name);

                }
                else if(strcmp(block_buffer[j].name, "") && block_buffer[j].type == 0) {//删除文件
                    block_size = inode_table[block_buffer[j].inode_id].size;
                    if (block_buffer[j].item_count == 1) block_buffer[j-1].item_count = 1;
                        block_buffer[j].item_count = 0;
                        block_buffer[j].inode_id = 0;
                        block_buffer[j].type = 0;
                        strcpy(block_buffer[j].name, "");
                        fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                        fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                         ++spBlock.free_inode_count;
                        spBlock.free_block_count += block_size;
                        uint32_t m = 1;
                        for (i = 0; i < id%32; ++i) m <<= 1;
                        spBlock.inode_map[id/32] -= m;
                        int be = 0, en = 0;
                        be = inode_table[id].block_point[0];
                        en = inode_table[id].block_point[block_size-1];
                        change_block_map(be, en);
                        fseek(fp, (inode_table[id].block_point[0]) * 1024, SEEK_SET);
                        memset(inode_table[id].block_point, -1, sizeof(inode_table[0].block_point));
                        fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                        //fseek(fp,0, SEEK_SET);
                        //fwrite(&spBlock, BLOCK_SIZE, 1, fp);

                }
            }

            father_id = block_buffer[1].inode_id;
            for (i = 0; i < 6; ++i) {
                if (inode_table[father_id].block_point[i] != -1) {
                   fseek(fp, (inode_table[father_id].block_point[i]) * 1024, SEEK_SET);
                   fread(block_buffer, sizeof(block_buffer), 1, fp);
                   for (j = 0; j < 8; ++j) {
                        if (!strcmp(block_buffer[j].name, name)) {

                            if (block_buffer[j].item_count == 1) block_buffer[j-1].item_count = 1;
                            block_buffer[j].item_count = 0;
                            block_buffer[j].inode_id = 0;
                            block_buffer[j].type = 0;
                            strcpy(block_buffer[j].name, "");
                            fseek(fp, (inode_table[father_id].block_point[i]) * 1024, SEEK_SET);
                            fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                            ++spBlock.free_inode_count;
                            --spBlock.dir_inode_count;
                            spBlock.free_block_count += block_size;
                            uint32_t m = 1;
                            for (i = 0; i < id%32; ++i) m <<= 1;
                            spBlock.inode_map[id/32] -= m;
                            int be = 0, en = 0;
                            be = inode_table[id].block_point[0];
                            en = inode_table[id].block_point[block_size-1];
                            change_block_map(be, en);
                            fseek(fp, (inode_table[id].block_point[0]) * 1024, SEEK_SET);
                            memset(inode_table[id].block_point, -1, sizeof(inode_table[0].block_point));
                            fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                            //fseek(fp,0, SEEK_SET);
                            //fwrite(&spBlock, BLOCK_SIZE, 1, fp);
                            return;
                        }

                    }
                }
            }

        }

    }
}

void change_block_map(int be, int en) {
    uint32_t mm = 1;
    int i = 0;
    if (en / 32 == be / 32) {
        for (i = 0; i < be % 32 - 1; ++i) mm <<= 1;
        for (i = be % 32; i <= en % 32; ++i) {
            spBlock.block_map[en/32] -= mm;
            mm <<= 1;
        }
    } else {
        for (i = 0; i < be % 32; ++i) mm <<= 1;
        for (i = be % 32; i < 32; ++i) {
            spBlock.block_map[be/32] -= mm;
            mm <<= 1;
        }
        mm = 1;
        for (i = 0; i <= en % 32; ++i) {
            spBlock.block_map[en/32] -= mm;
            mm <<= 1;
        }
    }
}

void delete_dir(char *path) {
    int cnt = 0, i = 0, j = 0, k = 0;
    int block_size = 0;
    Split(path);
    int id = 0, index = 0;
    if (p == 2) {
        for (i = 0; i < 6; ++i) {
            if (inode_table[id].block_point[i] != -1) {
                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                fread(block_buffer, sizeof(block_buffer), 1, fp);
                for (k = 0; k < 8; ++k) {
                    if (!strcmp(block_buffer[k].name, split[p-1])) {
                        k = 9;
                        break;
                    }
                }
                if (k != 9) {
                    puts("The folder has not existed.");
                    return;
                }
                for (j = 0; j < 8; ++j) {
                    if (!strcmp(block_buffer[j].name, split[p-1])) {
                        block_size = inode_table[block_buffer[j].inode_id].size;
                        id = block_buffer[j].inode_id;//文件夹在inode表中的位置
                        dele(id, split[p-1]);
                        i = 6;
                        break;
                    }
                }
            }
        }
    } else {
        while (index < p - 2) {
            for (i = 0; i < 6; ++i) {
                if (inode_table[id].block_point[i] != -1) {
                    fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                    fread(block_buffer, sizeof(block_buffer), 1, fp);
                    for (j = 0; j < 8; ++j) {
                        if (!strcmp(block_buffer[j].name, split[index+1])) {
                            id = block_buffer[j].inode_id;
                            ++index;
                            i = 8;
                            break;
                        }
                    }
                }
                if (i != 8) {
                    puts("Wrong path.");
                    return;
                }
            }
        }

        for (i = 0; i < 6; ++i) {
            if (inode_table[id].block_point[i] != -1) {
                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                fread(block_buffer, sizeof(block_buffer), 1, fp);
                for (k = 0; k < 8; ++k) {
                    if (!strcmp(block_buffer[k].name, split[p-1])) {
                        k = 9;
                        break;

                    }
                }
                if (k != 9) {
                    puts("The folder has not existed.");
                    return;
                }
               for (j = 0; j < 8; ++j) {
                    if (!strcmp(block_buffer[j].name, split[p-1])) {
                        id = block_buffer[j].inode_id;
                        block_size = inode_table[block_buffer[j].inode_id].size;
                        dele(id, split[p-1]);
                        i = 6;
                        break;
                    }
                }
            }
        }
    }
}

void delete_file(char *path) {
   int cnt = 0, i = 0, j = 0, k = 0, current_id;
    int block_size = 0;
    Split(path);
    int id = 0, index = 0;
    if (p == 2) {
        for (i = 0; i < 6; ++i) {
            if (inode_table[id].block_point[i] != -1) {
                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                fread(block_buffer, sizeof(block_buffer), 1, fp);
                for (k = 0; k < 8; ++k) {
                    if (!strcmp(block_buffer[k].name, split[p-1])) {
                        k = 9;
                        break;
                    }
                }
                if (k != 9) {
                    puts("The file has not existed.\n");
                    return;
                }

                for (j = 0; j < 8; ++j) {
                    if (!strcmp(block_buffer[j].name, split[p-1])) {
                        block_size = inode_table[block_buffer[j].inode_id].size;
                        current_id = block_buffer[j].inode_id;
                        if (block_buffer[j].item_count == 1) block_buffer[j-1].item_count = 1;
                        block_buffer[j].item_count = 0;
                        block_buffer[j].inode_id = 0;
                        block_buffer[j].type = 0;
                        strcpy(block_buffer[j].name, "");
                        fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                        fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                        i = 6;
                        break;
                    }
                }
            }
        }
    } else {
        while (index < p - 2) {
            for (i = 0; i < 6; ++i) {
                if (inode_table[id].block_point[i] != -1) {
                    fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                    fread(block_buffer, sizeof(block_buffer), 1, fp);
                    for (j = 0; j < 8; ++j) {
                        if (!strcmp(block_buffer[j].name, split[index+1])) {
                            id = block_buffer[j].inode_id;
                            ++index;
                            i = 8;
                            break;
                        }
                    }
                }
                if (i != 8) {
                    puts("Wrong path.");
                    return;
                }
            }
        }
        for (i = 0; i < 6; ++i) {
            if (inode_table[id].block_point[i] != -1) {
                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                fread(block_buffer, sizeof(block_buffer), 1, fp);
                for (k = 0; k < 8; ++k) {
                    if (!strcmp(block_buffer[k].name, split[p-1])) {
                        k = 9;
                        break;
                    }
                }
                if (k != 9) {
                    puts("The file has not existed.\n");
                    return;
                }

               for (j = 0; j < 8; ++j) {
                    if (!strcmp(block_buffer[j].name, split[p-1])) {
                        current_id = block_buffer[j].inode_id;
                        block_size = inode_table[block_buffer[j].inode_id].size;
                        if (block_buffer[j].item_count == 1) block_buffer[j-1].item_count = 1;
                        block_buffer[j].item_count = 0;
                        block_buffer[j].inode_id = 0;
                        block_buffer[j].type = 0;
                        strcpy(block_buffer[j].name, "");
                        fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                        fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                        i = 6;
                        break;
                    }
                }
            }
        }
    }
    ++spBlock.free_inode_count;
    spBlock.free_block_count += block_size;
    uint32_t m = 1;
    for (i = 0; i < current_id%32; ++i) m <<= 1;
    spBlock.inode_map[current_id/32] -= m;

    int be = 0, en = 0;
    be = inode_table[current_id].block_point[0];
    en = inode_table[current_id].block_point[block_size-1];
    change_block_map(be, en);
    fseek(fp, (inode_table[current_id].block_point[0]) * 1024, SEEK_SET);
    memset(inode_table[current_id].block_point, -1, sizeof(inode_table[0].block_point));
    fwrite(block_buffer, sizeof(block_buffer), 1, fp);
    //fseek(fp,0, SEEK_SET);
    //fwrite(&spBlock, BLOCK_SIZE, 1, fp);

}

void move(char *from, char *to) {
    int cnt = 0, i = 0, j = 0, k = 0, current_id;
    int block_size = 0;
    int id = 0, index = 0;
    char filename[30];
    Split(from);
    if (p == 2) {
        for (i = 0; i < 6; ++i) {
            if (inode_table[id].block_point[i] != -1) {
                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                fread(block_buffer, sizeof(block_buffer), 1, fp);
                for (k = 0; k < 8; ++k) {
                    if (!strcmp(block_buffer[k].name, split[p-1])) {
                        k = 9;
                        break;
                    }
                }
                if (k != 9) {
                    puts("The file has not existed.\n");
                    return;
                }
                for (j = 0; j < 8; ++j) {
                    if (!strcmp(block_buffer[j].name, split[p-1])) {
                        block_size = inode_table[block_buffer[j].inode_id].size;
                        current_id = block_buffer[j].inode_id;
                        if (block_buffer[j].item_count == 1) block_buffer[j-1].item_count = 1;
                        block_buffer[j].item_count = 0;
                        block_buffer[j].inode_id = 0;
                        block_buffer[j].type = 0;
                        strcpy(block_buffer[j].name, "");
                        fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                        fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                        i = 6;
                        break;
                    }
                }
            }
        }
    } else {
        while (index < p - 2) {
            for (i = 0; i < 6; ++i) {
                if (inode_table[id].block_point[i] != -1) {
                    fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                    fread(block_buffer, sizeof(block_buffer), 1, fp);
                    for (j = 0; j < 8; ++j) {
                        if (!strcmp(block_buffer[j].name, split[index+1])) {
                            id = block_buffer[j].inode_id;
                            ++index;
                            i = 8;
                            break;
                        }
                    }
                }
                if (i != 8) {
                    puts("Wrong path.");
                    return;
                }
            }
        }
        for (i = 0; i < 6; ++i) {
            if (inode_table[id].block_point[i] != -1) {
                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                fread(block_buffer, sizeof(block_buffer), 1, fp);
                for (k = 0; k < 8; ++k) {
                    if (!strcmp(block_buffer[k].name, split[p-1])) {
                        k = 9;
                        break;
                    }
                }
                if (k != 9) {
                    puts("The file has not existed.\n");
                    return;
                }
                for (j = 0; j < 8; ++j) {
                    if (!strcmp(block_buffer[j].name, split[p-1])) {
                        current_id = block_buffer[j].inode_id;
                        block_size = inode_table[block_buffer[j].inode_id].size;
                        if (block_buffer[j].item_count == 1) block_buffer[j-1].item_count = 1;
                        block_buffer[j].item_count = 0;
                        block_buffer[j].inode_id = 0;
                        block_buffer[j].type = 0;
                        strcpy(block_buffer[j].name, "");
                        fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                        fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                        i = 6;
                        break;
                    }
                }
            }
        }
    }
    strcpy(filename,split[p-1]);
    Split(to);
    id = 0;
    index = 0;
    if (p == 1) {
        for (i = 0; i < 6; ++i) {
            if (inode_table[id].block_point[i] != -1) {
                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                fread(block_buffer, sizeof(block_buffer),1, fp);
                for (k = 0; k < 8; ++k) {
                    if (!strcmp(block_buffer[k].name, filename) && block_buffer[k].type == 0) {
                        puts("The file has already existed.");
                        return;
                    }
                }
                for (j = 0; j < 8; ++j) {
                    if (block_buffer[j].item_count == 1) {
                        if (j == 7) {
                            if (i == 5) {
                                puts("Not enough space.");
                                i = 6;
                                break;
                            } else {
                                inode_table[id].block_point[i+1] = inode_table[id].block_point[i] + 1;
                                break;
                            }
                        } else {
                            block_buffer[j].item_count = 0;
                            strcpy(block_buffer[j+1].name, filename);
                            block_buffer[j+1].item_count = 1;
                            block_buffer[j+1].type = 0;
                            block_buffer[j+1].inode_id = current_id;
                            fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                            fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                            i = 6;
                            break;
                        }
                    }

                }
            }
        }
    } else {
        while (index < p - 1) {
            for (i = 0; i < 6; ++i) {
                if (inode_table[id].block_point[i] != -1) {
                    fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                    fread(block_buffer, sizeof(block_buffer), 1, fp);
                    for (j = 0; j < 8; ++j) {
                        if (!strcmp(block_buffer[j].name, split[index+1])) {
                            id = block_buffer[j].inode_id;
                            ++index;
                            i = 8;
                            break;
                        }
                    }

                }
                if (i != 8) {
                    puts("Wrong path.");
                    return;
                }
            }
        }
        for (i = 0; i < 6; ++i) {
            if (inode_table[id].block_point[i] != -1) {
                fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                fread(block_buffer, sizeof(block_buffer),1, fp);
                for (k = 0; k < 8; ++k) {
                    if (!strcmp(block_buffer[k].name, filename) && block_buffer[k].type == 0) {
                        puts("The file has already existed.");
                        return;
                    }
                }
                for (j = 0; j < 8; ++j) {
                    if (block_buffer[j].item_count == 1) {
                        if (j == 7) {
                            if (i == 5) {
                                puts("Not enough space.");
                                i = 6;
                                break;
                            } else {
                                inode_table[id].block_point[i+1] = inode_table[id].block_point[i] + 1;
                                break;
                            }
                        } else {
                            block_buffer[j].item_count = 0;
                            strcpy(block_buffer[j+1].name, filename);
                            block_buffer[j+1].item_count = 1;
                            block_buffer[j+1].type = 0;
                            block_buffer[j+1].inode_id = current_id;
                            fseek(fp, (inode_table[id].block_point[i]) * 1024, SEEK_SET);
                            fwrite(block_buffer, sizeof(block_buffer), 1, fp);
                            i = 6;
                            break;
                        }
                    }

                }
            }
        }

    }
}

void shutdown() {
    int i = 0;
    fseek(fp, 0L, SEEK_SET);
    fwrite(&spBlock, BLOCK_SIZE, 1, fp);
    fseek(fp, 1024L, SEEK_SET);
    for (i = 0; i < 1024; ++i)
        fwrite(inode_table, sizeof(inode), 1, fp);
    fclose(fp);
}
