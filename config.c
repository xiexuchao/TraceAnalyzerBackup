#include "pool.h"
#include "detector.h"

void load_parameters(struct pool_info *pool,char *config)
{
    int name,value;
    char buf[SIZE_BUFFER];
    char *ptr;

    memset(buf,0,sizeof(char)*SIZE_BUFFER);
    strcpy(pool->filename_config,config);
    pool->file_config=fopen(pool->filename_config,"r");

    while(fgets(buf,sizeof(buf),pool->file_config))
    {
        if(buf[0]=='#'||buf[0]==' ') continue;
        ptr=strchr(buf,'=');
        if(!ptr) continue;
        name=ptr-buf;	//the end of name string+1
        value=name+1;	//the start of value string
        while(buf[name-1]==' ') name--;
        buf[name]=0;

		if(strcmp(buf,"size of scm")==0)
		{
			sscanf(buf+value,"%d",&pool->size_scm);
		}
		else if(strcmp(buf,"size of ssd")==0)
		{
			sscanf(buf+value,"%d",&pool->size_ssd);
		}
		else if(strcmp(buf,"size of hdd")==0)
		{
			sscanf(buf+value,"%d",&pool->size_hdd);
		}
		else if(strcmp(buf,"size of chunk")==0)
		{
			sscanf(buf+value,"%d",&pool->size_chunk);
		}
		else if(strcmp(buf,"size of subchk")==0)
		{
			sscanf(buf+value,"%d",&pool->size_subchk);
		}
		else if(strcmp(buf,"window type")==0)
		{
			sscanf(buf+value,"%d",&pool->window_type);
		}
		else if(strcmp(buf,"window size")==0)
		{
			sscanf(buf+value,"%d",&pool->window_size);
		}
		else if(strcmp(buf,"threshold rw")==0)
		{
			sscanf(buf+value,"%lf",&pool->threshold_rw);
		}
		else if(strcmp(buf,"threshold cbr")==0)
		{
			sscanf(buf+value,"%lf",&pool->threshold_cbr);
		}
		else if(strcmp(buf,"threshold car")==0)
		{
			sscanf(buf+value,"%lf",&pool->threshold_car);
		}
		else if(strcmp(buf,"threshold seq")==0)
		{
			sscanf(buf+value,"%d",&pool->threshold_sequential);
		}
		else if(strcmp(buf,"threshold inactive")==0)
		{	
			sscanf(buf+value,"%d",&pool->threshold_inactive);
		}
		else if(strcmp(buf,"threshold intensive")==0)
		{
			sscanf(buf+value,"%d",&pool->threshold_intensive);
		}
		else if(strcmp(buf,"threshold free gc")==0)
		{
			sscanf(buf+value,"%d",&pool->threshold_gc_free);
		}
		else if(strcmp(buf,"size of stream")==0)
		{
			sscanf(buf+value,"%d",&pool->size_stream);
		}
		else if(strcmp(buf,"size of stride")==0)
		{
			sscanf(buf+value,"%d",&pool->size_stride);
		}
		else if(strcmp(buf,"size of interval")==0)
		{
			sscanf(buf+value,"%d",&pool->size_interval);
		}
        memset(buf,0,sizeof(char)*SIZE_BUFFER);
    }
    fclose(pool->file_config);
}

