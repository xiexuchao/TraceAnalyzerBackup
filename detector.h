/*************************************************************************
> File Name: detector.h
> Author: Xuchao Xie
> Mail: xiexuchao@foxmail.com
> Created Time: Sat 27 Feb 2016 11:47:23 PM CST
************************************************************************/

#ifndef _DETECTOR_H
#define _DETECTOR_H

#include "pool.h"

/*Sequential Accesses Detection*/
struct stream_info{
	unsigned int chk_id;
	unsigned int type;	//read or write
	unsigned int sum;	//IO requests absorbed in
	unsigned int size;	//Sectors(512Bytes)
	long long min;		//start lba
	long long max;		//current max lba
	long long time;		//time when the first req arrived
};

void seq_detect(struct pool_info *pool);
void stream_flush(struct pool_info *pool);

#endif
