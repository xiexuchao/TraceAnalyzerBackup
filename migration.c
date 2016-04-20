#include "migration.h"

void data_migrate(struct pool_info *pool,unsigned int device)
{
	unsigned int i;

	/*
		use multiple FOR loop to make sure that there are enough space in SCM and SSD
		to satisfy the data migration from SSD or HDD.
	*/
	if(device==SCM)
	{
		for(i=pool->chunk_min;i<=pool->chunk_max;i++)
		{
			if(pool->chunk[i].location==SCM)
			{
				if(pool->chunk[i].location==SCM)
				{
					pool->migrate_scm_scm++;
				}
				else if(pool->chunk[i].location_next==HDD)
				{
					if(find_free(pool,HDD)==SUCCESS)
					{
						pool->migrate_scm_hdd++;
						pool->chunk[i].location=HDD;
						pool->free_chk_hdd--;
						pool->free_chk_scm++;
					}	
					else
					{
						printf("No free storage space in HDD ?\n");
					}
				}
				else if(pool->chunk[i].location_next==SSD)
				{
					if(find_free(pool,SSD)==SUCCESS)
					{
						pool->migrate_scm_ssd++;
						pool->chunk[i].location=SSD;
						pool->free_chk_ssd--;
						pool->free_chk_scm++;
					}
					else
					{
						printf("No free storage space in SSD ?\n");
					}

				}
			}//scm
		}//for
	}//if
	else if(device==SSD)
	{
		for(i=pool->chunk_min;i<=pool->chunk_max;i++)
		{
			if(pool->chunk[i].location==SSD)
			{
				if(pool->chunk[i].location==SSD)
				{
					pool->migrate_ssd_ssd++;
				}
				else if(pool->chunk[i].location_next==HDD)
				{
					if(find_free(pool,HDD)==SUCCESS)
					{
						pool->migrate_ssd_hdd++;
						pool->chunk[i].location=HDD;
						pool->free_chk_hdd--;
						pool->free_chk_ssd++;
					}
					else
					{
						printf("No free storage space in HDD ?\n");
					}
				}
				else if(pool->chunk[i].location_next==SCM)
				{
					if(find_free(pool,SCM)==SUCCESS)
					{
						pool->migrate_ssd_scm++;
						pool->chunk[i].location=SCM;
						pool->free_chk_scm--;
						pool->free_chk_ssd++;
					}
					else
					{
						printf("No free storage space in SCM ?\n");
					}
				}
			}//ssd
		}//for
	}//if
	else if(device==HDD)
	{
		for(i=pool->chunk_min;i<=pool->chunk_max;i++)
		{
			if(pool->chunk[i].location==HDD)
			{
				if(pool->chunk[i].location_next==HDD)
				{
					pool->migrate_hdd_hdd++;
				}
				else if(pool->chunk[i].location_next==SCM)
				{
					if(find_free(pool,SCM)==SUCCESS)
					{
						pool->migrate_hdd_scm++;
						pool->chunk[i].location=SCM;
						pool->free_chk_scm--;
						pool->free_chk_hdd++;
					}
					else
					{
						printf("No free storage space in SCM ?\n");
					}
				}
				else if(pool->chunk[i].location_next==SCM)
				{
					if(find_free(pool,SCM)==SUCCESS)
					{
						pool->migrate_hdd_ssd++;
						pool->chunk[i].location=SSD;
						pool->free_chk_ssd--;
						pool->free_chk_hdd++;
					}
					else
					{
						printf("No free storage space in SSD ?\n");
					}
				}
			}//if
		}//for
	}
	else
	{
		printf("Wrong Device! \n");
	}
}//end

int find_free(struct pool_info* pool,unsigned int device)
{
	if(device==SCM)
	{
		//Do GC
		/*while(pool->free_chk_scm < pool->threshold_gc_free)
		{
			printf("while() GC in SCM\n");
			make_free(pool,SCM);
		}*/

		if(pool->free_chk_scm > 0)
		{
			return SUCCESS;
		}
		else
		{
			return FAILURE;
		}
	
	}
	else if(device==SSD)
	{
		//Do GC
	/*	while(pool->free_chk_ssd < pool->threshold_gc_free)
		{
			printf("while() GC in SSD\n");
			make_free(pool,SSD);
		}*/
		
		if(pool->free_chk_ssd > 0)
		{
			return SUCCESS;
		}
		else
		{
			return FAILURE;
		}
	}
	else //HDD
	{
		if(pool->free_chk_hdd > 0)
		{
			return SUCCESS;
		}
		else
		{
			return FAILURE;
		}
	}
}


/*

only for SCM and SSD

LRU ?

*/
int make_free(struct pool_info* pool,unsigned int device)
{
	unsigned int i;

	if(device==SCM)
	{
		for(i=0;i<pool->chunk_sum;i++)
		{
			if((pool->chunk[i].location==SCM)&&(pool->chunk[i].location_next==HDD)) //Migrate to HDD
			{
				pool->migrate_scm_hdd++;
				pool->chunk[i].location=HDD;
				pool->free_chk_hdd--;
				pool->free_chk_scm++;
				return SUCCESS;
			}
		}
		return FAILURE;
	}//if
	else if(device==SSD)
	{
		for(i=0;i<pool->chunk_sum;i++)
		{
			if((pool->chunk[i].location==SSD)&&(pool->chunk[i].location_next==HDD)) //Migrate to HDD
			{
				pool->migrate_ssd_hdd++;
				pool->chunk[i].location=HDD;
				pool->free_chk_hdd--;
				pool->free_chk_ssd++;
				return SUCCESS;
			}
		}
		return FAILURE;
	}//else
	return FAILURE;
}