void initialize(struct pool_info *pool,char *trace,char *output,char *log)
{
    unsigned int i,j;

    pool->chunk_sum=((pool->size_scm+pool->size_ssd+pool->size_hdd)*1024-1)/pool->size_chunk+1;
	pool->chunk_sub=pool->size_chunk/pool->size_subchk; //size_chunk%size_subchk must equal to 0 in config.txt
    pool->chunk_max=0;
    pool->chunk_min=0;
    pool->chunk_all=0;
    pool->chunk_win=0;

    pool->chunk_scm=1024*pool->size_scm/pool->size_chunk;
    pool->chunk_ssd=1024*pool->size_ssd/pool->size_chunk;
    pool->chunk_hdd=1024*pool->size_hdd/pool->size_chunk;
	pool->free_chk_scm=pool->chunk_scm;
	pool->free_chk_ssd=pool->chunk_ssd;
	pool->free_chk_hdd=pool->chunk_hdd;

    pool->window_sum=0;
    pool->window_time_start=0;
    pool->window_time_end=0;

    pool->time_start=0;
    pool->time_end=0;

    pool->req_sum_all=0;
    pool->req_sum_read=0;
    pool->req_sum_write=0;
    pool->req_size_all=0;
    pool->req_size_read=0;
    pool->req_size_write=0;

    pool->seq_sum_all=0;
    pool->seq_sum_read=0;
    pool->seq_sum_write=0;	
    pool->seq_size_all=0;
    pool->seq_size_read=0;
    pool->seq_size_write=0;

    pool->seq_stream_all=0;
    pool->seq_stream_read=0;
    pool->seq_stream_write=0;

	pool->migrate_scm_scm=0;	//data migration
	pool->migrate_scm_ssd=0;
	pool->migrate_scm_hdd=0;
	pool->migrate_ssd_scm=0;
	pool->migrate_ssd_ssd=0;
	pool->migrate_ssd_hdd=0;
	pool->migrate_hdd_scm=0;
	pool->migrate_hdd_ssd=0;
	pool->migrate_hdd_hdd=0;

	pool->size_in_window=0;
	pool->req_in_window=0;
	pool->time_in_window=0;
	pool->i_non_access=0;
	pool->i_inactive=0;
	pool->i_seq_intensive=0;
	pool->i_seq_less_intensive=0;
	pool->i_random_intensive=0;
	pool->i_random_less_intensive=0;

    for(i=0;i<SIZE_ARRAY;i++)
    {
        pool->window_time[i]=0;
        pool->chunk_access[i]=0;

        pool->pattern_non_access[i]=0;
        pool->pattern_inactive[i]=0;
        pool->pattern_seq_intensive[i]=0;
        pool->pattern_seq_less_intensive[i]=0;
        pool->pattern_random_intensive[i]=0;
        pool->pattern_random_less_intensive[i]=0;
    }

	strcpy(pool->filename_trace,trace);
    strcpy(pool->filename_output,output);
    strcpy(pool->filename_log,log);
    pool->file_trace=fopen(pool->filename_trace,"r");
    pool->file_output=fopen(pool->filename_output,"w");
    pool->file_log=fopen(pool->filename_log,"w");

	pool->chunk=(struct chunk_info *)malloc(sizeof(struct chunk_info)*pool->chunk_sum);
	alloc_assert(pool->chunk,"pool->chunk");
	memset(pool->chunk,0,sizeof(struct chunk_info)*pool->chunk_sum);
	pool->req=(struct request_info *)malloc(sizeof(struct request_info));
	alloc_assert(pool->req,"pool->req");
	memset(pool->req,0,sizeof(struct request_info));
	pool->stream=(struct stream_info *)malloc(sizeof(struct stream_info)*pool->size_stream);
	alloc_assert(pool->stream,"pool->stream");
	memset(pool->stream,0,sizeof(struct stream_info)*pool->size_stream);
	pool->map=(struct map_info *)malloc(sizeof(struct map_info)*pool->chunk_sum);
	alloc_assert(pool->map,"pool->map");
	memset(pool->map,0,sizeof(struct map_info));
	pool->record_win=(struct record_info *)malloc(sizeof(struct record_info)*pool->chunk_sum);
	alloc_assert(pool->record_win,"pool->record_win");
	memset(pool->record_win,0,sizeof(struct record_info)*pool->chunk_sum);
	pool->record_all=(struct record_info *)malloc(sizeof(struct record_info)*pool->chunk_sum);
	alloc_assert(pool->record_all,"pool->record_all");
	memset(pool->record_all,0,sizeof(struct record_info)*pool->chunk_sum);

    printf("-------------Initializing...chunk_sum=%d------------\n",pool->chunk_sum);
    for(i=0;i<pool->chunk_sum;i++)
    {
        pool->map[i].lcn=i;
        pool->map[i].pcn=i;

        pool->record_win[i].accessed=0;
        pool->record_all[i].accessed=0;
       
		pool->chunk[i].status=FREE;
        pool->chunk[i].pattern=PATTERN_UNKNOWN;
        pool->chunk[i].pattern_last=PATTERN_UNKNOWN;
		pool->chunk[i].location=HDD;
        pool->chunk[i].location_next=HDD;

        pool->chunk[i].req_sum_all=0;
        pool->chunk[i].req_sum_read=0;
        pool->chunk[i].req_sum_write=0;
        pool->chunk[i].req_size_all=0;
        pool->chunk[i].req_size_read=0;
        pool->chunk[i].req_size_write=0;

        pool->chunk[i].seq_sum_all=0;
        pool->chunk[i].seq_sum_read=0;
        pool->chunk[i].seq_sum_write=0;
        pool->chunk[i].seq_size_all=0;
        pool->chunk[i].seq_size_read=0;
        pool->chunk[i].seq_size_write=0;

        pool->chunk[i].seq_stream_all=0;
        pool->chunk[i].seq_stream_read=0;
        pool->chunk[i].seq_stream_write=0;

		for(j=0;j<SIZE_ARRAY;j++)
        {
            pool->chunk[i].history_pattern[j]=' ';
            pool->chunk[i].history_migration[j]=0;
        }

		pool->chunk[i].subchk=(struct chunk_info *)malloc(sizeof(struct chunk_info)*pool->chunk_sub);
		alloc_assert(pool->chunk[i].subchk,"pool->chunk[i].subchk");
		memset(pool->chunk[i].subchk,0,sizeof(struct chunk_info)*pool->chunk_sub);
		for(j=0;j<pool->chunk_sub;j++)
		{			
			pool->chunk[i].subchk[j].status=FREE;
			pool->chunk[i].subchk[j].pattern=PATTERN_UNKNOWN;
			pool->chunk[i].subchk[j].pattern_last=PATTERN_UNKNOWN;
			pool->chunk[i].subchk[j].location=pool->chunk[i].location;
			pool->chunk[i].subchk[j].location_next=pool->chunk[i].location_next;

			pool->chunk[i].subchk[j].req_sum_all=0;
			pool->chunk[i].subchk[j].req_sum_read=0;
			pool->chunk[i].subchk[j].req_sum_write=0;
			pool->chunk[i].subchk[j].req_size_all=0;
			pool->chunk[i].subchk[j].req_size_read=0;
			pool->chunk[i].subchk[j].req_size_write=0;

			pool->chunk[i].subchk[j].seq_sum_all=0;
			pool->chunk[i].subchk[j].seq_sum_read=0;
			pool->chunk[i].subchk[j].seq_sum_write=0;
			pool->chunk[i].subchk[j].seq_size_all=0;
			pool->chunk[i].subchk[j].seq_size_read=0;
			pool->chunk[i].subchk[j].seq_size_write=0;

			pool->chunk[i].subchk[j].seq_stream_all=0;
			pool->chunk[i].subchk[j].seq_stream_read=0;
			pool->chunk[i].subchk[j].seq_stream_write=0;
		}

    }

    pool->req->time=0;
    pool->req->lba=0;
    pool->req->type=0;
    pool->req->size=0;

    for(i=0;i<pool->size_stream;i++)
    {
        pool->stream[i].chk_id=0;
        pool->stream[i].type=0;
        pool->stream[i].sum=0;
        pool->stream[i].size=0;		
        pool->stream[i].min=0;
        pool->stream[i].max=0;
        pool->stream[i].time=0;
    }
}
