/*************************************************************************
> File Name: migration.h
> Author: Xuchao Xie
> Mail: xiexuchao@foxmail.com
> Created Time: Sun 28 Feb 2016 09:09:11 AM CST
************************************************************************/

#ifndef _MIGRATION_H
#define _MIGRATION_H

#include "pool.h"

#define SCM_TO_SCM 1
#define SCM_TO_SSD 2
#define SCM_TO_HDD 3
#define SSD_TO_SCM 4
#define SSD_TO_SSD 5
#define SSD_TO_HDD 6
#define HDD_TO_SCM 7
#define HDD_TO_SSD 8
#define HDD_TO_HDD 9

void data_migrate(struct pool_info *pool,unsigned int device);
int find_free(struct pool_info* pool,unsigned int device);
int make_free(struct pool_info* pool,unsigned int device);


#endif
