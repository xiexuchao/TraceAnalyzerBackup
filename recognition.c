#include "pool.h"

void pool_run(char *trace,char *config,char *output,char *log)
{
	unsigned int chunk_num;

	struct pool_info *pool;
	pool=(struct pool_info *)malloc(sizeof(struct pool_info));
	alloc_assert(pool,"pool");
	memset(pool,0,sizeof(struct pool_info));

	load_parameters(pool,config);
	initialize(pool,trace,output,log);

#ifdef __NETAPP_TRACE__
	chunk_num=warmup_pool_netapp(pool);
	//fgets(pool->buffer,SIZE_BUFFER,pool->file_trace);	//read the first line out
	while(get_request_netapp(pool)!=FAILURE)
#elif defined __ASCII_TRACE__
	chunk_num=warmup_pool_ascii(pool);
	while(get_request_ascii(pool)!=FAILURE)
#else
	chunk_num=warmup_pool_msr(pool);
	while(get_request_msr(pool)!=FAILURE)
#endif
	{			
		seq_detect(pool);	//Sequential IO Detection
		stat_update(pool);

		//update window info
		pool->size_in_window+=pool->req->size;
		pool->req_in_window++;
		if(pool->req_in_window==1)
		{
			pool->window_time_start=pool->req->time;
		}
		pool->window_time_end=pool->req->time;
		


		if(pool->window_type==WINDOW_DATA)
		{
			//THE CURRENT WINDOW IS FULL
			if((pool->size_in_window>=pool->window_size*2048)||((feof(pool->file_trace)!=0)&&(pool->size_in_window>0)))
			{
				stream_flush(pool);	//Flush information in POOL->STREAMS into each Chunks
				pattern_recognize(pool);
				if(pool->free_chk_scm < pool->threshold_gc_free)
				{
					data_migrate(pool,SCM);
				}
				if(pool->free_chk_ssd < pool->threshold_gc_free)
				{
					data_migrate(pool,SSD);
				}
				data_migrate(pool,HDD);
			}
		}//if
		else if(pool->window_type==WINDOW_TIME)
		{
			
			if(((pool->window_time_end-pool->window_time_start)>=pool->window_size*60*1000*1000)||((feof(pool->file_trace)!=0)&&(pool->window_time_end>0)))
			{

				printf("!!!!!!!!!!!!\n");
				printf("req wind=%d \n",pool->req_in_window);
				printf("req time=%lld \n",pool->req->time);
				printf("strt time=%lld \n",pool->window_time_start);
				printf("end  time=%lld \n",pool->window_time_end);
				printf("window time=%lld \n",pool->window_time_end-pool->window_time_start);

				stream_flush(pool);
				pattern_recognize(pool);
			}
		}
		else
		{
			printf("Wrong Window Type, which should be WINDOW_DATA or WINDOW_TIME\n");
			exit(-1);
		}
	}//while

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


void pattern_recognize(struct pool_info *pool)
{
	unsigned int i;
	/*Pattern Detection*/
	pool->time_in_window=(long double)(pool->window_time_end-pool->window_time_start)/(long double)1000000000;
	
	for(i=pool->chunk_min;i<=pool->chunk_max;i++)
	{
		pool->chunk[i].pattern_last=pool->chunk[i].pattern;

		if(pool->chunk[i].req_sum_all==0)//no access
		{
			/*No Access*/
			if(pool->record_all[i].accessed!=0)
			{
				pool->i_non_access++;
			}
			pool->chunk[i].pattern=PATTERN_NON_ACCESS;
			pool->chunk[i].location_next=HDD;
		}
		else if(pool->chunk[i].req_sum_all < pool->threshold_inactive)//inactive
		{
			/*Inactive*/
			pool->i_inactive++;
			if(((long double)pool->chunk[i].req_sum_read/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
			{
				/*Inactive Read*/
				pool->chunk[i].pattern=PATTERN_INACTIVE_R;
				pool->chunk[i].location_next=HDD;
			}
			else if(((long double)pool->chunk[i].req_sum_write/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
			{
				/*Inactive Write*/
				pool->chunk[i].pattern=PATTERN_INACTIVE_W;
				pool->chunk[i].location_next=HDD;
			}
			else{
				/*Inactive Hybrid*/
				pool->chunk[i].pattern=PATTERN_INACTIVE_H;
				pool->chunk[i].location_next=HDD;
			}
		}
		else if(((long double)pool->chunk[i].seq_size_all/(long double)pool->chunk[i].req_size_all)>=pool->threshold_cbr &&
			((long double)pool->chunk[i].seq_sum_all/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_car)
		{
			/*SEQUENTIAL*/
			/*Sequential Intensive*/
			if(pool->chunk[i].req_sum_all>=(pool->req_in_window/pool->chunk_win)*pool->threshold_intensive)
			{
				pool->i_seq_intensive++;
				if(((long double)pool->chunk[i].req_sum_read/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
				{
					/*Sequential Intensive Read*/
					pool->chunk[i].pattern=PATTERN_SEQ_INTENSIVE_R;
					pool->chunk[i].location_next=SSD;
				}
				else if(((long double)pool->chunk[i].req_sum_write/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
				{
					/*Sequential Intensive Write*/
					pool->chunk[i].pattern=PATTERN_SEQ_INTENSIVE_W;
					pool->chunk[i].location_next=SCM;
				}
				else
				{
					/*Sequential Intensive Hybrid*/
					pool->chunk[i].pattern=PATTERN_SEQ_INTENSIVE_H;
					pool->chunk[i].location_next=SCM;
				}
			}
			else
			{
				pool->i_seq_less_intensive++;
				if(((long double)pool->chunk[i].req_sum_read/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
				{
					/*Sequential Less Intensive Read*/
					pool->chunk[i].pattern=PATTERN_SEQ_LESS_INTENSIVE_R;
					pool->chunk[i].location_next=HDD;
				}
				else if(((long double)pool->chunk[i].req_sum_write/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
				{
					/*Sequential Less Intensive Write*/
					pool->chunk[i].pattern=PATTERN_SEQ_LESS_INTENSIVE_W;
					pool->chunk[i].location_next=HDD;
				}
				else
				{
					/*Sequential Less Intensive Hybrid*/
					pool->chunk[i].pattern=PATTERN_SEQ_LESS_INTENSIVE_H;
					pool->chunk[i].location_next=HDD;
				}
			}
		}
		else
		{
			/*Random*/
			if(pool->chunk[i].req_sum_all>=(pool->req_in_window/pool->chunk_win)*pool->threshold_intensive)
			{
				pool->i_random_intensive++;
				if(((long double)pool->chunk[i].req_sum_read/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
				{
					/*Random Intensive Read*/
					pool->chunk[i].pattern=PATTERN_RANDOM_INTENSIVE_R;
					pool->chunk[i].location_next=SSD;
				}
				else if(((long double)pool->chunk[i].req_sum_write/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
				{
					/*Random Intensive Write*/
					pool->chunk[i].pattern=PATTERN_RANDOM_INTENSIVE_W;
					pool->chunk[i].location_next=SCM;
				}
				else
				{
					/*Random Intensive Hybrid*/
					pool->chunk[i].pattern=PATTERN_RANDOM_INTENSIVE_H;
					pool->chunk[i].location_next=SCM;
				}
			}
			else
			{
				pool->i_random_less_intensive++;
				if(((long double)pool->chunk[i].req_sum_read/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
				{
					/*Random Less Intensive Read*/
					pool->chunk[i].pattern=PATTERN_RANDOM_LESS_INTENSIVE_R;
					pool->chunk[i].location_next=SSD;
				}
				else if(((long double)pool->chunk[i].req_sum_write/(long double)pool->chunk[i].req_sum_all)>=pool->threshold_rw)
				{
					/*Random Less Intensive Write*/
					pool->chunk[i].pattern=PATTERN_RANDOM_LESS_INTENSIVE_W;
					pool->chunk[i].location_next=SSD;
				}
				else
				{
					/*Random Less Intensive Hybrid*/
					pool->chunk[i].pattern=PATTERN_RANDOM_LESS_INTENSIVE_H;
					pool->chunk[i].location_next=SSD;
				}
			}
		}
		//Only record limited information (the first SIZE_ARRY windows)
		if(pool->window_sum<SIZE_ARRAY)
		{
			pool->window_time[pool->window_sum]=pool->time_in_window;
			pool->chunk_access[pool->window_sum]=pool->chunk_win;

			pool->chunk[i].history_pattern[pool->window_sum]=pool->chunk[i].pattern;

			pool->pattern_non_access[pool->window_sum]=pool->i_non_access/(double)pool->chunk_all;
			pool->pattern_inactive[pool->window_sum]=pool->i_inactive/(double)pool->chunk_all;
			pool->pattern_seq_intensive[pool->window_sum]=pool->i_seq_intensive/(double)pool->chunk_all;
			pool->pattern_seq_less_intensive[pool->window_sum]=pool->i_seq_less_intensive/(double)pool->chunk_all;
			pool->pattern_random_intensive[pool->window_sum]=pool->i_random_intensive/(double)pool->chunk_all;
			pool->pattern_random_less_intensive[pool->window_sum]=pool->i_random_less_intensive/(double)pool->chunk_all;
		}

		print_log(pool,i);	//print info of each chunk in this window to log file.
		/*Initialize the statistics in each chunk*/
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
	}//for

	/*Update the pool info*/
	pool->window_sum++;
	if(pool->window_sum%50==0)
	{
		printf("------------pool->window_sum=%d---------------\n",pool->window_sum);
	}
	pool->window_time_start=0;
	pool->window_time_end=0;

	/*Start a new window*/
	pool->size_in_window=0;
	pool->req_in_window=0;
	pool->time_in_window=0;

	pool->i_non_access=0;
	pool->i_inactive=0;
	pool->i_seq_intensive=0;
	pool->i_seq_less_intensive=0;
	pool->i_random_intensive=0;
	pool->i_random_less_intensive=0;

	//accessed chunks in each window
	memset(pool->record_win,0,sizeof(struct record_info)*pool->chunk_sum);
	printf("pool->chunk_win=%d\n",pool->chunk_win);
	pool->chunk_win=0;
}