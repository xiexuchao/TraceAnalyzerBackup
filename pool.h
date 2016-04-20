#ifndef _POOL_H
#define _POOL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "detector.h"
#include "migration.h"

//#define __NETAPP_TRACE__
#define __ASCII_TRACE__

#define SUCCESS		0
#define FAILURE		1
#define READ		0
#define WRITE		1
#define FREE		0
#define BUSY		1

#define SCM			1
#define SSD			2
#define HDD			3

#define SIZE_BUFFER 200
#define SIZE_ARRAY	250

#define WINDOW_DATA	0		//GB IO SIZE
#define WINDOW_TIME	1		//MINUTES

#define PATTERN_UNKNOWN						' '

#define PATTERN_NON_ACCESS					'_'	
#define PATTERN_INACTIVE_R					'I'
#define PATTERN_INACTIVE_W					'i'
#define PATTERN_INACTIVE_H					'!'

#define PATTERN_SEQ_INTENSIVE_R				'1'	
#define PATTERN_SEQ_INTENSIVE_W				'2'	
#define PATTERN_SEQ_INTENSIVE_H				'3'	

#define PATTERN_SEQ_LESS_INTENSIVE_R		'A'	
#define PATTERN_SEQ_LESS_INTENSIVE_W		'B'	
#define PATTERN_SEQ_LESS_INTENSIVE_H		'C'	

#define PATTERN_RANDOM_INTENSIVE_R			'R'	
#define PATTERN_RANDOM_INTENSIVE_W			'W'	
#define PATTERN_RANDOM_INTENSIVE_H			'H'	

#define PATTERN_RANDOM_LESS_INTENSIVE_R		'r'	
#define PATTERN_RANDOM_LESS_INTENSIVE_W		'w'	
#define PATTERN_RANDOM_LESS_INTENSIVE_H		'h'	

struct pool_info{
	/*For Storage Pool*/   
	unsigned int size_scm;
    unsigned int size_ssd;
    unsigned int size_hdd;
    unsigned int size_chunk;
	unsigned int size_subchk;

    unsigned int size_stream;	//sequential detection
    unsigned int size_stride;
    unsigned int size_interval;

    unsigned int chunk_sum;	    //total in storage pool
	unsigned int chunk_sub;		//sub chunks of each chunk
    unsigned int chunk_max;
    unsigned int chunk_min;
    unsigned int chunk_all;	    //chunks accessed in trace file
    unsigned int chunk_win;	    //chunks accessed in one window
    
	unsigned int chunk_scm;
    unsigned int chunk_ssd;
    unsigned int chunk_hdd;
	unsigned int free_chk_scm;
	unsigned int free_chk_ssd;
	unsigned int free_chk_hdd;

    unsigned int window_type;
    unsigned int window_size;
    unsigned int window_sum;
    long long window_time_start;		// ns
    long long window_time_end;			// ns

    double threshold_rw;
    double threshold_cbr;
    double threshold_car;
    unsigned int threshold_sequential;
    unsigned int threshold_inactive;
    unsigned int threshold_intensive;
	unsigned int threshold_gc_free;

	/*For Trace File*/
    long long time_start;
    long long time_end;

    unsigned int req_sum_all;		// IO num
    unsigned int req_sum_read;
    unsigned int req_sum_write;
    long double  req_size_all;		// IO size
    long double  req_size_read;
    long double  req_size_write;

    unsigned int seq_sum_all;		// Seq. IO num
    unsigned int seq_sum_read;
    unsigned int seq_sum_write;
    long double	 seq_size_all;		// Seq. IO size
    long double  seq_size_read;
    long double  seq_size_write;

    unsigned int seq_stream_all;	// Seq. stream num
    unsigned int seq_stream_read;
    unsigned int seq_stream_write;

	unsigned int migrate_scm_scm;	//data migration
	unsigned int migrate_scm_ssd;
	unsigned int migrate_scm_hdd;
	unsigned int migrate_ssd_scm;
	unsigned int migrate_ssd_ssd;
	unsigned int migrate_ssd_hdd;
	unsigned int migrate_hdd_scm;
	unsigned int migrate_hdd_ssd;
	unsigned int migrate_hdd_hdd;

	unsigned int size_in_window;
	unsigned int req_in_window;
	long double  time_in_window;
	double i_non_access;
	double i_inactive;
	double i_seq_intensive;
	double i_seq_less_intensive;
	double i_random_intensive;
	double i_random_less_intensive;

    long double  window_time[SIZE_ARRAY];//s
    unsigned int chunk_access[SIZE_ARRAY];

    char   buffer[SIZE_BUFFER];
    double pattern_non_access[SIZE_ARRAY];
	double pattern_inactive[SIZE_ARRAY];
    double pattern_seq_intensive[SIZE_ARRAY];
    double pattern_seq_less_intensive[SIZE_ARRAY];
    double pattern_random_intensive[SIZE_ARRAY];
    double pattern_random_less_intensive[SIZE_ARRAY];

    char filename_trace[100];
    char filename_output[100];
    char filename_config[100];
    char filename_log[100];
    FILE *file_trace;
    FILE *file_output;
    FILE *file_config;
    FILE *file_log;

    struct map_info		*map;
    struct chunk_info	*chunk;
    struct record_info	*record_win; //how many chks were accessed in a window
    struct record_info	*record_all; //total chks accessed in trace file
    struct request_info *req;
    struct stream_info	*stream;
};

struct chunk_info{
	unsigned int status;
    unsigned int pattern;	
    unsigned int pattern_last;// related to current IO pattern
    unsigned int location;	//SCM,SSD OR HDD
    unsigned int location_next;

    /*information in a window*/
    unsigned int req_sum_all;		// IO num
    unsigned int req_sum_read;
    unsigned int req_sum_write;
    long double  req_size_all;		// IO size
    long double  req_size_read;
    long double  req_size_write;

    unsigned int seq_sum_all;		// Seq. IO num
    unsigned int seq_sum_read;
    unsigned int seq_sum_write;
    long double	 seq_size_all;		// Seq. IO size
    long double  seq_size_read;
    long double  seq_size_write;

    unsigned int seq_stream_all;	// Seq. stream num
    unsigned int seq_stream_read;
    unsigned int seq_stream_write;

	char  history_pattern[SIZE_ARRAY];
    short history_migration[SIZE_ARRAY];

	struct chunk_info *subchk;
};

struct request_info{
    long long time;
    long long lba;
    unsigned int type;//0->Read,1->Write
    unsigned int size;
};

struct map_info{
    unsigned int lcn;	//logical chunk number
    unsigned int pcn;	//physical chunk number
    unsigned int location;
};

struct record_info{
    unsigned int accessed;//accessed or not in a window
};
//initialize.c
void load_parameters(struct pool_info *pool,char *config);
void initialize(struct pool_info *pool,char *trace,char *output,char *log);
//pool.c
int warmup_pool_msr(struct pool_info *pool);
int warmup_pool_netapp(struct pool_info *pool);
int warmup_pool_ascii(struct pool_info *pool);
int get_request_msr(struct pool_info *pool);
int get_request_netapp(struct pool_info *pool);
int get_request_ascii(struct pool_info *pool);
void stat_update(struct pool_info *pool);
void stat_print(struct pool_info *pool);
void alloc_assert(void *p,char *s);
void print_log(struct pool_info *pool,unsigned int i);

void pool_run(char *trace,char *config,char *output,char *log);
void pattern_recognize(struct pool_info *pool);

#endif
