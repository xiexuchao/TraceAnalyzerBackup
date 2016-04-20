#include "pool.h"

void seq_detection_test(char *trace,char *config,char *output,char *log);
void seq_detect2(struct pool_info *pool);
void stream_flush2(struct pool_info *pool);

void main()
{
	//pool_run("trace1.csv","config.txt","output.txt","log.txt");
	//pool_run("2.trace","config.txt","output.txt","log.txt");
	seq_detection_test("3.trace","config.txt","output.txt","log.txt");
}

void seq_detection_test(char *trace,char *config,char *output,char *log)
{
	struct pool_info *pool;
	pool=(struct pool_info *)malloc(sizeof(struct pool_info));
	alloc_assert(pool,"pool");
	memset(pool,0,sizeof(struct pool_info));

	load_parameters(pool,config);
	initialize(pool,trace,output,log);

	while(get_request_ascii(pool)!=FAILURE)
	{			
		seq_detect2(pool);	//Sequential IO Detection
		stat_update(pool);
	}//while
	stream_flush2(pool);

	stat_print(pool);

	fclose(pool->file_trace);
	fclose(pool->file_output);
	fclose(pool->file_log);

	free(pool->map);
	free(pool->chunk);
	free(pool->record_win);
	free(pool->record_all);
	free(pool->req);
	free(pool->stream);
	free(pool);
}


//Sequential IO Detection
void seq_detect2(struct pool_info *pool)
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
				//if((pool->req->lba>=pool->stream[i].min)&&(pool->req->lba<=(pool->stream[i].max+pool->size_stride+1)))//////
				if(pool->req->lba==(pool->stream[i].max+pool->size_stride+1))
				{
					pool->stream[i].sum++;
					pool->stream[i].max=pool->req->lba+pool->req->size-1;
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
		//printf("@@@@@@@%d\n\n",min_stream);
		//if(pool->stream[min_stream].size>=pool->threshold_sequential)
		if(pool->stream[min_stream].sum>1)
		{
			pool->seq_stream_all++;
			pool->seq_sum_all+=pool->stream[min_stream].sum;
			if(pool->stream[min_stream].type==READ)
			{
				pool->seq_stream_read++;
				pool->seq_sum_read+=pool->stream[min_stream].sum;
			}
			else
			{
				pool->seq_stream_write++;
				pool->seq_sum_write+=pool->stream[min_stream].sum;
			}
		}
		pool->stream[min_stream].type=pool->req->type;
		pool->stream[min_stream].sum=1;
		pool->stream[min_stream].size=pool->req->size;
		pool->stream[min_stream].min=pool->req->lba;
		pool->stream[min_stream].max=pool->req->lba+pool->req->size-1;
		pool->stream[min_stream].time=pool->req->time;
	}//if
}


void stream_flush2(struct pool_info *pool)
{
	unsigned int i;
	/**Flush information in POOL->STREAMS into each Chunks**/
	for(i=0;i<pool->size_stream;i++)
	{
		if(pool->stream[i].size!=0)
		{
			if(pool->stream[i].sum>1)
			{
				pool->seq_stream_all++;
				pool->seq_sum_all+=pool->stream[i].sum;
				if(pool->stream[i].type==READ)
				{
					pool->seq_stream_read++;
					pool->seq_sum_read+=pool->stream[i].sum;
				}
				else
				{
					pool->seq_stream_write++;
					pool->seq_sum_write+=pool->stream[i].sum;
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