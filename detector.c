/*************************************************************************
> File Name: seq_detect.h
> Author: Xuchao Xie
> Mail: xiexuchao@foxmail.com
> Created Time: Sat 27 Feb 2016 11:43:00 PM CST
************************************************************************/
#include "detector.h"

//Sequential IO Detection
void seq_detect(struct pool_info *pool)
{
	unsigned int i,distribute=FAILURE;
	long long min_time=0x7fffffffffffffff;
	unsigned int min_stream; 

	for(i=0;i<pool->size_stream;i++)
	{
		if(pool->stream[i].size!=0)
		{
			if(pool->req->type==pool->stream[i].type)
			{
				if((pool->req->lba>=pool->stream[i].min)&&
					(pool->req->lba<=(pool->stream[i].max+pool->size_stride+1)))
				{
					pool->stream[i].sum++;
					pool->stream[i].size+=pool->req->size;
					if((pool->req->lba+pool->req->size)>(pool->stream[i].max+pool->size_stride))
					{
						pool->stream[i].max=pool->req->lba+pool->req->size-1;
					}
					pool->stream[i].time=pool->req->time;
					distribute=SUCCESS;
					break;
				}
			}
		}
	}
	if(distribute!=SUCCESS)
	{
		for(i=0;i<pool->size_stream;i++)
		{
			if(pool->stream[i].size==0)
			{
				pool->stream[i].chk_id=(unsigned int)(pool->req->lba/(pool->size_chunk*2048));
				pool->stream[i].type=pool->req->type;
				pool->stream[i].sum=1;
				pool->stream[i].size=pool->req->size;
				pool->stream[i].min=pool->req->lba;
				pool->stream[i].max=pool->req->lba+pool->req->size-1;
				pool->stream[i].time=pool->req->time;
				distribute=SUCCESS;
				break;
			}
		}
	}
	if(distribute!=SUCCESS)/*Using LRU to kick out a stream*/
	{
		for(i=0;i<pool->size_stream;i++)
		{
			if(pool->stream[i].time<min_time)
			{
				min_time=pool->stream[i].time;
				min_stream=i;
			}
		}
		if(pool->stream[min_stream].size>=pool->threshold_sequential)
		{
			pool->seq_stream_all++;
			pool->seq_sum_all+=pool->stream[min_stream].sum;
			pool->seq_size_all+=(long double)pool->stream[min_stream].size/2048;
			pool->chunk[pool->stream[min_stream].chk_id].seq_stream_all++;
			pool->chunk[pool->stream[min_stream].chk_id].seq_sum_all+=pool->stream[min_stream].sum;
			pool->chunk[pool->stream[min_stream].chk_id].seq_size_all+=(long double)pool->stream[min_stream].size/2048;
			if(pool->stream[min_stream].type==READ)
			{
				pool->seq_stream_read++;
				pool->seq_sum_read+=pool->stream[min_stream].sum;
				pool->seq_size_read+=(long double)pool->stream[min_stream].size/2048;
				pool->chunk[pool->stream[min_stream].chk_id].seq_stream_read++;
				pool->chunk[pool->stream[min_stream].chk_id].seq_sum_read+=pool->stream[min_stream].sum;
				pool->chunk[pool->stream[min_stream].chk_id].seq_size_read+=(long double)pool->stream[min_stream].size/2048;
			}
			else
			{
				pool->seq_stream_write++;
				pool->seq_sum_write+=pool->stream[min_stream].sum;
				pool->seq_size_write+=(long double)pool->stream[min_stream].size/2048;
				pool->chunk[pool->stream[min_stream].chk_id].seq_stream_write++;
				pool->chunk[pool->stream[min_stream].chk_id].seq_sum_write+=pool->stream[min_stream].sum;
				pool->chunk[pool->stream[min_stream].chk_id].seq_size_write+=(long double)pool->stream[min_stream].size/2048;
			}
		}
		pool->stream[min_stream].chk_id=(unsigned int)(pool->req->lba/(pool->size_chunk*2048));
		pool->stream[min_stream].type=pool->req->type;
		pool->stream[min_stream].sum=1;
		pool->stream[min_stream].size=pool->req->size;
		pool->stream[min_stream].min=pool->req->lba;
		pool->stream[min_stream].max=pool->req->lba+pool->req->size-1;
		pool->stream[min_stream].time=pool->req->time;
	}//if
}

void stream_flush(struct pool_info *pool)
{
	unsigned int i;
	/**Flush information in POOL->STREAMS into each Chunks**/
	for(i=0;i<pool->size_stream;i++)
	{
		if(pool->stream[i].size!=0)
		{
			if(pool->stream[i].size>=pool->threshold_sequential)
			{
				pool->seq_stream_all++;
				pool->seq_sum_all+=pool->stream[i].sum;
				pool->seq_size_all+=(long double)pool->stream[i].size/2048;
				pool->chunk[pool->stream[i].chk_id].seq_stream_all++;
				pool->chunk[pool->stream[i].chk_id].seq_sum_all+=pool->stream[i].sum;
				pool->chunk[pool->stream[i].chk_id].seq_size_all+=(long double)pool->stream[i].size/2048;
				if(pool->stream[i].type==READ)
				{
					pool->seq_stream_read++;
					pool->seq_sum_read+=pool->stream[i].sum;
					pool->seq_size_read+=(long double)pool->stream[i].size/2048;
					pool->chunk[pool->stream[i].chk_id].seq_stream_read++;
					pool->chunk[pool->stream[i].chk_id].seq_sum_read+=pool->stream[i].sum;
					pool->chunk[pool->stream[i].chk_id].seq_size_read+=(long double)pool->stream[i].size/2048;
				}
				else
				{
					pool->seq_stream_write++;
					pool->seq_sum_write+=pool->stream[i].sum;
					pool->seq_size_write+=(long double)pool->stream[i].size/2048;
					pool->chunk[pool->stream[i].chk_id].seq_stream_write++;
					pool->chunk[pool->stream[i].chk_id].seq_sum_write+=pool->stream[i].sum;
					pool->chunk[pool->stream[i].chk_id].seq_size_write+=(long double)pool->stream[i].size/2048;
				}
			}
		}
		pool->stream[i].chk_id=0;
		pool->stream[i].type=0;
		pool->stream[i].sum=0;
		pool->stream[i].size=0;		
		pool->stream[i].min=0;
		pool->stream[i].max=0;
		pool->stream[i].time=0;
	}
}

