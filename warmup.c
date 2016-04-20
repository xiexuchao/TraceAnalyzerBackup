#include "pool.h"

int warmup_pool_msr(struct pool_info *pool)
{
	int i,j=0;
	long long req_timestamp,req_offset;
	char req_hostname[10],req_type[10];
	unsigned int req_disknumber,req_size,req_responsetime;
	long long lba_max=0,lba_min=0x7fffffffffffffff;
	unsigned int chk_id;

	while(fgets(pool->buffer,SIZE_BUFFER,pool->file_trace))
	{
		for(i=0;i<sizeof(pool->buffer);i++)
		{
			if(pool->buffer[i]==',')
			{
				pool->buffer[i]=' ';
			}
		}
		sscanf(pool->buffer,"%lld %s %d %s %lld %d %d\n",&req_timestamp,req_hostname,
			&req_disknumber,req_type,&req_offset,&req_size,&req_responsetime);
		if((req_timestamp<0)||(req_disknumber<0)||(req_offset<0)||(req_size<0)||(req_responsetime<0))
		{
			printf("get_request_msr()--Error in Trace File!\n");
			printf("%s\n",pool->buffer);
			exit(-1);
		}
		j++;
		if(j%1000000==0)
		{
			printf("scanning(%s)%d\n",pool->filename_trace,j);
		}
		if(j==1)
		{
			pool->time_start=req_timestamp;
		}
		pool->time_end=req_timestamp;

		if(req_offset<lba_min)
		{
			lba_min=req_offset;
		}
		if(req_offset>lba_max)
		{
			lba_max=req_offset;
		}

		chk_id=(unsigned int)((req_offset/512)/(pool->size_chunk*2048));
		if(pool->record_all[chk_id].accessed==0)
		{
			pool->chunk_all++;
			pool->record_all[chk_id].accessed=1;
			if(pool->chunk[chk_id].status==FREE)
			{
				pool->chunk[chk_id].status=BUSY;
				if(chk_id<pool->chunk_scm)
				{
					pool->chunk[chk_id].location=SCM;
					pool->free_chk_scm--;
				}
				else if(chk_id<(pool->chunk_ssd+pool->chunk_scm))
				{
					pool->chunk[chk_id].location=SSD;
					pool->free_chk_ssd--;
				}
				else
				{
					pool->chunk[chk_id].location=HDD;
					pool->free_chk_hdd--;
				}
			}
		}
	}
	pool->chunk_min=(int)((lba_min/512)/(pool->size_chunk*1024*2));
	pool->chunk_max=(int)((lba_max/512)/(pool->size_chunk*1024*2));
	printf("------------------------Chunks: min=%d, max=%d, total=%d, exactly=%d------------------------\n",
		pool->chunk_min,pool->chunk_max,pool->chunk_max-pool->chunk_min+1,pool->chunk_all);

	fclose(pool->file_trace);
	pool->file_trace=fopen(pool->filename_trace,"r");

	return pool->chunk_max-pool->chunk_min+1;
}

int warmup_pool_netapp(struct pool_info *pool)
{
	unsigned int i,j=0;
	long double elapsed;
	char cmd[10];
	int lun_ssid,op,phase,nblks,host_id;
	long long lba;
	long long lba_max=0,lba_min=0x7fffffffffffffff;
	unsigned int chk_id;

	//fgets(pool->buffer,SIZE_BUFFER,pool->file_trace);	//read the first line out
	while(fgets(pool->buffer,SIZE_BUFFER,pool->file_trace))
	{
		for(i=0;i<sizeof(pool->buffer);i++)
		{
			if(pool->buffer[i]==',')
			{
				pool->buffer[i]=' ';
			}
		}
		sscanf(pool->buffer,"%Lf %s %d %d %d %lld %d %d\n",&elapsed,cmd,
			&lun_ssid,&op,&phase,&lba,&nblks,&host_id);
		if((elapsed<0)||(lun_ssid<0)||(op<0)||(phase<0)||(lba<0)||(nblks<0)||(host_id<0))
		{
			printf("warmup_pool_netapp()--Error in Trace File!\n");
			printf("%s\n",pool->buffer);
			exit(-1);
		}
		j++;
		if(j%1000000==0)
		{
			printf("scanning(%s)%d\n",pool->filename_trace,j);
		}
		if(j==1)
		{
			pool->time_start=(long long)(elapsed*1000);
		}
		pool->time_end=(long long)(elapsed*1000);

		if(lba<lba_min)
		{
			lba_min=lba;
		}
		if(lba>lba_max)
		{
			lba_max=lba;
		}

		chk_id=(int)(lba/(pool->size_chunk*2048));
		if(pool->record_all[chk_id].accessed==0)
		{
			pool->chunk_all++;
			pool->record_all[chk_id].accessed=1;
			if(pool->chunk[chk_id].status==FREE)
			{
				pool->chunk[chk_id].status=BUSY;
				if(chk_id<pool->chunk_scm)
				{
					pool->chunk[chk_id].location=SCM;
					pool->free_chk_scm--;
				}
				else if(chk_id<(pool->chunk_ssd+pool->chunk_scm))
				{
					pool->chunk[chk_id].location=SSD;
					pool->free_chk_ssd--;
				}
				else
				{
					pool->chunk[chk_id].location=HDD;
					pool->free_chk_hdd--;
				}
			}
		}
	}
	pool->chunk_min=(int)(lba_min/(pool->size_chunk*1024*2));
	pool->chunk_max=(int)(lba_max/(pool->size_chunk*1024*2));
	printf("------------------------Chunks: min=%d, max=%d, total=%d, exactly=%d------------------------\n",
		pool->chunk_min,pool->chunk_max,pool->chunk_max-pool->chunk_min+1,pool->chunk_all);

	fclose(pool->file_trace);
	pool->file_trace=fopen(pool->filename_trace,"r");

	return pool->chunk_max-pool->chunk_min+1;
}


int warmup_pool_ascii(struct pool_info *pool)
{
	unsigned int j=0;
	long double elapsed;
	int lun_ssid,op,nblks;
	long long lba;
	long long lba_max=0,lba_min=0x7fffffffffffffff;
	unsigned int chk_id;

	while(fgets(pool->buffer,SIZE_BUFFER,pool->file_trace))
	{
		sscanf(pool->buffer,"%Lf %d %lld %d %d\n",&elapsed,&lun_ssid,&lba,&nblks,&op);
		if((elapsed<0)||(lun_ssid<0)||(op<0)||(lba<0)||(nblks<0))
		{
			printf("warmup_pool_ascii()--Error in Trace File!\n");
			printf("%s\n",pool->buffer);
			exit(-1);
		}
		j++;
		if(j%1000000==0)
		{
			printf("scanning(%s)%d\n",pool->filename_trace,j);
		}
		if(j==1)
		{
			pool->time_start=(long long)(elapsed*1000);
		}
		pool->time_end=(long long)(elapsed*1000);

		if(lba<lba_min)
		{
			lba_min=lba;
		}
		if(lba>lba_max)
		{
			lba_max=lba;
		}

		chk_id=(int)(lba/(pool->size_chunk*2048));
		if(pool->record_all[chk_id].accessed==0)
		{
			pool->chunk_all++;
			pool->record_all[chk_id].accessed=1;
			if(pool->chunk[chk_id].status==FREE)
			{
				pool->chunk[chk_id].status=BUSY;
				if(chk_id<pool->chunk_scm)
				{
					pool->chunk[chk_id].location=SCM;
					pool->free_chk_scm--;
				}
				else if(chk_id<(pool->chunk_ssd+pool->chunk_scm))
				{
					pool->chunk[chk_id].location=SSD;
					pool->free_chk_ssd--;
				}
				else
				{
					pool->chunk[chk_id].location=HDD;
					pool->free_chk_hdd--;
				}
			}
		}
	}
	pool->chunk_min=(int)(lba_min/(pool->size_chunk*1024*2));
	pool->chunk_max=(int)(lba_max/(pool->size_chunk*1024*2));
	printf("------------------------Chunks: min=%d, max=%d, total=%d, exactly=%d------------------------\n",
		pool->chunk_min,pool->chunk_max,pool->chunk_max-pool->chunk_min+1,pool->chunk_all);

	fclose(pool->file_trace);
	pool->file_trace=fopen(pool->filename_trace,"r");

	return pool->chunk_max-pool->chunk_min+1;
}